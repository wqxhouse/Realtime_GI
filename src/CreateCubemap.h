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

public:
	CreateCubemap(const FLOAT NearClip, const FLOAT FarClip);
	~CreateCubemap();

	void Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality);
	void SetPosition(Float3 position);
	void GetTargetViews(RenderTarget2D &resCubemapTarget);
	void Create(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox);

	void SetCubemapSize(uint32 size);

	const PerspectiveCamera &GetCubemapCamera();

private:
	uint32 _CubemapWidth;
	uint32 _CubemapHeight;

	PerspectiveCamera cubemapCamera;
	RenderTarget2D cubemapTarget;
	DepthStencilBuffer cubemapDepthTarget;
	Float3 position;

	struct CameraStruct{
		Float3 Eye;
		Float3 LookAt;
		Float3 Up;
	} CubemapCameraStruct[6];

	static const CameraStruct DefaultCubemapCameraStruct[6];
	static const Float3 DefaultPosition;
};

