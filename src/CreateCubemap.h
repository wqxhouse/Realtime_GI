/*
	Author : bxs3514
	Date   : 2015.11
*/

#pragma once

#include <PCH.h>
#include <Graphics\\Camera.h>
#include <Graphics\\GraphicsTypes.h>

using namespace SampleFramework11;

class CreateCubemap
{
private:
	const uint32 CubemapWidth = 128;
	const uint32 CubemapHeight = 128;

	PerspectiveCamera cubemapCamera;//Camera used to catch scenes in 6 directions
	//D3D11_TEXTURE2D_DESC cubemapTex;
	//D3D11_SUBRESOURCE_DATA cubemapData;
	//std::vector<XMFLOAT4> cubemapColor[6];//RGBA color buffer of cubemap faces
	RenderTarget2D cubemapTarget;
	DepthStencilBuffer cubemapDepthTarget;
	Float3 position;
	struct CameraStruct{
		Float3 Eye;
		Float3 LookAt;
		Float3 Up;
	}CubemapCameraStruct[6];
	static const CameraStruct DefaultCubemapCameraStruct[6];
	static const Float3 DefaultPosition;
public:
	CreateCubemap(const float NearClip, const float FarClip);
	VOID Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality);
	VOID SetPosition(Float3 position);
	CONST PerspectiveCamera &GetCubemapCamera();
	VOID Create();
	~CreateCubemap();
};

