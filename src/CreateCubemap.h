/*
	Author : bxs3514
	Date   : 2015.11
*/

#pragma once

#include <App.h>
#include <PCH.h>
#include <Utility.h>
#include <Graphics\\Camera.h>
#include <Graphics\\Skybox.h>
#include <Graphics\\GraphicsTypes.h>

#include "MeshRenderer.h"

using namespace SampleFramework11;

class CreateCubemap
{
private:
	const uint32 CubemapWidth = 128;
	const uint32 CubemapHeight = 128;

	//MeshRenderer *meshRenderer = nullptr;

	PerspectiveCamera cubemapCamera;//Camera used to catch scenes in 6 directions
	RenderTarget2D cubemapTarget;
	DepthStencilBuffer cubemapDepthTarget;
	Float3 position;
	struct CameraStruct{
		Float3 Eye;
		Float3 LookAt;
		Float3 Up;
	}CubemapCameraStruct[6];
	static CONST CameraStruct DefaultCubemapCameraStruct[6];
	static CONST Float3 DefaultPosition;
public:
	CreateCubemap(CONST FLOAT NearClip, CONST FLOAT FarClip);
	VOID Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality);
	VOID SetPosition(Float3 position);
	CONST PerspectiveCamera &GetCubemapCamera();
	VOID GetTargetViews(RenderTarget2D &resCubemapTarget, DepthStencilBuffer &resCubemapDepthTarget);
	VOID Create(CONST DeviceManager &deviceManager, MeshRenderer *meshRenderer,
		CONST RenderTarget2D &velocityTarget, CONST Float4x4 &modelTransform, ID3D11ShaderResourceView *environmentMap,
		CONST SH9Color &environmentMapSH, CONST Float2 &jitterOffset, Skybox *skybox);
	~CreateCubemap();
};

