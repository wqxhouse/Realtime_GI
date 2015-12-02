#pragma once
#include <SF11_Math.h>
#include <Graphics\\GraphicsTypes.h>

using namespace SampleFramework11;

struct PointLight // 2 vec4
{
	Float3 cPos;
	float cRadius;
	Float3 cColor;
	int padding;
};

struct SpotLight // 3 vec4
{
	Float3 cPos;
	float cSpotCutOff;
	Float3 cColor;
	float cSpotPow;
	Float3 cDir;
	float pad;
};

struct SHProbeLight
{
	Float3 cPos;
	uint32 cProbeIndex;
	float cRadius;
	float cIntensity;
	float pad1;
	float pad2;
};
