cbuffer VSConstants : register(b0)
{
    float4x4 WorldViewProjection;
}

struct VSInput
{
    float4 PositionOS 		: POSITION;
};

struct VSOutput
{
    float4 PositionCS 		: SV_Position;
};

VSOutput VS(in VSInput input)
{
    VSOutput output;

    // Calc the clip-space position
    output.PositionCS = mul(input.PositionOS, WorldViewProjection);

    return output;
}
