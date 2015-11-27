#include "IrradianceVolume.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "DebugRenderer.h"
#include "BoundUtils.h"

IrradianceVolume::IrradianceVolume()
	: _cubemapCamera(1.0f, 90.0f * (Pi / 180), 0.01f, 40.0f), // TODO: experiment with far clip plane
	_dirLightCam(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f), _weightSum(0.0f)

{
}

void IrradianceVolume::Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, 
	Camera *camera, StructuredBuffer *pointLightBuffer, LightClusters *clusters, DebugRenderer *debugRenderer)
{
	_device = device;
	_context = context;
	_meshRenderer = meshRenderer;
	_mainCamera = camera;
	_clusters = clusters;
	_debugRenderer = debugRenderer;

	_cubemapSizeGBuffer = 16;
	_cubemapSizeTexcoord = 32;
	//_unitsBetweenProbes = 0.8f; // auto compute this value baesd on texture resource limit 16384
	//_unitsBetweenProbes = 50.0f; // auto compute this value baesd on texture resource limit 16384
	_unitsBetweenProbes = 1.0f; // auto compute this value based on texture resource limit 16384

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


	_meshDepthVS = CompileVSFromFile(_device, L"SimpleDepthOnly.hlsl", "VS");
	DXCall(_device->CreateInputLayout(layout, 1, _meshDepthVS->ByteCode->GetBufferPointer(), 
		_meshDepthVS->ByteCode->GetBufferSize(), &_proxyMeshDepthInputLayout));


	// ========================================================
	_dirLightMapSize = 128;
	_indirectLightMapSize = 256;

	_dirLightDiffuseBufferRT.Initialize(_device, _dirLightMapSize, _dirLightMapSize, DXGI_FORMAT_R16G16B16A16_FLOAT);
	_indirectLightDiffuseBufferRT.Initialize(_device, _indirectLightMapSize, _indirectLightMapSize, DXGI_FORMAT_R16G16B16A16_FLOAT);

	_dirLightDiffuseVS = CompileVSFromFile(_device, L"DirectDiffuse.hlsl", "VS");
	_dirLightDiffusePS = CompilePSFromFile(_device, L"DirectDiffuse.hlsl", "PS");

	_dirLightProxyMeshShadowMapBuffer.Initialize(_device, _dirLightMapSize, _dirLightMapSize, DXGI_FORMAT_D16_UNORM, true);

	_rasterizerStates.Initialize(_device);
	_depthStencilStates.Initialize(_device);

	_directDiffuseConstants.Initialize(_device);
	_pointLightBuffer = pointLightBuffer;
	_samplerStates.Initialize(_device);

	_relightCubemapVS = CompileVSFromFile(_device, L"RelightCubemap.hlsl", "VS");
	_relightCubemapPS = CompilePSFromFile(_device, L"RelightCubemap.hlsl", "PS");

	CompileOptions opts;
	opts.Add("CubemapSize_", _cubemapSizeGBuffer);
	_relightSHIntegrateCS = CompileCSFromFile(_device, L"RelightSH.hlsl", "IntegrateCS", "cs_5_0", opts);
	_relightSHReductionCS = CompileCSFromFile(_device, L"RelightSH.hlsl", "ReductionCS", "cs_5_0", opts);
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
	//uint32 probeNumX = (uint32)ceilf(numProbesAxis.x) - 1;
	//uint32 probeNumY = (uint32)ceilf(numProbesAxis.y) - 1;
	//uint32 probeNumZ = (uint32)ceilf(numProbesAxis.z) - 1;

	//probeNumX = Max(probeNumX, (uint32)2);
	//probeNumY = Max(probeNumY, (uint32)2);
	//probeNumZ = Max(probeNumZ, (uint32)2);

	//_cubemapNum = probeNumX * probeNumY * probeNumZ;

	//// TODO: find better way to calc this


	//_positionList.clear();
	//_positionList.resize(_cubemapNum);
	//for (uint32 z = 0; z < probeNumZ; z++) {
	//for (uint32 y = 0; y < probeNumY; y++) {
	//for (uint32 x = 0; x < probeNumX; x++) {
	//	uint32 index = z * probeNumY * probeNumX + y * probeNumX + x;
	//	float posX = _unitsBetweenProbes * (1 + x);
	//	float posY = _unitsBetweenProbes * (1 + y);
	//	float posZ = _unitsBetweenProbes * (1 + z);

	//	Float3 positionWS = Float3(posX, posY, posZ) + Float3(bbox.Min);
	//	_positionList[index] = positionWS;
	//}}}

	uint32 numProbesX = (uint32)floorf(numProbesAxis.x);
	uint32 numProbesY = (uint32)floorf(numProbesAxis.y);
	uint32 numProbesZ = (uint32)floorf(numProbesAxis.z);

	numProbesX = Max(numProbesX, (uint32)2);
	numProbesY = Max(numProbesY, (uint32)2);
	numProbesZ = Max(numProbesZ, (uint32)2);

	_cubemapNum = numProbesX * numProbesY * numProbesZ;

	float calcUnitDistX = diff.x / (numProbesX+1);
	float calcUnitDistY = diff.y / (numProbesY+1);
	float calcUnitDistZ = diff.z / (numProbesZ+1);

	_positionList.clear();
	_positionList.resize(_cubemapNum);
	for (uint32 z = 0; z < numProbesZ; z++) {
		for (uint32 y = 0; y < numProbesY; y++) {
			for (uint32 x = 0; x < numProbesX; x++) {
				uint32 index = z * numProbesY * numProbesX + y * numProbesX + x;
				float posX = calcUnitDistX * (1 + x);
				float posY = calcUnitDistX  * (1 + y);
				float posZ = calcUnitDistX  * (1 + z);

				Float3 positionWS = Float3(posX, posY, posZ) + Float3(bbox.Min);
				_positionList[index] = positionWS;
			}
		}
	}


	createCubemapAtlasRTs();
	createSHComputeBuffers();

	_probeLights.clear();
	_probeLights.resize(_cubemapNum);
}

