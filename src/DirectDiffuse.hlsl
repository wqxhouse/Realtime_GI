#include "SharedConstants.h"
#include "AppSettings.hlsl"
#include "LightCommon.hlsli"

cbuffer DirectDiffuseConstants : register(b0)
{
    float4x4 ModelToWorld;
	float4x4 WorldToLightProjection;
	
	float3 ClusterScale;
	float3 ClusterBias;
}

struct ClusterData
{
	uint offset;
	uint counts;
};

Texture2D DepthMap : register(t0);
StructuredBuffer<PointLight> PointLights : register(t1);
StructuredBuffer<uint> LightIndices : register(t2);
Texture3D<ClusterData> Clusters : register(t3);

SamplerComparisonState ShadowMapPCFSampler : register(s0);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
	float3 NormalOS				: NORMAL;			
	float2 UV					: TEXCOORD;
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;
	float3 PosWS				: POSITION_WS;
	float3 NormalWS				: NORMAL_WS;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;
	float3 PosWS				: POSITION_WS;
	float3 NormalWS				: NORMAL_WS;
};


//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input)
{
	VSOutput output;
	float2 uuvv = input.UV;
	uuvv.y = 1 - uuvv.y;
    output.PositionCS = float4(uuvv * 2.0 - 1.0, 0.0, 1.0); 
	output.PosWS = mul(float4(input.PositionOS, 1.0), ModelToWorld).xyz;
	output.NormalWS = mul(input.NormalOS, (float3x3)ModelToWorld);
    return output;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target
{
	float3 lighting = float3(0, 0, 0);

	float3 normalWS = normalize(input.NormalWS);

	// Do Sun lighting
	lighting += CalcDirectionalLightLambert(normalWS, LightDirection, LightColor, input.PosWS);

	// resolve sun shadow
	float4 light_Clip = mul(float4(input.PosWS, 1.0f), WorldToLightProjection);
	float3 light_NDC = light_Clip.xyz / light_Clip.w;
	float2 light_uv = light_NDC.xy * 0.5 + 0.5;
	light_NDC.z -= 0.001;

	float sunShadow = DepthMap.SampleCmpLevelZero(ShadowMapPCFSampler, light_uv, light_NDC.z);
	lighting *= sunShadow;

	// Load cluster data	
	uint4 cluster_coord = uint4(input.PosWS * ClusterScale + ClusterBias, 0);
	ClusterData data = Clusters.Load(cluster_coord);

	uint offset = data.offset;
	uint counts = data.counts;
	uint pointLightCount = counts & 0xFFFF;

	// Point light
	for (uint i = 0; i < pointLightCount; i++)
	{
		uint lightIndex = LightIndices[offset + i];
		PointLight pl = PointLights[lightIndex];
		lighting += CalcPointLightLambert(normalWS, pl, input.PosWS);
	}
	
	return float4(lighting, 1.0f);
	// return float4(sunShadow, sunShadow, sunShadow, 1.0f);
	// return float4(light_NDC.z, light_NDC.z, light_NDC.z, 1.0f);
	// return float4(light_NDC.z, light_NDC.z, light_NDC.z, 1.0f);
}

