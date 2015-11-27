//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include "SharedConstants.h"
#include "EVSM.hlsl"
#include <SH.hlsl>
#include "AppSettings.hlsl"

#define UseMaps_ (UseNormalMapping_ || UseAlbedoMap_ || UseMetallicMap_ || UseRoughnessMap_ || UseEmissiveMap_)

#if IsGBuffer_
	#include "Surface.hlsli"
#endif

//=================================================================================================
// Constants
//=================================================================================================
static const uint NumCascades = 4;

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer VSConstants : register(b0)
{
    float4x4 World;
	float4x4 View;
    float4x4 WorldViewProjection;
    float4x4 PrevWorldViewProjection;
}

cbuffer PSConstants : register(b0)
{
	float3 CameraPosWS;
	float4x4 ShadowMatrix;
	float4 CascadeSplits;
	float4 CascadeOffsets[NumCascades];
	float4 CascadeScales[NumCascades];
	float OffsetScale;
	float PositiveExponent;
	float NegativeExponent;
	float LightBleedingReduction;
	float4x4 View_; // used in gbuffer branch to calc view space normal
	float4x4 Projection;
	SH9Color EnvironmentSH;
	float2 RTSize;
	float2 JitterOffset;
	float3 ProbePositionWS[2];
	float3 BoxSize[2];
	float3 ObjPositionWS;
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2DArray ShadowMap : register(t2);
TextureCube<float3> SpecularCubemap : register(t3);
Texture2D<float2> SpecularCubemapLookup : register(t4);

Texture2D RoughnessMap : register(t5);
Texture2D MetallicMap : register(t6);
Texture2D EmissiveMap : register(t7);
//TextureCubeArray<float3> SpecularCubemapArray : register(t8);
TextureCube<float3> SpecularCubemapArray1 : register(t8);
TextureCube<float3> SpecularCubemapArray2 : register(t9);

SamplerState AnisoSampler : register(s0);
SamplerState EVSMSampler : register(s1);
SamplerState LinearSampler : register(s2);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
    float3 NormalOS 		    : NORMAL;

	#if UseMaps_
		float2 UV				: TEXCOORD;
	#endif

    #if UseNormalMapping_
        float3 TangentOS        : TANGENT;
        float3 BitangentOS      : BITANGENT;
    #endif
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;

    float3 NormalWS 		    : NORMALWS;
    float DepthVS			    : DEPTHVS;

    float3 PositionWS 		    : POSITIONWS;

    float3 PrevPosition         : PREVPOSITION;

	#if UseMaps_
		float2 UV				: UV;
	#endif

    #if UseNormalMapping_
        float3 TangentWS        : TANGENT;
        float3 BitangentWS      : BITANGENT;
    #endif
};

#if CentroidSampling_
    #define SampleMode_ centroid
#else
    #define SampleMode_
#endif

struct PSInput
{
    SampleMode_ float4 PositionSS 		    : SV_Position;

    SampleMode_ float3 NormalWS 		    : NORMALWS;
    SampleMode_ float DepthVS			    : DEPTHVS;

    SampleMode_ float3 PositionWS 		    : POSITIONWS;

    SampleMode_ float3 PrevPosition         : PREVPOSITION;

	#if UseMaps_
		SampleMode_ float2 UV				: UV;
	#endif

    #if UseNormalMapping_
        SampleMode_ float3 TangentWS        : TANGENT;
        SampleMode_ float3 BitangentWS      : BITANGENT;
    #endif
};

struct PSOutput
{
	#if CreateCubemap_ && !IsGBuffer_
		float4 Color                : SV_Target0;
	#elif IsGBuffer_
		float4 RT0					: SV_Target0;	// albedo (xyz) 
		float4 RT1					: SV_Target1;	// roughness(x) | metallic(y) | emissive (z) | SSR? (w)
		float4 RT2					: SV_Target2;	// spheremap_vs_normal (xy) | velocity (zw)
	#else
		float4 Color                : SV_Target0;
		float2 Velocity             : SV_Target1;
	#endif
};

//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input, in uint VertexID : SV_VertexID)
{
    VSOutput output;

    // Calc the world-space position
    output.PositionWS = mul(float4(input.PositionOS, 1.0f), World).xyz;

	// Calc the view-space depth
	output.DepthVS = mul(float4(output.PositionWS, 1.0f), View).z;

    // Calc the clip-space position
    output.PositionCS = mul(float4(input.PositionOS, 1.0f), WorldViewProjection);

	// Rotate the normal into world space
    output.NormalWS = normalize(mul(input.NormalOS, (float3x3)World));

    output.PrevPosition = mul(float4(input.PositionOS, 1.0f), PrevWorldViewProjection).xyw;

	#if UseMaps_
		output.UV = input.UV;
	#endif

    #if UseNormalMapping_
        output.TangentWS = normalize(mul(input.TangentOS, (float3x3)World));
        output.BitangentWS = normalize(mul(input.BitangentOS, (float3x3)World));
    #endif

    return output;
}

