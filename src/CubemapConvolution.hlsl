float4 main(float4 pos : POSITION) : SV_POSITION
{
	return pos;
}

float4 psmain(in float4 input : POSITION) : SV_Target
{
	return input;
}