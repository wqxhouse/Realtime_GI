struct DebugGeomPerInstanceData
{
	float4x4 Transform;
	float4 Color;
};

struct VS_OUT
{
	float4 iPosH  : SV_Position;
	float4 iColor : COLOR;
};

struct PS_IN
{
	float4 iPosH  : SV_Position;
	float4 iColor : COLOR;
};

cbuffer DebugGeomConstants : register(b0)
{
	DebugGeomPerInstanceData data[819];
}

VS_OUT DebugGeom_VS( float3 pos : POSITION, uint instanceID : SV_InstanceID )
{
	VS_OUT output;
	output.iPosH = mul(float4(pos, 1.0f), data[instanceID].Transform);
	output.iColor = data[instanceID].Color;
	return output;
}

float4 DebugGeom_PS(PS_IN input) : SV_Target0
{
	return input.iColor; 
}