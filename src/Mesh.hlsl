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
	float3 CubemapPositionWS;
	float4x4 ShadowMatrix;
	float4 CascadeSplits;
    float4 CascadeOffsets[NumCascades];
    float4 CascadeScales[NumCascades];
    float OffsetScale;
    float PositiveExponent;
    float NegativeExponent;
    float LightBleedingReduction;
    float4x4 Projection;
    SH9Color EnvironmentSH;
    float2 RTSize;
    float2 JitterOffset;
	float3 BoxMax;
	float3 BoxMin;
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
	#if CreateCubemap_
		float4 Color                : SV_Target0;
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


float3 parallaxCorrection(float3 PositionWS, float3 NormalWS){
	float3 DirectionWS = normalize(PositionWS - CameraPosWS);
	float3 ReflDirectionWS = reflect(DirectionWS, NormalWS);

	float3 FirstPlaneIntersect = (BoxMax - PositionWS) / ReflDirectionWS;
	float3 SecondPlaneIntersect = (BoxMin - PositionWS) / ReflDirectionWS;

	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);

	float Distance = min(min(FurthestPlane.x, FurthestPlane.y), FurthestPlane.z);

	float3 IntersectPositionWS = PositionWS + ReflDirectionWS * Distance;

	ReflDirectionWS = IntersectPositionWS - CubemapPositionWS;

	return ReflDirectionWS;
}

float radicalInverse_VdC(uint bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint NumSamples)
{
	return float2((float)i / (float)NumSamples, radicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a = Roughness * Roughness;

	float Phi = 2 * Pi * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float3 UpVector = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3 (1, 0, 0);
		float3 TangentX = normalize(cross(UpVector, N));
		float3 TangentY = cross(N, TangentX);

		return TangentX * H.x + TangentY * H.y + N * H.z;
}

//=================================================================================================
//Pre-Filter Environment Map
//=================================================================================================
float3 PrefilterEnvMap(float Roughness, float3 R){
	float3 N = R;
		float3 V = R;

		float3 PrefilteredColor = 0;

		const uint NumSamples = 1024;
	float TotalWeight = 0;

	for (uint i = 0; i < NumSamples; i++)
	{
		float2 Xi = Hammersley(i, NumSamples);
			float3 H = ImportanceSampleGGX(Xi, Roughness, N);
			float3 L = 2 * dot(V, H) * H - V;

			float NoL = saturate(dot(N, L));

		if (NoL > 0)
		{
			PrefilteredColor += SpecularCubemap.SampleLevel(LinearSampler, L, i).rgb * NoL;
			TotalWeight += NoL;
		}
	}

	return PrefilteredColor / TotalWeight;
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

		//float3 reflectWS = parallaxCorrection(positionWS, normalize(normalWS));
		float3 reflectWS = reflect(-viewWS, normalWS);
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
	
		#if ConvoluteCube_
			lighting += SpecularCubemap.SampleLevel(LinearSampler, reflectWS, mipLevel) * fresnel * PrefilterEnvMap(roughness, reflectWS);
		#else
			lighting += SpecularCubemap.SampleLevel(LinearSampler, reflectWS, mipLevel) * fresnel;
		#endif
    }

	
	// Emissive term
	lighting += emissiveColor;
	lighting *= shadowVisibility;

	PSOutput output;

	output.Color = float4(lighting, 1.0f);
	output.Color.xyz *= exp2(ExposureScale);

	#if !CreateCubemap_
		float2 prevPositionSS = (input.PrevPosition.xy / input.PrevPosition.z) * float2(0.5f, -0.5f) + 0.5f;
		prevPositionSS *= RTSize;
		output.Velocity = input.PositionSS.xy - prevPositionSS;
		output.Velocity -= JitterOffset;
	#endif

	#if ConvoluteCube_
		output.Color = float4(1, 0, 0, 1);
	#endif

    return output;
}