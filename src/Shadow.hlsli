#ifndef SHADOW_HLSLI
#define SHADOW_HLSLI

// static const uint NumCascades = 4;
#include "EVSM.hlsl"

//-------------------------------------------------------------------------------------------------
// Samples the EVSM shadow map
//-------------------------------------------------------------------------------------------------
float SampleShadowMapEVSM(in float3 shadowPos, in float3 shadowPosDX,
	in float3 shadowPosDY, uint cascadeIdx)
{
	float2 exponents = GetEVSMExponents(PositiveExponent, NegativeExponent,
		CascadeScales[cascadeIdx].xyz);
	float2 warpedDepth = WarpDepth(shadowPos.z, exponents);

	float4 occluder = ShadowMap.SampleGrad(EVSMSampler, float3(shadowPos.xy, cascadeIdx),
		shadowPosDX.xy, shadowPosDY.xy);

	// Derivative of warping at depth
	float2 depthScale = 0.0001f * exponents * warpedDepth;
	float2 minVariance = depthScale * depthScale;

	float posContrib = ChebyshevUpperBound(occluder.xz, warpedDepth.x, minVariance.x, LightBleedingReduction);
	float negContrib = ChebyshevUpperBound(occluder.yw, warpedDepth.y, minVariance.y, LightBleedingReduction);
	float shadowContrib = posContrib;
	shadowContrib = min(shadowContrib, negContrib);

	return shadowContrib;
}

//-------------------------------------------------------------------------------------------------
// Samples the appropriate shadow map cascade
//-------------------------------------------------------------------------------------------------
float SampleShadowCascade(in float3 shadowPosition, in float3 shadowPosDX,
	in float3 shadowPosDY, in uint cascadeIdx)
{
	shadowPosition += CascadeOffsets[cascadeIdx].xyz;
	shadowPosition *= CascadeScales[cascadeIdx].xyz;

	shadowPosDX *= CascadeScales[cascadeIdx].xyz;
	shadowPosDY *= CascadeScales[cascadeIdx].xyz;

	float shadow = SampleShadowMapEVSM(shadowPosition, shadowPosDX, shadowPosDY, cascadeIdx);

	return shadow;
}

//--------------------------------------------------------------------------------------
// Computes the visibility term by performing the shadow test
//--------------------------------------------------------------------------------------
float ShadowVisibility(in float3 positionWS, in float depthVS)
{
	float shadowVisibility = 1.0f;
	uint cascadeIdx = 0;

	// Project into shadow space
	float3 samplePos = positionWS;
	float3 shadowPosition = mul(float4(samplePos, 1.0f), ShadowMatrix).xyz;
	float3 shadowPosDX = ddx(shadowPosition);
	float3 shadowPosDY = ddy(shadowPosition);

	// Figure out which cascade to sample from
	[unroll]
	for (uint i = 0; i < NumCascades - 1; ++i)
	{
		[flatten]
		if (depthVS > CascadeSplits[i])
			cascadeIdx = i + 1;
	}

	shadowVisibility = SampleShadowCascade(shadowPosition, shadowPosDX, shadowPosDY,
		cascadeIdx);

	return shadowVisibility;
}

#endif