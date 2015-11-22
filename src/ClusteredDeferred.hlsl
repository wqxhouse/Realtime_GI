//=================================================================================================
// Includes
//=================================================================================================
#include "SharedConstants.h"
#include "AppSettings.hlsl"
#include "LightCommon.hlsli"
#include <SH.hlsl>

static const uint NumCascades = 4;

////////////////////////////////////////////////////////////
// Resources
////////////////////////////////////////////////////////////
cbuffer ClusteredDeferredConstants : register(b0)
{
	float4x4 ShadowMatrix;
	float4 CascadeSplits;
	float4 CascadeOffsets[NumCascades];
	float4 CascadeScales[NumCascades];
	float OffsetScale;
	float PositiveExponent;
	float NegativeExponent;
	float LightBleedingReduction;
	float4x4 ProjectionToWorld;
	float4x4 ViewToWorld;
	SH9Color EnvironmentSH;

	float3 CameraPosWS;
	float NearPlane;
	float3 CameraZAxisWS;
	float FarPlane;
	float ProjTermA;
	float ProjTermB;
}

Texture2D RT0 : register(t0);
Texture2D RT1 : register(t1);
Texture2D RT2 : register(t2);
Texture2D DepthMap : register(t3);
Texture2DArray ShadowMap : register(t4);
StructuredBuffer<PointLight> PointLights : register(t5);

TextureCube<float3> SpecularCubemap : register(t6);
Texture2D<float2> SpecularCubemapLookup : register(t7);

SamplerState EVSMSampler : register(s0);
SamplerState LinearSampler : register(s1);

#include "Shadow.hlsli"

/////////////////////////////////////////////////////////////
// Input / Output
/////////////////////////////////////////////////////////////
struct VSInput
{
    float4 PositionCS 		    : POSITION;
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;
	float3 ViewRay				: VIEWRAY;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;
	float3 ViewRay				: VIEWRAY;
};

/////////////////////////////////////////////////////////////
// Shader
/////////////////////////////////////////////////////////////
VSOutput ClusteredDeferredVS(in VSInput input)
{
	VSOutput output;
    output.PositionCS = input.PositionCS;

	float4 quadWPos = mul(input.PositionCS, ProjectionToWorld);
	quadWPos /= quadWPos.w;
	output.ViewRay = quadWPos.xyz - CameraPosWS;

	return output;
}

float4 ClusteredDeferredPS(in PSInput input) : SV_Target0
{
	uint4 fragCoord = uint4(input.PositionSS);

	float4 rt0 = RT0.Load(uint3(fragCoord.xy, 0));
	float4 rt1 = RT1.Load(uint3(fragCoord.xy, 0));
	float4 rt2 = RT2.Load(uint3(fragCoord.xy, 0));
	float ndcZ = DepthMap.Load(uint3(fragCoord.xy, 0)).x;
	
	Surface surface = GetSurfaceFromGBuffer(
		rt0, rt1, rt2, ndcZ, input.ViewRay, ViewToWorld, 
		CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB);

	float3 lighting = float3(0.0f, 0.0f, 0.0f);

	if(EnableDirectLighting)
	{
		// Do Sun lighting
		lighting += CalcDirectionalLight(surface.normalWS, LightDirection, LightColor, 
			surface.diffuse, surface.metallic, surface.roughness, surface.posWS, CameraPosWS);

		float sunShadow = EnableShadows ? ShadowVisibility(surface.posWS, surface.depthVS) : 1.0f;
		lighting *= sunShadow;

		// Point light
		PointLight pl = PointLights[0];
		lighting += CalcPointLight(surface, pl, CameraPosWS);
	}

	if(EnableAmbientLighting)
	{
		float3 viewWS = normalize(CameraPosWS - surface.posWS); // TODO: this can be reused in several places
		
		float3 indirectDiffuse = EvalSH9Cosine(surface.normalWS, EnvironmentSH);

		lighting += indirectDiffuse * surface.diffuse;

		float3 reflectWS = reflect(-viewWS, surface.normalWS);

		uint width, height, numMips;
		SpecularCubemap.GetDimensions(0, width, height, numMips);

		const float SqrtRoughness = sqrt(surface.roughness);

		// Compute the mip level, assuming the top level is a roughness of 0.01
		float mipLevel = saturate(SqrtRoughness - 0.01f) * (numMips - 1.0f);

		//float gradientMipLevel = SpecularCubemap.CalculateLevelOfDetail(LinearSampler, vtxReflectWS);
		//if(UseGradientMipLevel)
		//	mipLevel = max(mipLevel, gradientMipLevel);

		// Compute fresnel
		float viewAngle = saturate(dot(viewWS, surface.normalWS));
		float2 AB = SpecularCubemapLookup.SampleLevel(LinearSampler, float2(viewAngle, SqrtRoughness), 0.0f);
		float fresnel = surface.metallic * AB.x + AB.y;
		fresnel *= saturate(surface.metallic * 100.0f);

		lighting += SpecularCubemap.SampleLevel(LinearSampler, reflectWS, mipLevel) * fresnel;
	}


	return float4(lighting, 1.0f);
	// return float4(surface.depthVS, surface.depthVS, surface.depthVS, 1.0f);
}