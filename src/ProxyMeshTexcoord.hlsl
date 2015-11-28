#include "SharedConstants.h"

cbuffer VSConstants : register(b0)
{
    float4x4 WorldViewProjection;
}

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
	float3 NormalOS				: NORMAL;			// TODO: optimize this out later
	float2 UV					: TEXCOORD;
};


struct VSOutput
{
    float4 PositionCS 		    : SV_Position;
	float2 UV					: UV;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;
	float2 UV					: UV;
};


//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input)
{
	VSOutput output;
    output.PositionCS = mul(float4(input.PositionOS, 1.0f), WorldViewProjection);
	output.UV = input.UV;
    return output;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target
{
	return float4(input.UV, 0.0, 1.0);
	//return float4(1.0, 1.0, 1.0, 1.0);
}

