/*
	Author : bxs3514
	Date   : 2015.11
	Create environment cube map from a specific position of the world.
*/

#include "CreateCubemap.h"

const Float3 CreateCubemap::DefaultPosition = { 0.0f, 0.0f, 0.0f };

const CreateCubemap::CameraStruct CreateCubemap::DefaultCubemapCameraStruct[6] = {
		{ DefaultPosition, Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Left
		{ DefaultPosition, Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Right
		{ DefaultPosition, Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 0.0f, -1.0f) },//Up
		{ DefaultPosition, Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) },//Down
		{ DefaultPosition, Float3(0.0f, 0.0f, 1.0f), Float3(0.0f, 1.0f, 0.0f) }, //Center
		{ DefaultPosition, Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, 1.0f, 0.0f) },//Back
};

CreateCubemap::CreateCubemap(const float NearClip, const float FarClip) 
	: cubemapCamera(1.0f, 90.0f * (Pi / 180), NearClip, FarClip)
{
}

VOID CreateCubemap::Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality){
	cubemapTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, numMipLevels,
		1, 0, TRUE, FALSE, 6, TRUE);
	cubemapDepthTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_D32_FLOAT, numMipLevels, 1,
		0, 6, TRUE);
}


VOID CreateCubemap::SetPosition(Float3 newPosition){
	position = newPosition;
	for (int faceIndex = 0; faceIndex < 6; faceIndex++){
		CubemapCameraStruct[faceIndex] = { newPosition,
			newPosition + DefaultCubemapCameraStruct[faceIndex].LookAt,
			newPosition + DefaultCubemapCameraStruct[faceIndex].Up
		};
	}
}


VOID CreateCubemap::Create(CONST DeviceManager &deviceManager, MeshRenderer *meshRenderer,
	CONST RenderTarget2D &velocityTarget, CONST Float4x4 &modelTransform, ID3D11ShaderResourceView *environmentMap,
	CONST SH9Color &environmentMapSH, CONST Float2 &jitterOffset, Skybox *skybox){

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	SetViewport(deviceManager.ImmediateContext(), 128, 128);

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		ID3D11RenderTargetViewPtr RTView = cubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);
		cubemapCamera.SetLookAt(CubemapCameraStruct[cubeboxFaceIndex].Eye,
			CubemapCameraStruct[cubeboxFaceIndex].LookAt,
			CubemapCameraStruct[cubeboxFaceIndex].Up);

		deviceManager.ImmediateContext()->ClearRenderTargetView(RTView, clearColor);
		deviceManager.ImmediateContext()->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11RenderTargetView *renderTarget[1] = { RTView };
		deviceManager.ImmediateContext()->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->RenderDepth(deviceManager.ImmediateContext(), cubemapCamera, modelTransform, false);

		deviceManager.ImmediateContext()->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->Render(deviceManager.ImmediateContext(), cubemapCamera, modelTransform, 
			environmentMap, environmentMapSH, jitterOffset, TRUE);

		skybox->RenderEnvironmentMap(deviceManager.ImmediateContext(), environmentMap, cubemapCamera.ViewMatrix(),
			cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));
		
		renderTarget[0] = nullptr;
		deviceManager.ImmediateContext()->OMSetRenderTargets(1, renderTarget, nullptr);
	}

	SetViewport(deviceManager.ImmediateContext(), deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}


CONST PerspectiveCamera &CreateCubemap::GetCubemapCamera(){
	const int face = 4;
	cubemapCamera.SetLookAt(DefaultCubemapCameraStruct[face].Eye,
		DefaultCubemapCameraStruct[face].LookAt,
		DefaultCubemapCameraStruct[face].Up);
	return cubemapCamera;
}

VOID CreateCubemap::GetTargetViews(RenderTarget2D &resCubemapTarget){
	resCubemapTarget = cubemapTarget;
}


CreateCubemap::~CreateCubemap()
{
}
