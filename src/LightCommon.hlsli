#ifndef LIGHTCOMMON_HLSLI
#define LIGHTCOMMON_HLSLI

#include "Surface.hlsli"
#include "BRDF.hlsli"

struct PointLight
{
	float3 posWS;
	float radius;
	float3 color;
};

float PointLightAttenuation(float radius, float dist)
{
	// Soft transition
	float attenuation = pow(1.0 + (dist / radius), -2.0) * 1.2;

	float cutoff = radius * 0.7;
	attenuation *= 1.0 - smoothstep(0.0, 1.0, ((dist / cutoff) - 1.0) * 4.0);
	attenuation = max(0.0, attenuation);
	return attenuation;
}

float3 CalcDirectionalLight(in float3 normal, in float3 lightDir, in float3 lightColor,
	in float3 diffuseAlbedo, in float3 specularAlbedo, in float roughness,
	in float3 positionWS, in float3 CameraPosWS)
{
	float3 lighting = diffuseAlbedo * (1.0f / 3.14159f);

	float3 view = normalize(CameraPosWS - positionWS);
	const float nDotL = saturate(dot(normal, lightDir));

	if (nDotL > 0.0f)
	{
		const float nDotV = saturate(dot(normal, view));
		float3 h = normalize(view + lightDir);

		float specular = GGX_Specular(roughness, normal, h, view, lightDir);
		float3 fresnel = Fresnel(specularAlbedo, h, lightDir);

		lighting += specular * fresnel;
	}

	return lighting * nDotL * lightColor;
}

float3 CalcPointLight(in Surface surface, in PointLight pointLight, in float3 CameraPosWS)
{
	float3 lighting = surface.diffuse * (1.0f / 3.14159f);

	// TODO: factor out common terms between lights
	float3 view = normalize(CameraPosWS - surface.posWS);
	float3 lightDir = pointLight.posWS - surface.posWS;
	float dist = length(lightDir);
	lightDir = normalize(lightDir);

	const float att = PointLightAttenuation(pointLight.radius, dist);
	const float nDotL = saturate(dot(surface.normalWS, lightDir));

	if (nDotL > 0.0f)
	{
		const float nDotV = saturate(dot(surface.normalWS, view));
		float3 h = normalize(view + lightDir);

		float specular = GGX_Specular(surface.roughness, surface.normalWS, h, view, lightDir);
		float3 fresnel = Fresnel(surface.metallic, h, lightDir);

		lighting += specular * fresnel;
	}

	return lighting * nDotL * pointLight.color * att;
}

#endif