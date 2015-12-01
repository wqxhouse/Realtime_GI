#include "SharedConstants.h"
#include "AppSettings.hlsl"
#include "LightCommon.hlsli"

#define MAX_STEPS 300
#define MAX_INTERSECT_DIST 0.1
//#define STRIDE 1.1
//#define ZTHICKNESS 0.15
////////////////////////////////////////////////////////////
// Resources
////////////////////////////////////////////////////////////
cbuffer RayTracingConstants : register(b0)
{
	float4x4 ProjectionToWorld;
	float4x4 ViewToWorld;
	float4x4 WorldToView;
	float4x4 ViewToProjection;

	float3 CameraPosWS;
	float NearPlane;
	float3 CameraZAxisWS;
	float FarPlane;

	float3 ClusterScale;
	float ProjTermA;
	float3 ClusterBias;
	float ProjTermB;

	float4 invNDCToWorldZ;
}

Texture2D ClusteredColorMap : register(t0);
Texture2D RMEMap : register(t1);
Texture2D NormalMap : register(t2);
Texture2D DepthMap : register(t3);



/////////////////////////////////////////////////////////////
// Input / Output
/////////////////////////////////////////////////////////////
struct VSInput
{
	float4 PositionCS 		    : POSITION;
};

struct VSOutput
{
	float4 PositionCS 		    : SV_Position;
	float3 ViewRay				: VIEWRAY;
};

struct PSInput
{
	float4 PositionSS 		    : SV_Position;
	float3 ViewRay				: VIEWRAY;
};

VSOutput VS(in VSInput input)
{
	VSOutput output;
	output.PositionCS = input.PositionCS;

	float4 quadWPos = mul(input.PositionCS, ProjectionToWorld);
		quadWPos /= quadWPos.w;
	output.ViewRay = quadWPos.xyz - CameraPosWS;

	return output;
}

