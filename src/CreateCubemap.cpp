/*
	Author : bxs3514
	Date   : 2015.11
	Create environment cube map from a specific position of the world.
*/

#include "CreateCubemap.h"

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
	:cubemapCamera(1.0f, 90.0f * (Pi / 180), DefaultNearClip, DefaultFarClip),
	filterCamera(1.0f, 90.0f * (Pi / 180), DefaultNearClip, DefaultFarClip)
{

}

CreateCubemap::CreateCubemap(const float NearClip, const float FarClip) 
	: cubemapCamera(1.0f, 90.0f * (Pi / 180), NearClip, FarClip),
	filterCamera(1.0f, 90.0f * (Pi / 180), NearClip, FarClip)
{
}

void CreateCubemap::Initialize(ID3D11Device *device)
{
	cubemapTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 8,
		1, 0, TRUE, FALSE, 6, TRUE);
	cubemapDepthTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_D32_FLOAT, true, 1,
		0, 6, TRUE);
	prefilterCubemapTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 8,
		1, 0, TRUE, FALSE, 6, TRUE);
	prefilterDepthTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_D32_FLOAT, true, 1,
		0, 6, TRUE);

	GenFilterShader(device);
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

void CreateCubemap::GenFilterShader(ID3D11Device *device)
{
	_convoluteVS = CompileVSFromFile(device, L"PrefilterCubemap.hlsl", "VS", "vs_5_0");
	opts.Reset();

	for (uint32 filterFaceIndex = 0; filterFaceIndex < 6; ++filterFaceIndex){
		opts.Add("FilterFaceIndex_", filterFaceIndex);
		_convolutePS[filterFaceIndex] = CompilePSFromFile(device, L"PrefilterCubemap.hlsl", "PS", "ps_5_0", opts);
	}

}


void CreateCubemap::SetPosition(Float3 newPosition)
{
	position = newPosition;
	for (int faceIndex = 0; faceIndex < 6; faceIndex++)
	{
		CubemapCameraStruct[faceIndex] = 
		{	newPosition,
			newPosition + DefaultCubemapCameraStruct[faceIndex].LookAt,
			newPosition + DefaultCubemapCameraStruct[faceIndex].Up
		};
	}
}

Float3 CreateCubemap::GetPosition()
{
	return position;
}


void CreateCubemap::SetBoxSize(Float3 newBoxSize)
{
	_boxSize = newBoxSize;
}


Float3 CreateCubemap::GetBoxSize()
{
	return _boxSize;
}


void CreateCubemap::GenAndCacheConvoluteSphereInputLayout(const DeviceManager &deviceManager, const Model *model)
{	
	ID3D11InputLayoutPtr _meshInputLayouts;
	VertexShaderPtr _sphereVertexShader;
	ID3D11DeviceContext *context = deviceManager.ImmediateContext();

	std::unordered_map<const Mesh *, ID3D11InputLayoutPtr>;

	for (uint64 i = 0; i < model->Meshes().size(); ++i)
	{
		const Mesh& mesh = model->Meshes()[i];

		DXCall(deviceManager.Device()->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
			_convoluteVS->ByteCode->GetBufferPointer(), _convoluteVS->ByteCode->GetBufferSize(), &inputLayout));

	}
}

