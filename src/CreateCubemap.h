/*
	Author : bxs3514
	Date   : 2015.11
	Cubemap generator
*/

#pragma once

#include <App.h>
#include <PCH.h>
#include <Utility.h>
#include <AppSettings.h>
#include <Graphics\\Camera.h>
#include <Graphics\\Skybox.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\Model.h>
#include <Graphics\\ShaderCompilation.h>

#include "MeshRenderer.h"

#define SPHERE_FILE L"..\\Content\\Models\\sphere\\sphere.obj"

using namespace SampleFramework11;

class CreateCubemap
{

public:
	CreateCubemap(const FLOAT NearClip, const FLOAT FarClip);
	~CreateCubemap();

	void Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality);
	void SetPosition(Float3 position);
	Float3 GetPosition();
	void GetTargetViews(RenderTarget2D &resCubemapTarget);
	void Create(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox);

	const PerspectiveCamera &GetCubemapCamera();

private:
	void RenderPrefilterCubebox(const DeviceManager &deviceManager, const Float4x4 &sceneTransform);
	void GenAndCacheConvoluteSphereInputLayout(const DeviceManager &deviceManager, const Model *model);

	const uint32 CubemapWidth = 128;
	const uint32 CubemapHeight = 128;

	PerspectiveCamera cubemapCamera;
	RenderTarget2D cubemapTarget;
	DepthStencilBuffer cubemapDepthTarget;
	RenderTarget2D prefilterCubemapTarget;
	Float3 position;

	VertexShaderPtr _convoluteVS;
	PixelShaderPtr _convolutePS;

	struct CameraStruct{
		Float3 Eye;
		Float3 LookAt;
		Float3 Up;
	} CubemapCameraStruct[6];

	static const CameraStruct DefaultCubemapCameraStruct[6];
	static const Float3 DefaultPosition;

	struct VSConstant
	{
		Float4x4 world;
	};

	ConstantBuffer<VSConstant> _VSConstant;
};

