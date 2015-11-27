Texture2D DirectDiffuseMap : register(t0);
Texture2D ProxyMeshTexcoordAtlas :register(t1);
Texture2D AlbedoMapAtlas : register(t2);
Texture2D IndirectBouncesMap : register(t3);

SamplerState LinearSampler : register(s0);

struct VSOutput
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD;
};

VSOutput VS(in uint VertexID : SV_VertexID)
{
    VSOutput output;

    if(VertexID == 0)
    {
        output.Position = float4(-1.0f, 1.0f, 1.0f, 1.0f);
        output.TexCoord = float2(0.0f, 0.0f);
    }
    else if(VertexID == 1)
    {
        output.Position = float4(3.0f, 1.0f, 1.0f, 1.0f);
        output.TexCoord = float2(2.0f, 0.0f);
    }
    else
    {
        output.Position = float4(-1.0f, -3.0f, 1.0f, 1.0f);
        output.TexCoord = float2(0.0f, 2.0f);
    }

    return output;
}

float3 sampleScene(float2 offset, float2 posSS, float3 albedoDiffuse, uint2 texcoordTexScale)
{
	uint2 samplePos = posSS * texcoordTexScale + offset;
	float3 cubeToLightmap = ProxyMeshTexcoordAtlas.Load(uint3(samplePos, 0)).xyz;

	if(cubeToLightmap.z > 0.0)
	{
		return float3(0.1, 0.3, 0.7);
	}
	else 
	{
		float3 directDiffuse = DirectDiffuseMap.Sample(LinearSampler, cubeToLightmap.xy).xyz;
		float4 indirectDiffuse = IndirectBouncesMap.Sample(LinearSampler, cubeToLightmap.xy);
		indirectDiffuse /= indirectDiffuse.a;
		return albedoDiffuse * (directDiffuse + indirectDiffuse.rgb);
	}
}

float4 PS(in VSOutput input) : SV_Target
{
	// current rt is the same size as albedo texture
	uint2 rtSize;
	AlbedoMapAtlas.GetDimensions(rtSize.x, rtSize.y);

	uint2 texcoordSize; 
	ProxyMeshTexcoordAtlas.GetDimensions(texcoordSize.x, texcoordSize.y);

	uint2 texcoordScale = texcoordSize * (1.0 / rtSize);

	float3 albedo = AlbedoMapAtlas.Load(uint3(input.Position.xy, 0)).rgb;

	// do a 2x2 box filter over the samples
	float3 s1 = sampleScene(float2(-0.5, -0.5), input.Position.xy, albedo, texcoordScale);
	float3 s2 = sampleScene(float2(0.5, -0.5), input.Position.xy, albedo, texcoordScale);
	float3 s3 = sampleScene(float2(-0.5, 0.5), input.Position.xy, albedo, texcoordScale);
	float3 s4 = sampleScene(float2(0.5, 0.5), input.Position.xy, albedo, texcoordScale);

	float3 avg = (s1 + s2 + s3 + s4) / 4.0;
	return float4(max(avg, 0.0), 1.0);
	//return float4(albedo, 1.0);
	/*float3 cubeToLightmap = ProxyMeshTexcoordAtlas.Load(uint3(input.Position.xy * 2, 0)).xyz;
	return float4(DirectDiffuseMap.Sample(LinearSampler, cubeToLightmap.xy).xyz * albedo, 1.0);*/
}



