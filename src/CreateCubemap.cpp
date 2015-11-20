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

void CreateCubemap::Initialize(ID3D11Device *device, uint32 numMipLevels, uint32 multiSamples, uint32 msQuality)
{
	cubemapTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_R16G16B16A16_FLOAT, 7,
		1, 0, TRUE, FALSE, 6, TRUE);
	cubemapDepthTarget.Initialize(device, CubemapWidth, CubemapHeight, DXGI_FORMAT_D32_FLOAT, numMipLevels, 1,
		0, 6, TRUE);

	_convoluteVS = CompileVSFromFile(device, L"PrefilterCubemap.hlsl", "VS", "vs_5_0");
	_convolutePS = CompilePSFromFile(device, L"PrefilterCubemap.hlsl", "PS", "ps_5_0");

	_VSConstant.Initialize(device);//Initialize original vertex constant variables
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

void CreateCubemap::GenAndCacheConvoluteSphereInputLayout(const DeviceManager &deviceManager, const Model *model)
{	
	ID3D11InputLayoutPtr _meshInputLayouts;
	VertexShaderPtr _sphereVertexShader;
	std::unordered_map<const Mesh *, ID3D11InputLayoutPtr>;


	for (uint64 i = 0; i < model->Meshes().size(); ++i)
	{
		const Mesh &mesh = model->Meshes()[i];
	}

	for (uint64 i = 0; i < model->Meshes().size(); ++i)
	{
		const Mesh& mesh = model->Meshes()[i];

		ID3D11InputLayoutPtr inputLayout;

		DXCall(deviceManager.Device()->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
			_convoluteVS->ByteCode->GetBufferPointer(), _convoluteVS->ByteCode->GetBufferSize(), &inputLayout));

	}
}

void CreateCubemap::RenderPrefilterCubebox(const DeviceManager &deviceManager, const Float4x4 &sceneTransform)
{
	Model *cubemapSphere = new Model();

	cubemapSphere->CreateWithAssimp(deviceManager.Device(), L"..\\Content\\Models\\sphere\\sphere.obj", false);

	GenAndCacheConvoluteSphereInputLayout(deviceManager, cubemapSphere);

	//Set Constant Variables
	_VSConstant.Data.world = sceneTransform;

	//Set ShaderResourse
	ID3D11ShaderResourceView* cubemapResourse[] = { cubemapTarget.SRView };

	deviceManager.ImmediateContext()->CSSetShaderResources(0, _countof(cubemapResourse), cubemapResourse);

	delete cubemapSphere;
}

void CreateCubemap::Create(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
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
		
		//Create pre-filiter cubebox from original cubebox.
		RenderPrefilterCubebox(deviceManager, sceneTransform);

		renderTarget[0] = nullptr;
		context->OMSetRenderTargets(1, renderTarget, nullptr);
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


CreateCubemap::~CreateCubemap()
{
}
