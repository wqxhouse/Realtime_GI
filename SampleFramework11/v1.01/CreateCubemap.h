/*
	Author : bxs3514
	Date   : 2015.11
*/

#pragma once

#include <PCH.h>
#include <Graphics\Camera.h>

using namespace SampleFramework11;

class CreateCubemap
{
private:
	PerspectiveCamera cubemapCamera;//Camera used to catch scenes in 6 directions
	D3D11_TEXTURE2D_DESC cubemapTex;
	D3D11_SUBRESOURCE_DATA cubemapData;
	std::vector<XMFLOAT4> cubemapColor[6];//RGBA color buffer of cubemap faces
	Float3 Position;
public:
	VOID Initialize();
	VOID SetPosition(Float3 position);
	CONST PerspectiveCamera &GetFirstPersonCamera();
	VOID Create();
	~CreateCubemap();
};