//-------------------------------------------------------------------------------------------------
// Samples the EVSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapEVSM(in float3 shadowPos, in float3 shadowPosDX,
                          in float3 shadowPosDY, uint cascadeIdx)
{
    float2 exponents = GetEVSMExponents(PositiveExponent, NegativeExponent,
                                        CascadeScales[cascadeIdx].xyz);
    float2 warpedDepth = WarpDepth(shadowPos.z, exponents);

    float4 occluder = ShadowMap.SampleGrad(EVSMSampler, float3(shadowPos.xy, cascadeIdx),
                                            shadowPosDX.xy, shadowPosDY.xy);

    // Derivative of warping at depth
    float2 depthScale = 0.0001f * exponents * warpedDepth;
    float2 minVariance = depthScale * depthScale;

    float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x, LightBleedingReduction);
    float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y, LightBleedingReduction);
    float shadowContrib = posContrib;
    shadowContrib = min(shadowContrib, negContrib);

    return shadowContrib;
}

//-------------------------------------------------------------------------------------------------
// Samples the appropriate shadow map cascade
//-------------------------------------------------------------------------------------------------
float SampleShadowCascade(in float3 shadowPosition, in float3 shadowPosDX,
                          in float3 shadowPosDY, in uint cascadeIdx)
{
    shadowPosition += CascadeOffsets[cascadeIdx].xyz;
    shadowPosition *= CascadeScales[cascadeIdx].xyz;

    shadowPosDX *= CascadeScales[cascadeIdx].xyz;
    shadowPosDY *= CascadeScales[cascadeIdx].xyz;

    float shadow = SampleShadowMapEVSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);

    return shadow;
}

//--------------------------------------------------------------------------------------
// Computes the visibility term by performing the shadow test
//--------------------------------------------------------------------------------------
float ShadowVisibility(in float3 positionWS, in float depthVS)
{
	float shadowVisibility = 1.0f;
	uint cascadeIdx = 0;

    // Project into shadow space
    float3 samplePos = positionWS;
	float3 shadowPosition = mul(float4(samplePos, 1.0f), ShadowMatrix).xyz;
    float3 shadowPosDX = ddx(shadowPosition);
    float3 shadowPosDY = ddy(shadowPosition);

	// Figure out which cascade to sample from
	[unroll]
	for(uint i = 0; i < NumCascades - 1; ++i)
	{
		[flatten]
		if(depthVS > CascadeSplits[i])
			cascadeIdx = i + 1;
	}

	shadowVisibility = SampleShadowCascade(shadowPosition, shadowPosDX, shadowPosDY,
                                           cascadeIdx);

	return shadowVisibility;
}

// ================================================================================================
// Calculates the Fresnel factor using Schlick's approximation
// ================================================================================================
float3 Fresnel(in float3 specAlbedo, in float3 h, in float3 l)
{
    float3 fresnel = specAlbedo + (1.0f - specAlbedo) * pow((1.0f - saturate(dot(l, h))), 5.0f);

    // Fade out spec entirely when lower than 0.1% albedo
    fresnel *= saturate(dot(specAlbedo, 333.0f));

    return fresnel;
}

// ===============================================================================================
// Helper for computing the GGX visibility term
// ===============================================================================================
float GGX_V1(in float m2, in float nDotX)
{
    return 1.0f / (nDotX + sqrt(m2 + (1 - m2) * nDotX * nDotX));
}

// ===============================================================================================
// Computes the specular term using a GGX microfacet distribution, with a matching
// geometry factor and visibility term. Based on "Microfacet Models for Refraction Through
// Rough Surfaces" [Walter 07]. m is roughness, n is the surface normal, h is the half vector,
// l is the direction to the light source, and specAlbedo is the RGB specular albedo
// ===============================================================================================
float GGX_Specular(in float m, in float3 n, in float3 h, in float3 v, in float3 l)
{
    float nDotH = saturate(dot(n, h));
    float nDotL = saturate(dot(n, l));
    float nDotV = saturate(dot(n, v));

    float nDotH2 = nDotH * nDotH;
    float m2 = m * m;

    // Calculate the distribution term
    float d = m2 / (Pi * pow(nDotH * nDotH * (m2 - 1) + 1, 2.0f));

    // Calculate the matching visibility term
    float v1i = GGX_V1(m2, nDotL);
    float v1o = GGX_V1(m2, nDotV);
    float vis = v1i * v1o;

    return d * vis;
}

