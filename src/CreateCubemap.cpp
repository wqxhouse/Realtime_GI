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


VOID CreateCubemap::Create(){

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


CreateCubemap::~CreateCubemap()
{
}
