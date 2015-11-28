#include "SHProbeLight.hlsli"

cbuffer IndirectDiffuseConstants : register(b0)
{
	float4x4 ModelToWorld;

	float3 ClusterScale;
	float3 ClusterBias;
}

struct ClusterData
{
	uint offset;
	uint counts;
};

StructuredBuffer<SHProbeLight> SHProbeLights: register(t0);
StructuredBuffer<uint> LightIndices : register(t1);
Texture3D<ClusterData> Clusters : register(t2);

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
	float4 lighting = float4(0, 0, 0, 0);

	float3 normalWS = normalize(input.NormalWS);

	// Load cluster data	
	uint4 cluster_coord = uint4(input.PosWS * ClusterScale + ClusterBias, 0);
	ClusterData data = Clusters.Load(cluster_coord);

	uint offset = data.offset;
	uint counts = data.counts;
	uint pointLightCount = counts & 0xFFFF;
	uint shProbeLightCount = counts >> 16;

	// SH light
	uint shProbeLightOffset = offset + pointLightCount;
	for (uint i = 0; i < shProbeLightCount; i++)
	{
		uint shLightIndex = LightIndices[shProbeLightOffset + i];
		SHProbeLight sl = SHProbeLights[shLightIndex];
		lighting += CalcSHProbeLight(input.PosWS, normalWS, sl);
	}

	return lighting;
}

