#include <SH.hlsl>

TextureCube<float3> Cubemap : register(t0);
SamplerState LinearSampler : register(s0);

struct VSInput
{
	float3 PositionOS	:	POSITION;
	float3 Normal		:	NORMAL;
};

struct VSOutput
{
	float4 Position	:	SV_POSITION;
	float3 Normal	:	NORMAL;
};

struct PSInput
{
	float3 Normal	:	NORMAL;
};

struct PSOutput
{
	float4 Color	:	SV_Target0;
};

VSOutput VS(in VSInput In)
{
	VSOutput Out;
	Out.Normal = In.Normal;
	Out.Position = float4 ( In.PositionOS, 1.0f);
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
			PrefilteredColor += Cubemap.SampleLevel(LinearSampler, L, 0).rgb * NoL;
			TotalWeight += NoL;
		}
	}

	return PrefilteredColor / TotalWeight;
}


PSOutput PS(in PSInput input)
{
	PSOutput output;
	float4 color;

	uint height, width, numMips;
	Cubemap.GetDimensions(0, width, height, numMips);

	for (uint mipLevel = 0; mipLevel < numMips; ++mipLevel)
	{
		//Calcualte Roughness of every mip level
		float roughness = pow(mipLevel / (numMips - 1.0f) + 0.01f, 2);
		color = float4(PrefilterEnvMap(roughness, input.Normal), 1.0f);
	}

	output.Color = color;

	return output;
}