void IrradianceVolume::createSHComputeBuffers()
{
	_viewToWorldMatrixPalette.Initialize(_device, sizeof(Float4x4), 6 * _cubemapNum, 1);
	_shIntegrationConstants.Initialize(_device);

	// 3 is #(rgb) channel ; 6 is #cubefaces
	_relightIntegrationBuffer.Initialize(_device, sizeof(PackedSH9), _cubemapSizeGBuffer * 3 * 6 * _cubemapNum, 1, 1);
	_relightSHBuffer.Initialize(_device, sizeof(PaddedSH9Color), _cubemapNum, 1, 1);

	_weightSum = 0.0f;
	// Compute the final weight for integration
	for (UINT y = 0; y < _cubemapSizeGBuffer; ++y) {
		for (UINT x = 0; x < _cubemapSizeGBuffer; ++x) {

			const float u = (float(x) / float(_cubemapSizeGBuffer)) * 2.0f - 1.0f;
			const float v = (float(y) / float(_cubemapSizeGBuffer)) * 2.0f - 1.0f;

			const float temp = 1.0f + u * u + v * v;
			const float weight = 4.0f / (sqrtf(temp) * temp);

			_weightSum += weight;
		}
	}

	_weightSum *= 6.0f;

	// TODO: figure out a way to move this out of rendering loop
	_shIntegrationConstants.Data.FinalWeight = (4.0f * 3.14159f) / _weightSum;
	_shIntegrationConstants.ApplyChanges(_context);
	_shIntegrationConstants.SetCS(_context, 0);

	//===========
	// Set view matrix palette - TODO: make it calc in the shader to save storage and upload cost
	Float4x4 palette[6 * MAX_PROBE_NUM];
	for (uint32 currCubemap = 0; currCubemap < _cubemapNum; currCubemap++)
	{
		Float3 &eyePos = _positionList[currCubemap];
		for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
		{
			_cubemapCamera.SetLookAt(eyePos,
				_CubemapCameraStruct[cubeboxFaceIndex].LookAt,
				_CubemapCameraStruct[cubeboxFaceIndex].Up);

			palette[currCubemap * 6 + cubeboxFaceIndex]
				= Float4x4::Transpose(_cubemapCamera.WorldMatrix());
		}
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	BYTE* mappedData = NULL;
	_context->Map(_viewToWorldMatrixPalette.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	memcpy(mappedData, palette, sizeof(Float4x4) * _cubemapNum * 6);
	_context->Unmap(_viewToWorldMatrixPalette.Buffer, 0);
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

	//_relightCubemapRT.Initialize(_device, gbufferAtlasWidth, gbufferAtlasHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 0, 0, 1, 1, 0);
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
				_CubemapCameraStruct[cubeboxFaceIndex].LookAt + eyePos,
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

	ID3D11RenderTargetView *renderTarget[1] = { _proxyMeshTexCoordCubemapRT.RTView };
	_context->OMSetRenderTargets(1, renderTarget, _depthBufferTexcoordRT.DSView);

	for (uint32 currCubemap = 0; currCubemap < _cubemapNum; currCubemap++)
	{
		for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
		{
			Float3 &eyePos = _positionList[currCubemap];
			_cubemapCamera.SetLookAt(eyePos,
				_CubemapCameraStruct[cubeboxFaceIndex].LookAt + eyePos,
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

			renderProxyModel();
		}
	}

	renderTarget[0] = nullptr;
	_context->OMSetRenderTargets(1, renderTarget, nullptr);
}

void IrradianceVolume::renderProxyModel()
{
	SceneObject *proxyObj = _scene->getProxySceneObjectPtr();
	Model *proxyModel = proxyObj->model;

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

void IrradianceVolume::renderProxyMeshShadowMap()
{
	PIXEvent event(L"RenderProxyMeshShadowMap");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // keep blue channel for sky light
	//_context->RSSetState(_rasterizerStates.BackFaceCull());
	_context->RSSetState(_rasterizerStates.NoCull());
	_context->OMSetDepthStencilState(_depthStencilStates.DepthWriteEnabled(), 0);
	ID3D11RenderTargetView* renderTargets[1] = { nullptr };
	_context->OMSetRenderTargets(0, renderTargets, _dirLightProxyMeshShadowMapBuffer.DSView);

	_context->ClearDepthStencilView(_dirLightProxyMeshShadowMapBuffer.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	_context->VSSetShader(_meshDepthVS, nullptr, 0);
	_context->PSSetShader(nullptr, nullptr, 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->IASetInputLayout(_proxyMeshDepthInputLayout);

	D3D11_VIEWPORT viewport;
	viewport.Width = static_cast<float>(_dirLightMapSize);
	viewport.Height = static_cast<float>(_dirLightMapSize);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	_context->RSSetViewports(1, &viewport);

	
	_proxyMeshVSConstants.Data.WorldViewProjection =
		Float4x4::Transpose(*_scene->getProxySceneObjectPtr()->base * _dirLightCam.ViewProjectionMatrix());
	_proxyMeshVSConstants.ApplyChanges(_context);
	_proxyMeshVSConstants.SetVS(_context, 0);

	renderProxyModel();
	_debugRenderer->QueueSprite(_dirLightProxyMeshShadowMapBuffer.SRView, Float3(0, 128, 0), Float4(1, 1, 1, 1));
	_context->OMSetRenderTargets(0, renderTargets, nullptr);
}

void IrradianceVolume::renderProxyMeshDirectLighting()
{
	PIXEvent event(L"RenderProxyMeshDirectLighting");

	_context->RSSetState(_rasterizerStates.NoCull());
	_context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // keep blue channel for sky light
	_context->ClearRenderTargetView(_dirLightDiffuseBufferRT.RTView, clearColor);
	_context->VSSetShader(_dirLightDiffuseVS, nullptr, 0);
	_context->PSSetShader(_dirLightDiffusePS, nullptr, 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->IASetInputLayout(_proxyMeshInputLayout);

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

	// set constant buffer
	_directDiffuseConstants.Data.ModelToWorld = Float4x4::Transpose(*_scene->getProxySceneObjectPtr()->base);
	_directDiffuseConstants.Data.WorldToLightProjection = Float4x4::Transpose(_dirLightCam.ViewProjectionMatrix());
	_directDiffuseConstants.Data.ClusterBias = _clusters->getClusterBias();
	_directDiffuseConstants.Data.ClusterScale = _clusters->getClusterScale();
	_directDiffuseConstants.ApplyChanges(_context);
	_directDiffuseConstants.SetVS(_context, 0);
	_directDiffuseConstants.SetPS(_context, 0);

	// bind SRVs
	ID3D11ShaderResourceView* srvs[] =
	{
		_dirLightProxyMeshShadowMapBuffer.SRView,
		_pointLightBuffer->SRView,
		_clusters->getLightIndicesListSRV(), 
		_clusters->getClusterTexSRV(),
	};

	_context->PSSetShaderResources(0, _countof(srvs), srvs);

	ID3D11SamplerState* sampStates[1] = {
		_samplerStates.ShadowMapPCF(),
	};

	_context->PSSetSamplers(0, 1, sampStates);

	renderProxyModel();

	// claer states
	renderTarget[0] = nullptr;
	_context->OMSetRenderTargets(1, renderTarget, nullptr);
	ID3D11ShaderResourceView* nullSRVs[_countof(srvs)] = { nullptr };
	_context->PSSetShaderResources(0, _countof(srvs), nullSRVs);

	sampStates[0] = nullptr;
	_context->PSSetSamplers(0, 1, sampStates);

	_debugRenderer->QueueSprite(_dirLightDiffuseBufferRT.SRView, Float3(0, 0, 0), Float4(1, 1, 1, 1));
}

void IrradianceVolume::renderRelightCubemap()
{
	PIXEvent event(L"RenderRelightCubemap");

	_context->RSSetState(_rasterizerStates.NoCull());
	_context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);

	ID3D11Buffer* vbs[1] = { NULL };
	uint32 strides[1] = { 0 };
	uint32 offsets[1] = { 0 };
	_context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	_context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
	_context->IASetInputLayout(NULL);

	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.Width = static_cast<float>(_relightCubemapRT.Width);
	vp.Height = static_cast<float>(_relightCubemapRT.Height);
	_context->RSSetViewports(1, &vp);

	_context->VSSetShader(_relightCubemapVS, NULL, 0);
	_context->PSSetShader(_relightCubemapPS, NULL, 0);

	ID3D11RenderTargetView* rtvs[1] = { _relightCubemapRT.RTView };
	_context->OMSetRenderTargets(1, rtvs, NULL);

	ID3D11ShaderResourceView* srvs[3] = { 
		_dirLightDiffuseBufferRT.SRView, 
		_proxyMeshTexCoordCubemapRT.SRView, 
		_albedoCubemapRT.SRView
	};

	ID3D11SamplerState* sampStates[1] = {
		_samplerStates.Linear(),
	};
	_context->PSSetSamplers(0, 1, sampStates);
	_context->PSSetShaderResources(0, 3, srvs);

	_context->Draw(3, 0);

	srvs[0] = srvs[1] = srvs[2] = NULL;
	_context->PSSetShaderResources(0, 3, srvs);
	sampStates[0] = nullptr;
	_context->PSSetSamplers(0, 1, sampStates);
	rtvs[0] = { nullptr };
	_context->OMSetRenderTargets(1, rtvs, NULL);

	_debugRenderer->QueueSprite(_relightCubemapRT.SRView, Float3(0, 256, 0), Float4(1, 1, 1, 1));
}

void IrradianceVolume::Update()
{
	// update light cam
	// Shadow camera construction - transform scene aabb to light space aabb
	BBox &bbox = _scene->getSceneBoundingBox();
	Float3 lightCameraPos = Float3(0, 0, 0);
	Float3 lookAt = lightCameraPos - AppSettings::LightDirection;
	Float3 upDir = Float3(0.0f, 1.0f, 0.0f);
	Float4x4 lightView = XMMatrixLookAtLH(lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD());
	BBox lightSpaceBBox = GetTransformedBBox(bbox, lightView);
	Float3 lightMin = lightSpaceBBox.Min;
	Float3 lightMax = lightSpaceBBox.Max;

	OrthographicCamera shadowCamera(lightMin.x, lightMin.y, lightMax.x,
		lightMax.y, lightMin.z, lightMax.z);
	shadowCamera.SetLookAt(lightCameraPos, lookAt, upDir);
	_dirLightCam = shadowCamera;

	// update probe lights
	for (uint32 i = 0; i < _cubemapNum; i++)
	{
		_probeLights[i].cPos = _positionList[i];
		_probeLights[i].cRadius = _unitsBetweenProbes * 2.0f;
		_probeLights[i].cProbeIndex = i;
		_probeLights[i].cIntensity = AppSettings::DiffuseGI_Intensity;
	}
}

void IrradianceVolume::MainRender()
{
	renderProxyMeshShadowMap();
	renderProxyMeshDirectLighting();
	renderRelightCubemap();
	IntegrateSH();
}

void IrradianceVolume::IntegrateSH()
{
	PIXEvent intEvent(L"SH Integration");

	// Set shaders
	_context->VSSetShader(NULL, NULL, 0);
	_context->GSSetShader(NULL, NULL, 0);
	_context->DSSetShader(NULL, NULL, 0);
	_context->HSSetShader(NULL, NULL, 0);
	_context->PSSetShader(NULL, NULL, 0);
	_context->CSSetShader(_relightSHIntegrateCS, NULL, 0);

	// Set shader resources
	ID3D11ShaderResourceView* srvs[2] = { _relightCubemapRT.SRView, _viewToWorldMatrixPalette.SRView };
	_context->CSSetShaderResources(0, 2, srvs);

	// Set the output textures
	ID3D11UnorderedAccessView* outputBuffer[1] = { _relightIntegrationBuffer.UAView };
	_context->CSSetUnorderedAccessViews(0, 1, outputBuffer, NULL);

	// Do the initial integration + reduction
	_context->Dispatch(_cubemapNum, _cubemapSizeGBuffer, 6);

	// clear srvs
	// Set shader resources
	srvs[0] = nullptr;
	srvs[1] = nullptr;
	_context->CSSetShaderResources(0, 2, srvs);

	// ==================================================================

	// Set the shader
	_context->CSSetShader(_relightSHReductionCS, NULL, 0);

	ID3D11UnorderedAccessView* buffers[2] = {
		_relightIntegrationBuffer.UAView,
		_relightSHBuffer.UAView,
	};

	// Set outputs
	//outputBuffer[0] = _relightSHBuffer.UAView;
	_context->CSSetUnorderedAccessViews(0, 2, buffers, NULL);

	_shIntegrationConstants.SetCS(_context, 0);

	// Set shader resources
	//srvs[0] = _relightIntegrationBuffer.SRView;
	//srvs[1] = nullptr;
	//_context->CSSetShaderResources(0, 2, srvs);

	// Do the final reduction
	_context->Dispatch(_cubemapNum, 3, 1);

	// Clear out the SRV's and RT's
	//srvs[0] = srvs[1] = NULL;
	//_context->CSSetShaderResources(0, 1, srvs);

	buffers[0] = buffers[1] = NULL;
	_context->CSSetUnorderedAccessViews(0, 2, buffers, NULL);
	_context->CSSetShader(nullptr, NULL, 0);
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