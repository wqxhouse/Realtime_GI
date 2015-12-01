/*
	Author : bxs3514
	Date   : 2015.11
	
	Create environment cube map from a specific position of the world.
	Pre-filter the cubemap.
*/

#include "CreateCubemap.h"
#include "MeshRenderer.h"

#define DefaultNearClip 0.01f
#define DefaultFarClip 300.0f

const Float3 CreateCubemap::DefaultPosition = { 0.0f, 0.0f, 0.0f };

const CreateCubemap::CameraStruct CreateCubemap::DefaultCubemapCameraStruct[6] = {
		{ DefaultPosition, Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Left
		{ DefaultPosition, Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Right
		{ DefaultPosition, Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 0.0f, -1.0f) },//Up
		{ DefaultPosition, Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) },//Down
		{ DefaultPosition, Float3(0.0f, 0.0f, 1.0f), Float3(0.0f, 1.0f, 0.0f) }, //Center
		{ DefaultPosition, Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, 1.0f, 0.0f) },//Back
};

CreateCubemap::CreateCubemap()
	:_cubemapCamera(1.0f, 90.0f * (Pi / 180), DefaultNearClip, DefaultFarClip),
	_filterCamera(1.0f, 90.0f * (Pi / 180), DefaultNearClip, DefaultFarClip)
{
}

void CreateCubemap::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_device = device;
	_context = context;

	if (!_convolveSphereSet)
	{
		//_convolveSphere.CreateWithAssimp(_device, L"..\\Content\\Models\\sphere\\sphere.obj", false);
		_convolveSphere.CreateWithAssimp(_device, L"..\\Content\\Models\\sphere\\sphere_new.FBX", false);
		GenFilterShader();
		GenAndCacheConvoluteSphereInputLayout();
		_convolveSphereSet = true;
	}

	_CubemapWidth = 128;
	_CubemapHeight = 128; 

	_cubemapTarget.Initialize(device, _CubemapWidth, _CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 8,
		1, 0, TRUE, FALSE, 6, TRUE);
	_cubemapDepthTarget.Initialize(device, _CubemapWidth, _CubemapHeight, DXGI_FORMAT_D32_FLOAT, true, 1,
		0, 6, TRUE);
	_prefilterCubemapTarget.Initialize(device, _CubemapWidth, _CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 8,
		1, 0, TRUE, FALSE, 6, TRUE);
	_prefilterDepthTarget.Initialize(device, _CubemapWidth, _CubemapHeight, DXGI_FORMAT_D32_FLOAT, true, 1,
		0, 6, TRUE);

	//_convolutePS = CompilePSFromFile(device, L"PrefilterCubemap.hlsl", "PS", "ps_5_0");

	_VSConstants.Initialize(device);//Initialize original vertex constant variables
	_PSConstants.Initialize(device);
	_blendStates.Initialize(device);
	_rasterizerStates.Wireframe();
	_rasterizerStates.Initialize(device);

	_depthStencilStates.Initialize(device);

	_samplerStates.Initialize(device);

	D3D11_SAMPLER_DESC sampDesc = SamplerStates::AnisotropicDesc();
	sampDesc.MaxAnisotropy = ShadowAnisotropy;
	DXCall(device->CreateSamplerState(&sampDesc, &_evsmSampler));
}

void CreateCubemap::SetClipPlane(float nearPlane, float farPlane)
{
	_nearPlane = nearPlane;
	_farPlane = farPlane;
}

void CreateCubemap::GenFilterShader()
{
	_convoluteVS = CompileVSFromFile(_device, L"PrefilterCubemap.hlsl", "VS", "vs_5_0");
	opts.Reset();

	for (uint32 filterFaceIndex = 0; filterFaceIndex < 6; ++filterFaceIndex)
	{
		opts.Add("FilterFaceIndex_", filterFaceIndex);
		_convolutePS[filterFaceIndex] = CompilePSFromFile(_device, L"PrefilterCubemap.hlsl", "PS", "ps_5_0", opts);
	}
}

