#include "IrradianceVolume.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "DebugRenderer.h"

IrradianceVolume::IrradianceVolume()
	: _cubemapCamera(1.0f, 90.0f * (Pi / 180), 0.01f, 40.0f) // TODO: experiment with far clip plane

{
}

void IrradianceVolume::Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, DebugRenderer *debugRenderer)
{
	_device = device;
	_context = context;
	_meshRenderer = meshRenderer;
	_debugRenderer = debugRenderer;

	_cubemapSizeGBuffer = 16;
	_cubemapSizeTexcoord = 32;
	_unitsBetweenProbes = 0.8f; // auto compute this value baesd on texture resource limit 16384
}

void IrradianceVolume::SetProbeDensity(float unitsBetweenProbes)
{
	_unitsBetweenProbes = unitsBetweenProbes;
	setupResourcesForScene();
}

void IrradianceVolume::SetScene(Scene *scene)
{
	_scene = scene;
	setupResourcesForScene();
}

void IrradianceVolume::setupResourcesForScene()
{
	BBox &bbox = _scene->getSceneBoundingBox();
	Float3 diff = Float3(bbox.Max) - Float3(bbox.Min);
	Float3 numProbesAxis = diff * (float)(1.0 / _unitsBetweenProbes);
	uint32 probeNumX = (uint32)ceilf(numProbesAxis.x) - 1;
	uint32 probeNumY = (uint32)ceilf(numProbesAxis.y) - 1;
	uint32 probeNumZ = (uint32)ceilf(numProbesAxis.z) - 1;

	_cubemapNum = probeNumX * probeNumY * probeNumZ;

	_positionList.clear();
	_positionList.resize(_cubemapNum);
	for (uint32 z = 0; z < probeNumZ; z++) {
	for (uint32 y = 0; y < probeNumY; y++) {
	for (uint32 x = 0; x < probeNumX; x++) {
		uint32 index = z * probeNumY * probeNumX + y * probeNumX + x;
		float posX = _unitsBetweenProbes * (1 + x);
		float posY = _unitsBetweenProbes * (1 + y);
		float posZ = _unitsBetweenProbes * (1 + z);

		Float3 positionWS = Float3(posX, posY, posZ) + Float3(bbox.Min);
		_positionList[index] = positionWS;
	}}}

	createCubemapAtlasRTs();
}

void IrradianceVolume::createCubemapAtlasRTs()
{
	uint32 gbufferAtlasWidth = 6 * _cubemapSizeGBuffer;
	uint32 gbufferAtlasHeight = _cubemapNum * _cubemapSizeGBuffer;
	uint32 texcoordAtlasWidth = 6 * _cubemapSizeTexcoord;
	uint32 texcoordAtlasHeight = _cubemapNum * _cubemapSizeTexcoord;

	_albedoCubemapRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	_normalCubemapRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_R16G16_FLOAT);
	_proxyGeomTexCoordCubemapRT.Initialize(_device, texcoordAtlasWidth, texcoordAtlasHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	_depthBufferGBufferRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_D32_FLOAT);
	_depthBufferTexcoordRT.Initialize(_device, texcoordAtlasWidth, texcoordAtlasHeight, DXGI_FORMAT_D32_FLOAT);
}

void IrradianceVolume::RenderSceneAtlasGBuffer()
{
	PIXEvent event(L"RenderSceneAtlasGBuffer");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	_context->ClearRenderTargetView(_albedoCubemapRT.RTView, clearColor);
	_context->ClearRenderTargetView(_normalCubemapRT.RTView, clearColor);
	_context->ClearDepthStencilView(_depthBufferGBufferRT.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	for (uint32 currCubemap = 0; currCubemap < _cubemapNum; currCubemap++)
	{
		Float3 &eyePos = _positionList[currCubemap];
		for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
		{
			_cubemapCamera.SetLookAt(eyePos,
				_CubemapCameraStruct[cubeboxFaceIndex].LookAt,
				_CubemapCameraStruct[cubeboxFaceIndex].Up);

			D3D11_VIEWPORT viewport;
			viewport.Width = static_cast<float>(_cubemapSizeGBuffer);
			viewport.Height = static_cast<float>(_cubemapSizeGBuffer);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = (float)(cubeboxFaceIndex * _cubemapSizeGBuffer);
			viewport.TopLeftY = (float)(currCubemap * _cubemapSizeGBuffer);

			_context->RSSetViewports(1, &viewport);

			_meshRenderer->SortSceneObjects(_cubemapCamera.ViewMatrix());

			ID3D11RenderTargetView *renderTarget[2] = { _albedoCubemapRT.RTView, _normalCubemapRT.RTView };
			_context->OMSetRenderTargets(2, renderTarget, _depthBufferGBufferRT.DSView);

			SH9Color dummySH9;
			_meshRenderer->Render(_context, _cubemapCamera, Float4x4(), nullptr, dummySH9, 0);

			renderTarget[0] = renderTarget[1] = nullptr;
			_context->OMSetRenderTargets(2, renderTarget, nullptr);
		}
	}

	//for (size_t i = 0; i < _positionList.size(); i++)
	//{
	//	_debugRenderer->QueueLightSphere(_positionList[i], Float4(1, 1, 1, 1), 0.3f);
	//}
	// TODO: this is not safe
	// SetViewport(_context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}


const IrradianceVolume::CameraStruct IrradianceVolume::_CubemapCameraStruct[6] = 
{
	{ Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Left
	{ Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Right
	{ Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 0.0f, -1.0f) },//Up
	{ Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) },//Down
	{ Float3(0.0f, 0.0f, 1.0f), Float3(0.0f, 1.0f, 0.0f) }, //Center
	{ Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, 1.0f, 0.0f) },//Back
};