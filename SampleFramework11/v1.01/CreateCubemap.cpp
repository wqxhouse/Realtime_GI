#include "CreateCubemap.h"

const Float3 CreateCubemap::DefaultPosition = { 0.0f, 0.0f, 0.0f };

const CreateCubemap::CameraStruct CreateCubemap::DefaultCubemapCameraStruct[6] = {
		{ DefaultPosition, Float3(1.0f, 0.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Center
		{ DefaultPosition, Float3(0.0f, 1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Left
		{ DefaultPosition, Float3(-1.0f, 0.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Back
		{ DefaultPosition, Float3(0.0f, -1.0f, 0.0f), Float3(0.0f, 0.0f, 1.0f) }, //Right
		{ DefaultPosition, Float3(0.0f, 0.0f, 1.0f), Float3(-1.0f, 0.0f, 0.0f) }, //Up
		{ DefaultPosition, Float3(0.0f, 0.0f, -1.0f), Float3(1.0f, 0.0f, 0.0f) }  //Down
};

CreateCubemap::CreateCubemap(const float NearClip, const float FarClip) 
	: cubemapCamera(1.0f, 90.0f, NearClip, FarClip)
{
	cubemapCamera.SetFieldOfView(90.0f);
}

VOID CreateCubemap::Initialize(){

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
	return cubemapCamera;
}


CreateCubemap::~CreateCubemap()
{
}
