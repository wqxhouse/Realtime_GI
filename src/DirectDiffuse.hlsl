#include "SharedConstants.h"

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
	//float3 PosWS				: POSITION_WS;
	//float3 NormalWS				: NORMAL_WS;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;
	//float3 PosWS				: POSITION_WS;
	//float3 NormalWS				: NORMAL_WS;
};


//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input)
{
	VSOutput output;
    output.PositionCS = float4(input.UV * 2.0 - 1.0, 0.0, 1.0); 
    return output;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target
{
	return float4(0.7, 0.3, 0.0, 1.0);
}

