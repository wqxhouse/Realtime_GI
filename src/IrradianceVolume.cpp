#include "IrradianceVolume.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "DebugRenderer.h"

IrradianceVolume::IrradianceVolume()
	: _cubemapCamera(1.0f, 90.0f * (Pi / 180), 0.01f, 40.0f) // TODO: experiment with far clip plane

{
}

void IrradianceVolume::Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, Camera *camera, DebugRenderer *debugRenderer)
{
	_device = device;
	_context = context;
	_meshRenderer = meshRenderer;
	_mainCamera = camera;
	_debugRenderer = debugRenderer;

	_cubemapSizeGBuffer = 16;
	_cubemapSizeTexcoord = 32;
	//_unitsBetweenProbes = 0.8f; // auto compute this value baesd on texture resource limit 16384
	//_unitsBetweenProbes = 50.0f; // auto compute this value baesd on texture resource limit 16384
	_unitsBetweenProbes = 3.0f; // auto compute this value baesd on texture resource limit 16384

	// setup for proxy geometry
	_proxyMeshVSConstants.Initialize(_device);
	_proxyMeshVS = CompileVSFromFile(_device, L"ProxyMeshTexcoord.hlsl", "VS");
	_proxyMeshPS = CompilePSFromFile(_device, L"ProxyMeshTexcoord.hlsl", "PS");

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	DXCall(_device->CreateInputLayout(layout, 3,
		_proxyMeshVS->ByteCode->GetBufferPointer(),
		_proxyMeshVS->ByteCode->GetBufferSize(), &_proxyMeshInputLayout));


	// ========================================================
	_dirLightMapSize = 128;
	_indirectLightMapSize = 256;

	_dirLightDiffuseBufferRT.Initialize(_device, _dirLightMapSize, _dirLightMapSize, DXGI_FORMAT_R16G16B16A16_FLOAT);
	_indirectLightDiffuseBufferRT.Initialize(_device, _indirectLightMapSize, _indirectLightMapSize, DXGI_FORMAT_R16G16B16A16_FLOAT);

	_dirLightDiffuseVS = CompileVSFromFile(_device, L"DirectDiffuse.hlsl", "VS");
	_dirLightDiffusePS = CompilePSFromFile(_device, L"DirectDiffuse.hlsl", "PS");
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

	probeNumX = Max(probeNumX, (uint32)2);
	probeNumY = Max(probeNumY, (uint32)2);
	probeNumZ = Max(probeNumZ, (uint32)2);

	_cubemapNum = probeNumX * probeNumY * probeNumZ;

	// TODO: find better way to calc this


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
	_proxyMeshTexCoordCubemapRT.Initialize(_device, texcoordAtlasWidth, texcoordAtlasHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
	_depthBufferGBufferRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_D32_FLOAT);
	_depthBufferTexcoordRT.Initialize(_device, texcoordAtlasWidth, texcoordAtlasHeight, DXGI_FORMAT_D32_FLOAT);

	_relightCubemapRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_R16G16B16A16_FLOAT);
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
}

