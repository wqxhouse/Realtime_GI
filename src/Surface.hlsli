#ifndef SURFACE_HLSL
#define SURFACE_HLSL

struct Surface
{
	float3 posWS;
	float3 normalWS;
	float3 normalVS;
	float3 diffuse;
	float roughness;
	float metallic;
	float emissive;
	float depthVS;
};

// From Intel Tiled Deferred Shading Sample
float2 EncodeSphereMap(float3 n)
{
	float oneMinusZ = 1.0f - n.z;
	float p = sqrt(n.x * n.x + n.y * n.y + oneMinusZ * oneMinusZ);
	return n.xy / p * 0.5f + 0.5f;
}

float3 DecodeSphereMap(float2 e)
{
	float2 tmp = e - e * e;
	float f = tmp.x + tmp.y;
	float m = sqrt(4.0f * f - 1.0f);

	float3 n;
	n.xy = m * (e * 4.0f - 2.0f);
	n.z = 3.0f - 8.0f * f;
	return n;
}

float3 GetPositionWSFromNdcZ(in float ndcZ, in float3 viewRay, in float3 camPos, in float3 camZAxisWS, in float projA, in float projB, out float depthVS)
{
	viewRay = normalize(viewRay);
	float linearZ = projB / (ndcZ - projA);
	float viewRayProjCamZ = dot(camZAxisWS, viewRay);
	float3 posWS = camPos + viewRay * (linearZ / viewRayProjCamZ);
	depthVS = linearZ;
	return posWS;
}

Surface GetSurfaceFromGBuffer(in float4 RT0, in float4 RT1, in float4 RT2, 
				in float ndcZ, in float3 viewRay, in float4x4 ViewInv, 
				in float3 camPos, in float3 camZAxisWS, in float projA, in float projB)
{
	Surface surface;
	surface.diffuse = RT0.rgb;
	surface.roughness = RT1.r;
	surface.metallic = RT1.g;
	surface.emissive = RT1.b;
	surface.normalVS = DecodeSphereMap(RT2.zw);
	surface.normalWS = mul(surface.normalVS, (float3x3)ViewInv);
	surface.posWS = GetPositionWSFromNdcZ(ndcZ, viewRay, camPos, camZAxisWS, projA, projB, surface.depthVS);

	return surface;
}

#endif