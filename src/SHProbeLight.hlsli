#ifndef SH_PROBELIGHT
#define SH_PROBELIGHT

#include "LightCommon.hlsli"

struct SHProbeLight
{
	float3 posWS;
	float probeIndex;

	float radius;
	float intensity;
	float pad1;
	float pad2;
};

// TODO: refactor this - this is coupled with the #of t registers in ClusteredDeferred.hlsl
StructuredBuffer<PaddedSH9Color> shProbeCoefficients : register(t11);

float SHProbeLightFallOff(float radius, float dist)
{
	float falloff = 1.0 - clamp(dist / radius, 0.0, 1.0);
	return falloff;
}

float4 CalcSHProbeLight(in Surface surface, in SHProbeLight probeLight, in float3 CameraPosWS)
{
	float3 lighting = surface.diffuse * (1.0f / 3.14159f);

	// TODO: factor out common terms between lights
	
	float3 lightDir = probeLight.posWS - surface.posWS;
	float dist = length(lightDir);
	lightDir = normalize(lightDir);

	// TODO: optimize att in size lambert test
	// const float att = PointLightAttenuation(probeLight.radius, dist);
	const float att = SHProbeLightFallOff(probeLight.radius, dist);
	const float nDotL = saturate(dot(surface.normalWS, lightDir));
	float intensity = intensity = att * probeLight.intensity;
	float3 shColor = 0.0;

	if (nDotL > 0.0f)
	{
		//float3 view = normalize(CameraPosWS - surface.posWS);
		/*const float nDotV = saturate(dot(surface.normalWS, view));
		float3 h = normalize(view + lightDir);

		float specular = GGX_Specular(surface.roughness, surface.normalWS, h, view, lightDir);
		float3 fresnel = Fresnel(surface.metallic, h, lightDir);
		lighting += specular * fresnel*/;

		PaddedSH9Color sh9 = shProbeCoefficients[probeLight.probeIndex];
		shColor = max(EvalPaddedSH9Cosine(surface.normalWS, sh9), 0.0);
	}

	return float4(lighting * nDotL * shColor * intensity, intensity);
}


#endif