float3 CalcLighting(in float3 normal, in float3 lightDir, in float3 lightColor,
					in float3 diffuseAlbedo, in float3 specularAlbedo, in float roughness,
					in float3 positionWS)
{
    float3 lighting = diffuseAlbedo * (1.0f / 3.14159f);

    float3 view = normalize(CameraPosWS - positionWS);
    const float nDotL = saturate(dot(normal, lightDir));
    if(nDotL > 0.0f)
    {
        const float nDotV = saturate(dot(normal, view));
        float3 h = normalize(view + lightDir);

        float specular = GGX_Specular(roughness, normal, h, view, lightDir);
        float3 fresnel = Fresnel(specularAlbedo, h, lightDir);

        lighting += specular * fresnel;
    }

    return lighting * nDotL * lightColor;
}


float3 parallaxCorrection(float3 PositionWS, float3 newProbePositionWS, float3 NormalWS, float3 boxMax, float3 boxMin)
{
	float3 DirectionWS = normalize(PositionWS - CameraPosWS);
	float3 ReflDirectionWS = reflect(DirectionWS, NormalWS);

	float3 FirstPlaneIntersect = (boxMax - PositionWS) / ReflDirectionWS;
	float3 SecondPlaneIntersect = (boxMin - PositionWS) / ReflDirectionWS;

	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);

	float Distance = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

	float3 IntersectPositionWS = PositionWS + ReflDirectionWS * Distance;

		ReflDirectionWS = IntersectPositionWS - newProbePositionWS;

	return ReflDirectionWS;
}


