#pragma once

#include "PCH.h"

// Constants - low shadow quality in debug mode to speed up
#if _DEBUG 
static const float ShadowNearClip = 1.0f;
static const float FilterSize = 0.0f;
static const uint32 SampleRadius = 0;
static const float OffsetScale = 0.0f;
static const float LightBleedingReduction = 0.10f;
static const float PositiveExponent = 40.0f;
static const float NegativeExponent = 8.0f;
static const uint32 ShadowMapSize = 512;
static const uint32 ShadowMSAASamples = 1;
static const uint32 ShadowAnisotropy = 16;
static const bool EnableShadowMips = false;
#else 
static const float ShadowNearClip = 1.0f;
static const float FilterSize = 7.0f;
static const uint32 SampleRadius = 3;
static const float OffsetScale = 0.0f;
static const float LightBleedingReduction = 0.10f;
static const float PositiveExponent = 40.0f;
static const float NegativeExponent = 8.0f;
static const uint32 ShadowMapSize = 1024;
static const uint32 ShadowMSAASamples = 4;
static const uint32 ShadowAnisotropy = 16;
static const bool EnableShadowMips = true;
#endif
