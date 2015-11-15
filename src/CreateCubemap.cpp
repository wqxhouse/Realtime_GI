#include "CreateCubemap.h"

const Float3 CreateCubemap::DefaultPosition = { 0.0f, 0.0f, 0.0f };

const CreateCubemap::CameraStruct CreateCubemap::DefaultCubemapCameraStruct[6] = {
		//{ DefaultPosition, Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) }, //Center
		//{ DefaultPosition, Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) }, //Left
		//{ DefaultPosition, Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Back
		//{ DefaultPosition, Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Right
		//{ DefaultPosition, Float3(0.0f, 0.0f, 1.0f), Float3(-1.0f, 0.0f, 0.0f) }, //Up
		//{ DefaultPosition, Float3(0.0f, 0.0f, -1.0f), Float3(1.0f, 0.0f, 0.0f) }  //Down
		{ DefaultPosition, Float3(0.0f, 0.0f, 1.0f), Float3(0.0f, 1.0f, 0.0f) }, //Center
		{ DefaultPosition, Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Left
		{ DefaultPosition, Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, 1.0f, 0.0f) },//Back
		{ DefaultPosition, Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 1.0f, 0.0f) },//Right
		{ DefaultPosition, Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) },//Up
		{ DefaultPosition, Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }//Down
};

CreateCubemap::CreateCubemap(const float NearClip, const float FarClip) 
	: cubemapCamera(1.0f, 90.0f * (Pi / 180), NearClip, FarClip)
{
	cubemapCamera.SetFieldOfView(90.0f * (Pi / 180));
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


VOID CreateCubemap::Create(ID3D11DeviceContext *context, MeshRenderer *meshRenderer, 
	RenderTarget2D velocityTarget, Float4x4 modelTransform,
	ID3D11ShaderResourceView* environmentMap,
	SH9Color environmentMapSH, Float2 jitterOffset, Skybox skybox){

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	//ID3D11RenderTargetView *renderTarget[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, };

	for (int cubeboxFaceIndex = 0; cubeboxFaceIndex < 6; cubeboxFaceIndex++){
		//int cubeboxFaceIndex = 0;
		ID3D11RenderTargetViewPtr RTView = cubemapTarget.RTVArraySlices.at(cubeboxFaceIndex);
		ID3D11DepthStencilViewPtr DSView = cubemapDepthTarget.ArraySlices.at(cubeboxFaceIndex);
		cubemapCamera.SetLookAt(DefaultCubemapCameraStruct[cubeboxFaceIndex].Eye,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].LookAt,
			DefaultCubemapCameraStruct[cubeboxFaceIndex].Up);

		context->ClearRenderTargetView(RTView, clearColor);
		context->ClearDepthStencilView(DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

		//renderTarget[cubeboxFaceIndex] = RTView;
		ID3D11RenderTargetView *renderTarget[2] = { RTView, nullptr };
		context->OMSetRenderTargets(1, renderTarget, DSView);
		meshRenderer->RenderDepth(context, cubemapCamera, modelTransform, false);

		renderTarget[0] = RTView;
		renderTarget[1] = velocityTarget.RTView;
		//context->OMSetRenderTargets(2, renderTarget, DSView);
		//meshRenderer->Render(context, cubemapCamera, modelTransform, environmentMap, environmentMapSH, jitterOffset);

		if (AppSettings::RenderBackground){
			skybox.RenderEnvironmentMap(context, environmentMap, cubemapCamera.ViewMatrix(),
				cubemapCamera.ProjectionMatrix(), Float3(std::exp2(AppSettings::ExposureScale)));
		}
		renderTarget[0] = nullptr;
		context->OMSetRenderTargets(1, renderTarget, nullptr);
	}
}


CONST PerspectiveCamera &CreateCubemap::GetCubemapCamera(){
	const int face = 4;
	cubemapCamera.SetLookAt(DefaultCubemapCameraStruct[face].Eye,
		DefaultCubemapCameraStruct[face].LookAt,
		DefaultCubemapCameraStruct[face].Up);
	/*cubemapCamera.SetLookAt(DefaultPosition,
		Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f));*/
	return cubemapCamera;
}

VOID CreateCubemap::GetTargetViews(RenderTarget2D &resCubemapTarget, DepthStencilBuffer &resCubemapDepthTarget){
	resCubemapTarget = cubemapTarget;
	resCubemapDepthTarget = cubemapDepthTarget;
}


CreateCubemap::~CreateCubemap()
{
}