float2 NormalizedDeviceCoordToScreenCoord(float2 ndc, float2 dim)
{
	float2 screenCoord;
	screenCoord.x = dim.x * ((ndc.x * 0.5) + 0.5);
	screenCoord.y = dim.y * (1.0f - ((ndc.y * 0.5) + 0.5));
	return screenCoord;
}
/*
//////////////////////////
float distanceSquared(float2 a, float2 b)
{
	a -= b;
	return dot(a, a);
}

void swap(inout float a, inout float b)
{
	float t = a;
	a = b;
	b = t;
}

bool traceScreenSpaceRay(
	// Camera-space ray origin, which must be within the view volume
	float3 csOrig,
	// Unit length camera-space ray direction
	float3 csDir,
	// Number between 0 and 1 for how far to bump the ray in stride units
	// to conceal banding artifacts. Not needed if stride == 1.
	float jitter, float2 dim, PSInput input,
	// Pixel coordinates of the first intersection with the scene
	out float2 hitPixel,
	// Camera space location of the ray hit
	out float3 hitPoint)
{
	// Clip to the near plane
	float rayLength = ((csOrig.z + csDir.z * FarPlane) < NearPlane) ?
		(NearPlane - csOrig.z) / csDir.z : FarPlane;
	float3 csEndPoint = csOrig + csDir * rayLength;

	//NDCS  ray start & end point
	float4 H0 = mul(float4(csOrig, 1.0), ViewToProjection);
	float4 H1 = mul(float4(csEndPoint, 1.0), ViewToProjection);

	//projection factor
	float k0 = 1.0f / H0.w;
	float k1 = 1.0f / H1.w;

	// The interpolated homogeneous version of the camera-space points
	float3 Q0 = csOrig * k0;
	float3 Q1 = csEndPoint * k1;

	// Screen-space endpoints
	float2 P0 = H0.xy * k0;
	float2 P1 = H1.xy * k1;

	P0.xy = NormalizedDeviceCoordToScreenCoord(P0.xy, dim);
	P1.xy = NormalizedDeviceCoordToScreenCoord(P1.xy, dim);

	// If the line is degenerate, make it cover at least one pixel
	// to avoid handling zero-pixel extent as a special case later
	P1 += (distanceSquared(P0, P1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
	float2 delta = P1 - P0;

	// Permute so that the primary iteration is in x to collapse
	// all quadrant-specific DDA cases later
	bool permute = false;
	if(abs(delta.x) < abs(delta.y))
	{
		// This is a more-vertical line
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}
	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

	// Track the derivatives of Q and k
	float3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	float2 dP = float2(stepDir, delta.y * invdx);

	float stride = STRIDE;
	dP *= stride;
	dQ *= stride;
	dk *= stride;
 
	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

	// Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
	float4 PQk = float4(P0, Q0.z, k0);
	float4 dPQk = float4(dP, dQ.z, dk);
	float3 Q = Q0;
	// Adjust end condition for iteration direction
	float end = P1.x * stepDir;

	float stepCount = 0.0f;
	float prevZMaxEstimate = csOrig.z;
	float rayZMin = prevZMaxEstimate;
	float rayZMax = prevZMaxEstimate;
	float sceneZMax = rayZMax + 100.0f;

	for (;
		((PQk.x * stepDir) <= end) && (stepCount < MAX_STEPS) &&
		!((rayZMax >= sceneZMax + ZTHICKNESS) && (rayZMin <= sceneZMax)) &&
		(sceneZMax != 0.0f);
	++stepCount)
	{
		rayZMin = prevZMaxEstimate;
		rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
		prevZMaxEstimate = rayZMax;
		if (rayZMin > rayZMax)
		{
			swap(rayZMin, rayZMax);
		}

		hitPixel = permute ? PQk.yx : PQk.xy;

		float ndcZtest = DepthMap.Load(uint3(hitPixel.xy, 0)).x;
		float4 rt0 = float4(0, 0, 0, 1);
		float depthVS;
		float3 posWScoord = GetPositionWSFromNdcZ(ndcZtest, input.ViewRay, CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB, depthVS);
		
		
		//GetSurfaceFromGBuffer(
		//	rt0, rt0, rt0, ndcZtest, input.ViewRay, ViewToWorld,
		//	CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB).posWS;
		float storedDepth = depthVS;//mul(float4(posWScoord, 1.0), WorldToView).z;
		
		sceneZMax = storedDepth;

		PQk += dPQk;
	}

	// Advance Q based on the number of steps
	Q.xy += dQ.xy * stepCount;
	hitPoint = Q * (1.0f / PQk.w);
	return (rayZMax >= sceneZMax + ZTHICKNESS) && (rayZMin <= sceneZMax);
}

///////////////////////////////
float4 PS(in PSInput input) : SV_Target0
{
	uint4 fragCoord = uint4(input.PositionSS);

	float4 rt0 = float4(0, 0, 0, 1);
	float4 rt1 = RMEMap.Load(uint3(fragCoord.xy, 0));
	float4 rt2 = NormalMap.Load(uint3(fragCoord.xy, 0));
	float ndcZ = DepthMap.Load(uint3(fragCoord.xy, 0)).x;

	Surface surface = GetSurfaceFromGBuffer(
		rt0, rt1, rt2, ndcZ, input.ViewRay, ViewToWorld,
		CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB);

	//get screen dimension
	uint width;
	uint height;
	DepthMap.GetDimensions(width, height);
	float2 dim = float2(width, height);

	//get CS reflection ray
	float3 n = surface.normalWS;
	float3 d = normalize(input.ViewRay);
	float3 refl = normalize(reflect(d, n));
	float3 rayDirectionVS = mul(float4(refl, 0), WorldToView).xyz;

	//cone
	float3 rayOriginVS = mul(float4(surface.posWS, 1.0), WorldToView).xyz;

	float3 viewRayVS = mul(float4(d, 0), WorldToView).xyz;
	float rDotv = dot(normalize(rayDirectionVS), normalize(viewRayVS));

	// out parameters
	float2 hitPixel = float2(0.0f, 0.0f);
	float3 hitPoint = float3(0.0f, 0.0f, 0.0f);

	float jitter = STRIDE > 1.0f ? float(int(fragCoord.x + fragCoord.y) & 1) * 0.5f : 0.0f;
	
	bool intersection = traceScreenSpaceRay(rayOriginVS, rayDirectionVS, jitter, dim, input, hitPixel, hitPoint);
	
	float depthofhit = DepthMap.Load(uint3(hitPixel.xy, 0)).x;

	if (hitPixel.x > dim.x || hitPixel.x < 0.0f || hitPixel.y > dim.y || hitPixel.y < 0.0f)
	{
		intersection = false;
	}

	//return float4(hitPixel.xy, depthofhit, rDotv) * (intersection ? 1.0f : 0.0f);
	//return float4(intersection ? 1.0f : 0.0f, 0, 0, 1.0f);
	if (intersection)
	{
		return  float4(hitPixel.xy, depthofhit, rDotv);
	}
	else
	{
		return  float4(intersection ? 1.0f : 0.0f, 0, 0, 1.0f);//ClusteredColorMap.Load(uint3(fragCoord.xy, 0));
	}
}
*/