void IrradianceVolume::RenderSceneAtlasProxyMeshTexcoord()
{
	PIXEvent event(L"RenderProxyMeshTexcoordAtlas");

	float clearColor[4] = { 0.0f, 0.0f, 1.0f, 0.0f }; // keep blue channel for sky light
	_context->ClearRenderTargetView(_proxyMeshTexCoordCubemapRT.RTView, clearColor);
	_context->ClearDepthStencilView(_depthBufferTexcoordRT.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	_context->VSSetShader(_proxyMeshVS, nullptr, 0);
	_context->PSSetShader(_proxyMeshPS, nullptr, 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->IASetInputLayout(_proxyMeshInputLayout);

	SceneObject *proxyObj = _scene->getProxySceneObjectPtr();
	Model *proxyModel = proxyObj->model;

	ID3D11RenderTargetView *renderTarget[1] = { _proxyMeshTexCoordCubemapRT.RTView };
	_context->OMSetRenderTargets(1, renderTarget, _depthBufferTexcoordRT.DSView);

	for (uint32 currCubemap = 0; currCubemap < _cubemapNum; currCubemap++)
	{
		for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
		{
			Float3 &eyePos = _positionList[currCubemap];
			_cubemapCamera.SetLookAt(eyePos,
				_CubemapCameraStruct[cubeboxFaceIndex].LookAt,
				_CubemapCameraStruct[cubeboxFaceIndex].Up);

			_proxyMeshVSConstants.Data.WorldViewProjection =
				Float4x4::Transpose(*_scene->getProxySceneObjectPtr()->base * _cubemapCamera.ViewProjectionMatrix());

			_proxyMeshVSConstants.ApplyChanges(_context);
			_proxyMeshVSConstants.SetVS(_context, 0);
			_proxyMeshVSConstants.SetPS(_context, 0);

			D3D11_VIEWPORT viewport;
			viewport.Width = static_cast<float>(_cubemapSizeTexcoord);
			viewport.Height = static_cast<float>(_cubemapSizeTexcoord);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			viewport.TopLeftX = (float)(cubeboxFaceIndex * _cubemapSizeTexcoord);
			viewport.TopLeftY = (float)(currCubemap * _cubemapSizeTexcoord);
			_context->RSSetViewports(1, &viewport);

			// Set the vertices and indices
			for (size_t i = 0; i < proxyModel->Meshes().size(); i++)
			{
				Mesh *proxyMesh = &proxyModel->Meshes()[i];

				ID3D11Buffer* vertexBuffers[1] = { proxyMesh->VertexBuffer() };
				UINT vertexStrides[1] = { proxyMesh->VertexStride() };
				UINT offsets[1] = { 0 };
				_context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
				_context->IASetIndexBuffer(proxyMesh->IndexBuffer(), proxyMesh->IndexBufferFormat(), 0);

				// TODO: frustum culling for proxy object
				for (size_t j = 0; j < proxyMesh->MeshParts().size(); j++)
				{
					const MeshPart& part = proxyMesh->MeshParts()[j];
					_context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
				}
			}

		}
	}

	renderTarget[0] = nullptr;
	_context->OMSetRenderTargets(1, renderTarget, nullptr);
}

void IrradianceVolume::RenderProxyMeshDirectLighting()
{
	PIXEvent event(L"RenderProxyMeshDirectLighting");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // keep blue channel for sky light
	_context->ClearRenderTargetView(_dirLightDiffuseBufferRT.RTView, clearColor);
	_context->VSSetShader(_dirLightDiffuseVS, nullptr, 0);
	_context->PSSetShader(_dirLightDiffusePS, nullptr, 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->IASetInputLayout(_proxyMeshInputLayout);

	SceneObject *proxyObj = _scene->getProxySceneObjectPtr();
	Model *proxyModel = proxyObj->model;

	ID3D11RenderTargetView *renderTarget[1] = { _dirLightDiffuseBufferRT.RTView };
	_context->OMSetRenderTargets(1, renderTarget, nullptr);

	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(_dirLightMapSize);
	viewport.Height = static_cast<float>(_dirLightMapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	_context->RSSetViewports(1, &viewport);

	// Set the vertices and indices
	for (size_t i = 0; i < proxyModel->Meshes().size(); i++)
	{
		Mesh *proxyMesh = &proxyModel->Meshes()[i];

		ID3D11Buffer* vertexBuffers[1] = { proxyMesh->VertexBuffer() };
		UINT vertexStrides[1] = { proxyMesh->VertexStride() };
		UINT offsets[1] = { 0 };
		_context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
		_context->IASetIndexBuffer(proxyMesh->IndexBuffer(), proxyMesh->IndexBufferFormat(), 0);

		// TODO: frustum culling for proxy object
		for (size_t j = 0; j < proxyMesh->MeshParts().size(); j++)
		{
			const MeshPart& part = proxyMesh->MeshParts()[j];
			_context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
		}
	}

	renderTarget[0] = nullptr;
	_context->OMSetRenderTargets(1, renderTarget, nullptr);
}

void IrradianceVolume::MainRender()
{
	RenderProxyMeshDirectLighting();
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