void CreateCubemap::SetPosition(const Float3 &newPosition)
{
	_position = newPosition;
	for (int faceIndex = 0; faceIndex < 6; faceIndex++)
	{
		CubemapCameraStruct[faceIndex] = 
		{	newPosition,
			newPosition + DefaultCubemapCameraStruct[faceIndex].LookAt,
			DefaultCubemapCameraStruct[faceIndex].Up
		};
	}
}

Float3 CreateCubemap::GetPosition()
{
	return _position;
}


void CreateCubemap::SetBoxSize(const Float3 &newBoxSize)
{
	_boxSize = newBoxSize;
}

Float3 CreateCubemap::GetBoxSize()
{
	return _boxSize;
}

void CreateCubemap::GenAndCacheConvoluteSphereInputLayout()
{	
	ID3D11InputLayoutPtr _meshInputLayouts;
	VertexShaderPtr _sphereVertexShader;

	for (uint64 i = 0; i < _convolveSphere.Meshes().size(); ++i)
	{
		const Mesh& mesh = _convolveSphere.Meshes()[i];

		DXCall(_device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
			_convoluteVS->ByteCode->GetBufferPointer(), _convoluteVS->ByteCode->GetBufferSize(), &_inputLayout));

	}
}

void CreateCubemap::RenderPrefilterCubebox(const Float4x4 &sceneTransform)
{
	PIXEvent event(L"Prefilter Cubebox");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	Model *cubemapSphere = &_convolveSphere;

	SetViewport(_context, _CubemapWidth, _CubemapHeight);

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		_filterCamera.SetLookAt(DefaultCubemapCameraStruct[cubeboxFaceIndex].Eye,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].LookAt,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].Up);

		ID3D11RenderTargetViewPtr RTView = _prefilterCubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = _prefilterDepthTarget.ArraySlices.at(cubeboxFaceIndex);

		_context->ClearRenderTargetView(RTView, clearColor);
		_context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11RenderTargetView *renderTarget[1] = { RTView };
		_context->OMSetRenderTargets(1, renderTarget, DSView);

		//Start Render
		//for (int mipLevel = 0; mipLevel < 8; ++mipLevel){
		_PSConstants.Data.MipLevel = 0;
		_PSConstants.Data.CameraPos = _filterCamera.Position();

		// Set states
		float blendFactor[4] = { 1, 1, 1, 1 };
		_context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
		_context->OMSetDepthStencilState(_depthStencilStates.DepthEnabled(), 0);
		_context->RSSetState(_rasterizerStates.NoCull());
		//context->RSSetState(_rasterizerStates.BackFaceCull());

		ID3D11SamplerState* sampStates[3] = {
			_samplerStates.Anisotropic(),
			_evsmSampler,
			_samplerStates.LinearClamp(),
		};

		_context->PSSetSamplers(0, 3, sampStates);

		// Set shaders
		_context->DSSetShader(nullptr, nullptr, 0);
		_context->HSSetShader(nullptr, nullptr, 0);
		_context->GSSetShader(nullptr, nullptr, 0);

		//Set Constant Variables
		_VSConstants.Data.world = sceneTransform;
		_VSConstants.Data.View = Float4x4::Transpose(_filterCamera.ViewMatrix());
		_VSConstants.Data.WorldViewProjection = Float4x4::Transpose(sceneTransform * _filterCamera.ProjectionMatrix());//meshRenderer->_meshVSConstants.Data.WorldViewProjection;
		_VSConstants.ApplyChanges(_context);
		_VSConstants.SetVS(_context, 0);

		for (uint64 meshIdx = 0; meshIdx < cubemapSphere->Meshes().size(); ++meshIdx)
		{
			const Mesh& mesh = cubemapSphere->Meshes()[meshIdx];

			_context->VSSetShader(_convoluteVS, nullptr, 0);
			_context->PSSetShader(_convolutePS[cubeboxFaceIndex], nullptr, 0);

			ID3D11Buffer *vertexBuffers[1] = { mesh.VertexBuffer() };
			UINT vertexStrides[1] = { mesh.VertexStride() };
			UINT offsets[1] = { 0 };
			_context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
			_context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
			_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			_context->IASetInputLayout(_inputLayout);

			for (uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
			{
				const MeshPart& part = mesh.MeshParts()[partIdx];
				const MeshMaterial& material = cubemapSphere->Materials()[part.MaterialIdx];

				//Set ShaderResourse
				ID3D11ShaderResourceView* cubemapResourse[1] = { _cubemapTarget.SRView };

				_context->PSSetShaderResources(0, _countof(cubemapResourse), cubemapResourse);
				_context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
			}
		}
	}

	// SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}

