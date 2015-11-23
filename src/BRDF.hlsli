#ifndef BRDF_HLSLI
#define BRDF_HLSLI

#include <Constants.hlsl>

// ================================================================================================
// Calculates the Fresnel factor using Schlick's approximation
// ================================================================================================
float3 Fresnel(in float3 specAlbedo, in float3 h, in float3 l)
{
	float3 fresnel = specAlbedo + (1.0f - specAlbedo) * pow((1.0f - saturate(dot(l, h))), 5.0f);

	// Fade out spec entirely when lower than 0.1% albedo
	fresnel *= saturate(dot(specAlbedo, 333.0f));

	return fresnel;
}

// ===============================================================================================
// Helper for computing the GGX visibility term
// ===============================================================================================
float GGX_V1(in float m2, in float nDotX)
{
	return 1.0f / (nDotX + sqrt(m2 + (1 - m2) * nDotX * nDotX));
}

// ===============================================================================================
// Computes the specular term using a GGX microfacet distribution, with a matching
// geometry factor and visibility term. Based on "Microfacet Models for Refraction Through
// Rough Surfaces" [Walter 07]. m is roughness, n is the surface normal, h is the half vector,
// l is the direction to the light source, and specAlbedo is the RGB specular albedo
// ===============================================================================================
float GGX_Specular(in float m, in float3 n, in float3 h, in float3 v, in float3 l)
{
	float nDotH = saturate(dot(n, h));
	float nDotL = saturate(dot(n, l));
	float nDotV = saturate(dot(n, v));

	float nDotH2 = nDotH * nDotH;
	float m2 = m * m;

	// Calculate the distribution term
	float d = m2 / (Pi * pow(nDotH * nDotH * (m2 - 1) + 1, 2.0f));

	// Calculate the matching visibility term
	float v1i = GGX_V1(m2, nDotL);
	float v1o = GGX_V1(m2, nDotV);
	float vis = v1i * v1o;

	return d * vis;
}

#endif