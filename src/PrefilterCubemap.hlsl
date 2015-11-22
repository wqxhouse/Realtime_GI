#include <SH.hlsl>
#include "AppSettings.hlsl"

cbuffer VSConstant : register(b0)
{
	float4x4 World;
	float4x4 View;
	float4x4 WorldViewProjection;
}

cbuffer PSConstant : register(b0)
{
	float4 CameraPos;
	uint MipLevel;
}

TextureCube<float3> Cubemap : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput
{
	float3 PositionOS	:	POSITION;
	float3 NormalOS		:	NORMAL;
};

struct VSOutput
{
	float3 Normal		:	NORMALWS;
	float3 PositionWS	:	POSITIONWS;
	float4 Position		:	SV_POSITION;
};

struct PSInput
{
	float3 Normal	:	NORMALWS;
	float3 PositionWS	:	POSITIONWS;
};

struct PSOutput
{
	float4 Color	:	SV_TARGET0;
};

VSOutput VS(in VSInput In)
{
	VSOutput Out;

	// Calc the world-space position
	Out.Normal = In.NormalOS;// normalize(mul(In.NormalOS, (float3x3)World));
	Out.PositionWS = mul(float4(In.PositionOS, 1.0f), World).xyz;

	Out.Position = /*float4(In.PositionOS, 1.0f);*/mul(float4(In.PositionOS, 1.0f), WorldViewProjection);

	return Out;
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

	float3 PrefilteredColor = float3(0, 0, 0);

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
			PrefilteredColor += Cubemap.SampleLevel(LinearSampler, L, i).rgb * NoL;
			TotalWeight += NoL;
		}
	}

	return PrefilteredColor / TotalWeight;
}


PSOutput PS(in PSInput input)
{
	PSOutput output;
	float3 lighting = 0.0f;
	float3 Normal = input.Normal;

	float4 roughnessMap = 1.0f;
	//float roughness = roughnessMap.r * Roughness;
	float3 viewWS = normalize(CameraPos.xyz - input.PositionWS);
	float3 reflectWS = reflect(-viewWS, Normal);

	uint height, width, nummips;
	Cubemap.GetDimensions(0, width, height, nummips);

	//for (uint miplevel = 0; miplevel < nummips; ++miplevel)
	//{
	//	//calculate roughness of every mip level
	//	float roughness = pow(miplevel / (nummips - 1.0f) + 0.01f, 2);
	//	lighting = PrefilterEnvMap(roughness, input.Normal);
	//}
	float roughness = pow(MipLevel / (nummips - 1.0f) + 0.01f, 2);

	#if FilterFaceIndex_ == 0
		lighting = Cubemap.SampleLevel(LinearSampler, float3(Normal[2], Normal[1], -Normal[0]), MipLevel);
	#elif FilterFaceIndex_ == 1
		lighting = Cubemap.SampleLevel(LinearSampler, float3(-Normal[2], Normal[1], Normal[0]), MipLevel);
	#elif FilterFaceIndex_ == 2
		lighting = Cubemap.SampleLevel(LinearSampler, float3(Normal[0], Normal[2], -Normal[1]), MipLevel);
	#elif FilterFaceIndex_ == 3
		lighting = Cubemap.SampleLevel(LinearSampler, float3(Normal[0], -Normal[2], Normal[1]), MipLevel);
	#elif FilterFaceIndex_ == 4
		lighting = Cubemap.SampleLevel(LinearSampler, Normal, MipLevel);
	#else
		lighting = Cubemap.SampleLevel(LinearSampler, float3(-Normal[0], Normal[1], -Normal[2]), MipLevel);
	#endif

		lighting *= PrefilterEnvMap(roughness, viewWS);
	output.Color = float4(lighting, 1.0f);
	//output.Color = float4(input.Normal, 1);


	return output;
}