void CreateCubemap::Create(MeshRenderer *meshRenderer, 
	const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox)
{
	PIXEvent event(L"Render Cube Map");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	SetViewport(_context, _CubemapWidth, _CubemapHeight);

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		ID3D11RenderTargetViewPtr RTView = _cubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = _cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);
		/*ID3D11RenderTargetViewPtr PreRTView = prefilterCubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr PreDSView = prefilterDepthTarget.ArraySlices.at(cubeboxFaceIndex);*/

		_cubemapCamera.SetLookAt(CubemapCameraStruct[cubeboxFaceIndex].Eye,
			CubemapCameraStruct[cubeboxFaceIndex].LookAt,
			CubemapCameraStruct[cubeboxFaceIndex].Up);

		meshRenderer->SortSceneObjects(_cubemapCamera.ViewMatrix());

		_context->ClearRenderTargetView(RTView, clearColor);
		_context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11RenderTargetView *renderTarget[1] = { RTView };
		_context->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->RenderDepth(_context, _cubemapCamera, sceneTransform, false);

		meshRenderer->ComputeShadowDepthBoundsCPU(_cubemapCamera);

		// TODO: figure out how to reduce depth on a cubemap
		// meshRenderer->ReduceDepth(context, cubemapDepthTarget, cubemapCamera);
		meshRenderer->RenderShadowMap(_context, _cubemapCamera, sceneTransform);

		_context->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->Render(_context, _cubemapCamera, sceneTransform, environmentMap, environmentMapSH, jitterOffset);

		skybox->RenderEnvironmentMap(_context, environmentMap, _cubemapCamera.ViewMatrix(),
			_cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));

		/*renderTarget[0] = PreRTView;
		context->OMSetRenderTargets(1, renderTarget, PreDSView);
		meshRenderer->Render(context, cubemapCamera, sceneTransform, cubemapTarget.SRView, environmentMapSH, jitterOffset);

		skybox->RenderEnvironmentMap(context, cubemapTarget.SRView, cubemapCamera.ViewMatrix(),
			cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));*/

		renderTarget[0] = nullptr;
		_context->OMSetRenderTargets(1, renderTarget, nullptr);


		//Create pre-filiter cubebox from original cubebox.
		//RenderPrefilterCubebox(deviceManager, sceneTransform);
	}

	// SetViewport(_context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}


const PerspectiveCamera &CreateCubemap::GetCubemapCamera()
{
	const int face = 4;
	_cubemapCamera.SetLookAt(DefaultCubemapCameraStruct[face].Eye,
		DefaultCubemapCameraStruct[face].LookAt,
		DefaultCubemapCameraStruct[face].Up);
	return _cubemapCamera;
}

void CreateCubemap::GetTargetViews(RenderTarget2D &resCubemapTarget)
{
	resCubemapTarget = _cubemapTarget;
}

void CreateCubemap::SetCubemapSize(uint32 size)
{
	_CubemapWidth = size;
	_CubemapHeight = size;
}

void CreateCubemap::GetPreFilterRT(RenderTarget2D &prefilterTarget)
{
	prefilterTarget = _prefilterCubemapTarget;
}

CreateCubemap::~CreateCubemap()
{
}

Model CreateCubemap::_convolveSphere;
bool32 CreateCubemap::_convolveSphereSet;
VertexShaderPtr CreateCubemap::_convoluteVS;
PixelShaderPtr CreateCubemap::_convolutePS[6];
ID3D11InputLayoutPtr CreateCubemap::_inputLayout;