void CreateCubemap::RenderPrefilterCubebox(const DeviceManager &deviceManager, const Float4x4 &sceneTransform)
{
	PIXEvent event(L"Prefilter Cubebox");
	ID3D11DeviceContext *context = deviceManager.ImmediateContext();

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	
	Model *cubemapSphere = new Model(); 

	cubemapSphere->CreateWithAssimp(deviceManager.Device(), L"..\\Content\\Models\\sphere\\sphere.obj", false);

	GenAndCacheConvoluteSphereInputLayout(deviceManager, cubemapSphere);


	SetViewport(context, CubemapWidth, CubemapHeight);

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		filterCamera.SetLookAt(DefaultCubemapCameraStruct[cubeboxFaceIndex].Eye,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].LookAt,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].Up);

		ID3D11RenderTargetViewPtr RTView = prefilterCubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = prefilterDepthTarget.ArraySlices.at(cubeboxFaceIndex);

		context->ClearRenderTargetView(RTView, clearColor);
		context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11RenderTargetView *renderTarget[1] = { RTView };
		context->OMSetRenderTargets(1, renderTarget, DSView);

		//Start Render
		//for (int mipLevel = 0; mipLevel < 8; ++mipLevel){
			_PSConstants.Data.MipLevel = 0;
			_PSConstants.Data.CameraPos = filterCamera.Position();

			// Set states
			float blendFactor[4] = { 1, 1, 1, 1 };
			context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
			context->OMSetDepthStencilState(_depthStencilStates.DepthEnabled(), 0);
			context->RSSetState(_rasterizerStates.NoCull());
			//context->RSSetState(_rasterizerStates.BackFaceCull());

			ID3D11SamplerState* sampStates[3] = {
				_samplerStates.Anisotropic(),
				_evsmSampler,
				_samplerStates.LinearClamp(),
			};

			context->PSSetSamplers(0, 3, sampStates);

			// Set shaders
			context->DSSetShader(nullptr, nullptr, 0);
			context->HSSetShader(nullptr, nullptr, 0);
			context->GSSetShader(nullptr, nullptr, 0);

			//Set Constant Variables
			_VSConstants.Data.world = sceneTransform;
			_VSConstants.Data.View = Float4x4::Transpose(filterCamera.ViewMatrix());
			_VSConstants.Data.WorldViewProjection = Float4x4::Transpose(sceneTransform * filterCamera.ProjectionMatrix());//meshRenderer->_meshVSConstants.Data.WorldViewProjection;
			_VSConstants.ApplyChanges(context);
			_VSConstants.SetVS(context, 0);

			for (uint64 meshIdx = 0; meshIdx < cubemapSphere->Meshes().size(); ++meshIdx)
			{
				const Mesh& mesh = cubemapSphere->Meshes()[meshIdx];

				context->VSSetShader(_convoluteVS, nullptr, 0);
				context->PSSetShader(_convolutePS[cubeboxFaceIndex], nullptr, 0);

				ID3D11Buffer *vertexBuffers[1] = { mesh.VertexBuffer() };
				UINT vertexStrides[1] = { mesh.VertexStride() };
				UINT offsets[1] = { 0 };
				context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
				context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
				context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				context->IASetInputLayout(inputLayout);

				for (uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
				{
					const MeshPart& part = mesh.MeshParts()[partIdx];
					const MeshMaterial& material = cubemapSphere->Materials()[part.MaterialIdx];

					//Set ShaderResourse
					ID3D11ShaderResourceView* cubemapResourse[1] = { cubemapTarget.SRView };

					context->PSSetShaderResources(0, _countof(cubemapResourse), cubemapResourse);
					context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
				}
			}
			/*meshRenderer->Render(context, filterCamera, sceneTransform, cubemapTarget.SRView, environmentMapSH, jitterOffset);
			skybox->RenderEnvironmentMap(context, cubemapTarget.SRView, filterCamera.ViewMatrix(),
				filterCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));*/

		//}
	}

	SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
	delete cubemapSphere;
}

void CreateCubemap::Create(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, 
	const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox)
{
	PIXEvent event(L"Render Cube Map");

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	ID3D11DeviceContext *context = deviceManager.ImmediateContext();

	SetViewport(context, CubemapWidth, CubemapHeight);

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++)
	{
		ID3D11RenderTargetViewPtr RTView = cubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);
		/*ID3D11RenderTargetViewPtr PreRTView = prefilterCubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr PreDSView = prefilterDepthTarget.ArraySlices.at(cubeboxFaceIndex);*/

		cubemapCamera.SetLookAt(CubemapCameraStruct[cubeboxFaceIndex].Eye,
			CubemapCameraStruct[cubeboxFaceIndex].LookAt,
			CubemapCameraStruct[cubeboxFaceIndex].Up);

		meshRenderer->SortSceneObjects(cubemapCamera.ViewMatrix());

		context->ClearRenderTargetView(RTView, clearColor);
		context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11RenderTargetView *renderTarget[1] = { RTView };
		context->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->RenderDepth(context, cubemapCamera, sceneTransform, false);

		meshRenderer->ComputeShadowDepthBoundsCPU(cubemapCamera);

		// TODO: figure out how to reduce depth on a cubemap
		// meshRenderer->ReduceDepth(context, cubemapDepthTarget, cubemapCamera);
		meshRenderer->RenderShadowMap(context, cubemapCamera, sceneTransform);

		context->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->Render(context, cubemapCamera, sceneTransform, environmentMap, environmentMapSH, jitterOffset);

		skybox->RenderEnvironmentMap(context, environmentMap, cubemapCamera.ViewMatrix(),
			cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));

		/*renderTarget[0] = PreRTView;
		context->OMSetRenderTargets(1, renderTarget, PreDSView);
		meshRenderer->Render(context, cubemapCamera, sceneTransform, cubemapTarget.SRView, environmentMapSH, jitterOffset);

		skybox->RenderEnvironmentMap(context, cubemapTarget.SRView, cubemapCamera.ViewMatrix(),
			cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));*/

		renderTarget[0] = nullptr;
		context->OMSetRenderTargets(1, renderTarget, nullptr);


		//Create pre-filiter cubebox from original cubebox.
		//RenderPrefilterCubebox(deviceManager, sceneTransform);
	}

	SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}


const PerspectiveCamera &CreateCubemap::GetCubemapCamera()
{
	const int face = 4;
	cubemapCamera.SetLookAt(DefaultCubemapCameraStruct[face].Eye,
		DefaultCubemapCameraStruct[face].LookAt,
		DefaultCubemapCameraStruct[face].Up);
	return cubemapCamera;
}

void CreateCubemap::GetTargetViews(RenderTarget2D &resCubemapTarget)
{
	resCubemapTarget = cubemapTarget;
}

void CreateCubemap::GetPreFilterTargetViews(RenderTarget2D &prefilterTarget)
{
	prefilterTarget = prefilterCubemapTarget;
}


CreateCubemap::~CreateCubemap()
{
}