float probeWeightCalculate(float3 ProbePosition, float3 PositionWS, float3 BoxSize)
{
	float3 distance = ProbePosition - PositionWS;
	float3 absDistance = float3(abs(distance.x), abs(distance.y), abs(distance.z));

	float3 weight = (BoxSize - absDistance) / BoxSize;
	if (weight.x < 0 || weight.y < 0 || weight.z < 0) return 0;
	else
	{
		return max(weight.x, max(weight.y, weight.z));
	}
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
PSOutput PS(in PSInput input)
{
	float3 vtxNormal = normalize(input.NormalWS);
    float3 positionWS = input.PositionWS;
    float3 viewWS = normalize(CameraPosWS - positionWS);
    float3 normalWS = vtxNormal;

    #if UseNormalMapping_
        float3 normalTS = float3(0, 0, 1);
        float3 tangentWS = normalize(input.TangentWS);
        float3 bitangentWS = normalize(input.BitangentWS);
        float3x3 tangentToWorld = float3x3(tangentWS, bitangentWS, normalWS);

        if(EnableNormalMaps)
        {
            // Sample the normal map, and convert the normal to world space
            normalTS.xy = NormalMap.Sample(AnisoSampler, input.UV).xy * 2.0f - 1.0f;
            normalTS.z = sqrt(1.0f - saturate(normalTS.x * normalTS.x + normalTS.y * normalTS.y));
            normalTS = lerp(float3(0, 0, 1), normalTS, NormalMapIntensity);
            normalWS = normalize(mul(normalTS, tangentToWorld));
        }
    #endif

	float4 albedoMap = 1.0f;
	float4 roughnessMap = 1.0f;
	float4 metallicMap = 1.0f;
	float4 emissiveMap = 1.0f;

	#if UseAlbedoMap_
		albedoMap = AlbedoMap.Sample(AnisoSampler, input.UV);
	#endif

	// TODO: optimize - combine the maps in one texture in different channels
	#if UseRoughnessMap_
		roughnessMap = RoughnessMap.Sample(AnisoSampler, input.UV);
	#endif

	#if UseMetallicMap_
		metallicMap = MetallicMap.Sample(AnisoSampler, input.UV);
	#endif
	
	#if UseEmissiveMap_
		emissiveMap = EmissiveMap.Sample(AnisoSampler, input.UV);
	#endif

	float metallic = metallicMap.r * SpecularIntensity;
	float roughness = roughnessMap.r * Roughness;

	// TODO: currently, emissive is the actual color
	// Later make emissive as only a mask
	// The actual color is going to be passed in as cbuffer 
	float3 emissiveColor = emissiveMap.rgb * EmissiveIntensity;

    float3 diffuseAlbedo = albedoMap.xyz;
    diffuseAlbedo *= DiffuseIntensity;
    diffuseAlbedo *= (1.0f - metallic);

	float3 specularColor = lerp(0.04, albedoMap.xyz, metallic);

	PSOutput output;

	#if IsGBuffer_ // Deferred path
		output.RT0.rgb = diffuseAlbedo;
		// output.RT0.a   = shadowVisibility;
		output.RT0.a = 0;

		output.RT1.r   = roughness;
		output.RT1.g   = metallic;
		output.RT1.b   = EmissiveIntensity; // TODO: hook up emissive map later
		output.RT1.a   = 0.0f;
		// ouput.RT1.a = SSR?;

		float3 normalVS = normalize(mul(normalWS, (float3x3)View_));
		output.RT2.zw = EncodeSphereMap(normalVS);
		
		// Velocity
		float2 prevPositionSS = (input.PrevPosition.xy / input.PrevPosition.z) * float2(0.5f, -0.5f) + 0.5f;
		prevPositionSS *= RTSize;
		output.RT2.xy = input.PositionSS.xy - prevPositionSS;
		output.RT2.xy -= JitterOffset;

	#else // Forward path
		// Add in the primary directional light
		float shadowVisibility = EnableShadows ? ShadowVisibility(positionWS, input.DepthVS) : 1.0f;

		float3 lighting = 0.0f;

		if(EnableDirectLighting)
			lighting += CalcLighting(normalWS, LightDirection, LightColor, diffuseAlbedo, specularColor,
									 roughness, positionWS);

		// Add in the ambient
		if(EnableAmbientLighting)
		{
			float3 indirectDiffuse = EvalSH9Cosine(normalWS, EnvironmentSH);

			lighting += indirectDiffuse * diffuseAlbedo;

			//float3 parallaxCorrection(float3 PositionWS, float3 newProbePositionWS, float3 NormalWS, float3 boxMax, float3 boxMin)
			float3 reflectWS = parallaxCorrection(positionWS, ProbePositionWS[0], normalize(normalWS), ProbePositionWS[0] + BoxSize[0], ProbePositionWS[0] - BoxSize[0]);
			//float3 reflectWS = reflect(-viewWS, normalWS);
			float3 vtxReflectWS = reflect(-viewWS, vtxNormal);

			uint width, height, numMips;
			SpecularCubemap.GetDimensions(0, width, height, numMips);

			const float SqrtRoughness = sqrt(roughness);
			// Compute the mip level, assuming the top level is a roughness of 0.01
			float mipLevel = saturate(SqrtRoughness - 0.01f) * (numMips - 1.0f);

			float gradientMipLevel = SpecularCubemap.CalculateLevelOfDetail(LinearSampler, vtxReflectWS);
			if(UseGradientMipLevel)
				mipLevel = max(mipLevel, gradientMipLevel);

			// Compute fresnel
			float viewAngle = saturate(dot(viewWS, normalWS));
			float2 AB = SpecularCubemapLookup.SampleLevel(LinearSampler, float2(viewAngle, SqrtRoughness), 0.0f);
			float fresnel = metallic * AB.x + AB.y;
			fresnel *= saturate(metallic * 100.0f);

			float weight1 = probeWeightCalculate(ProbePositionWS[0], ObjPositionWS, BoxSize[0]);
			float weight2 = probeWeightCalculate(ProbePositionWS[1], ObjPositionWS, BoxSize[1]);

			//Test
			//weight1 = weight2 = 0;

			if (weight1 == 0 && weight2 == 0)
				lighting += SpecularCubemap.SampleLevel(LinearSampler, reflectWS, mipLevel) * fresnel;
			else
			{
				if (weight1 == 0) lighting += SpecularCubemapArray2.SampleLevel(LinearSampler, reflectWS, mipLevel) * weight2;
				else if (weight2 == 0) lighting += SpecularCubemapArray1.SampleLevel(LinearSampler, reflectWS, mipLevel) * weight1;
				else
					lighting += SpecularCubemapArray1.SampleLevel(LinearSampler, reflectWS, mipLevel) +
						SpecularCubemapArray2.SampleLevel(LinearSampler, reflectWS, mipLevel);
			}
		}
	
		// Emissive term
		lighting += emissiveColor;
		lighting *= shadowVisibility;

		output.Color = float4(lighting, 1.0f);
		output.Color.xyz *= exp2(ExposureScale);

		#if !CreateCubemap_
			float2 prevPositionSS = (input.PrevPosition.xy / input.PrevPosition.z) * float2(0.5f, -0.5f) + 0.5f;
			prevPositionSS *= RTSize;
			output.Velocity = input.PositionSS.xy - prevPositionSS;
			output.Velocity -= JitterOffset;
		#endif

	#endif

    return output;
}