float4 PS(in PSInput input) : SV_Target0
{
	uint4 fragCoord = uint4(input.PositionSS);

	float4 rt0 = float4(0, 0, 0, 1);
	float4 rt1 = RMEMap.Load(uint3(fragCoord.xy, 0));
	float4 rt2 = NormalMap.Load(uint3(fragCoord.xy, 0));
	float ndcZ = DepthMap.Load(uint3(fragCoord.xy, 0)).x;
	float4 oriColor = ClusteredColorMap.Load(uint3(fragCoord.xy, 0));

	float4x4 dummy;
	Surface surface = GetSurfaceFromGBuffer(
		rt0, rt1, rt2, ndcZ, input.ViewRay, ViewToWorld,
		CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB, dummy, invNDCToWorldZ);

	//get screen dimension
	uint width;
	uint height;
	DepthMap.GetDimensions(width, height);
	float2 dim = float2(width, height);

	//test coord
	float2 coord;

	//step counter;
	float t = 1;

	//get CS reflection ray
	float3 n = surface.normalWS;
	float3 d = normalize(input.ViewRay);//normalize(posWS.xyz - cs_camPos.xyz);//viewRay
	float3 refl = normalize(reflect(d, n));
	float3 ScreenRefl = mul(float4(refl, 0), WorldToView).xyz;

	//cone
	float3 viewRayVS = mul(float4(d, 0), WorldToView).xyz;
	float rDotv = dot(normalize(ScreenRefl), normalize(viewRayVS));
	bool intersection = false;
	float depthofhit;
	float2 hitPixel = float2(0.0f, 0.0f);
	
	//get reflactivity  1-nonreflection
	float reflectivity = surface.roughness;

	float4 reflColor = float4(0, 0, 0, 1);

	//CS ray start & end point
	float3 v0 = mul(float4(surface.posWS, 1.0), WorldToView).xyz;

	float3 v1 = v0 + ScreenRefl * FarPlane;

	//NDCS  ray start & end point
	float4 p0 = mul(float4(v0, 1.0), ViewToProjection);
	float4 p1 = mul(float4(v1, 1.0), ViewToProjection);

	//projection factor
	float k0 = 1.0f / p0.w;
	float k1 = 1.0f / p1.w;

	p0 *= k0;
	p1 *= k1;

	//get screen space coord
	p0.xy = NormalizedDeviceCoordToScreenCoord(p0.xy, dim);
	p1.xy = NormalizedDeviceCoordToScreenCoord(p1.xy, dim);

	v0 *= k0;
	v1 *= k1;

	float divisions = length(p1 - p0);
	float3 dV = (v1 - v0) / divisions;
	float dK = (k1 - k0) / divisions;
	float2 traceDir = (p1.xy - p0.xy) / divisions;

	float maxSteps = min(MAX_STEPS, divisions);

	//if (reflectivity < 1.0f)
	//{
		while (t < maxSteps)
		{
			coord = fragCoord.xy + traceDir * t;

			if (coord.x >= dim.x || coord.y >= dim.y || coord.x < 0 || coord.y < 0)
			{
				intersection = false;
				break;
			}

			float curDepth = (v0 + dV * t).z;
			curDepth /= k0 + dK * t;

			float ndcZtest = DepthMap.Load(uint3(coord.xy, 0)).x;

			float depthVS;
			float3 posWScoord = GetPositionWSFromNdcZ(ndcZtest, input.ViewRay, CameraPosWS, CameraZAxisWS, ProjTermA, ProjTermB, dummy, invNDCToWorldZ, depthVS);
			float storedDepth = depthVS;//mul(float4(posWScoord, 1.0), WorldToView).z;
			

			if (curDepth > storedDepth && curDepth - storedDepth < MAX_INTERSECT_DIST)//&& curDepth - storedDepth < MAX_INTERSECT_DIST && t > 15
			{
				intersection = true;
				hitPixel = coord.xy;
				depthofhit = ndcZtest;
				
				reflColor = float4(ClusteredColorMap.Load(uint3(coord.xy, 0)).xyz, 1.0);
				
				break;
			}
			t++;
		}
	//}
	
	//return float4(surface.posWS.y,0,0, 1.0f);
	//return reflColor;
	[flatten]
	if (intersection)
	{
		return  oriColor * (reflectivity)+reflColor * (1 - reflectivity) ;
	}
	else
	{
		return  oriColor;
	}
	
	//return  ClusteredColorMap.Load(uint3(fragCoord.xy, 0)) * (reflectivity) + reflColor * (1-reflectivity);
}

