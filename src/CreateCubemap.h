/*
	Author : bxs3514
	Date   : 2015.11
	Cubemap generator
*/

#pragma once

#include <App.h>
#include <PCH.h>
#include <Utility.h>
#include <AppSettings.h>
#include <Graphics\\Camera.h>
#include <Graphics\\Skybox.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\Model.h>
#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\SH.h>

using namespace SampleFramework11;
class MeshRenderer;
class CreateCubemap
{

public:
	CreateCubemap();
	~CreateCubemap();

	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context);
	
	void SetPosition(const Float3 &position);
	Float3 GetPosition();
	void SetBoxSize(const Float3 &boxSize);
	Float3 GetBoxSize();
	void GetTargetViews(RenderTarget2D &resCubemapTarget);
	void GetPreFilterRT(RenderTarget2D &prefilterTarget);

	void Create(MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox);
	void RenderPrefilterCubebox(const Float4x4 &sceneTransform);

	void SetCubemapSize(uint32 size);
	void SetClipPlane(float nearPlane, float farPlane);

	const PerspectiveCamera &GetCubemapCamera();

private:
	uint32 _CubemapWidth;
	uint32 _CubemapHeight;

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	PerspectiveCamera _cubemapCamera;
	PerspectiveCamera _filterCamera;
	RenderTarget2D _cubemapTarget;
	DepthStencilBuffer _cubemapDepthTarget;
	RenderTarget2D _prefilterCubemapTarget;
	DepthStencilBuffer _prefilterDepthTarget;
	Float3 _position;

	Float3 _boxSize;



	BlendStates _blendStates;
	DepthStencilStates _depthStencilStates;
	RasterizerStates _rasterizerStates;

	SamplerStates _samplerStates;
	ID3D11SamplerStatePtr _evsmSampler;

	CompileOptions opts;

	struct CameraStruct{
		Float3 Eye;
		Float3 LookAt;
		Float3 Up;
	} CubemapCameraStruct[6];

	static const CameraStruct DefaultCubemapCameraStruct[6];
	static const Float3 DefaultPosition;

	struct VSConstant
	{
		Float4x4 world;
		Float4Align Float4x4 View;
		Float4Align Float4x4 WorldViewProjection;
	};

	struct PSConstant
	{
		Float4 CameraPos;
		uint32 MipLevel;
	};

	static const uint32 ShadowAnisotropy = 16;

	ConstantBuffer<VSConstant> _VSConstants;
	ConstantBuffer<PSConstant> _PSConstants;

	float _nearPlane;
	float _farPlane;

	void GenAndCacheConvoluteSphereInputLayout();
	void GenFilterShader();

	static Model _convolveSphere;
	static bool32 _convolveSphereSet;

	static VertexShaderPtr _convoluteVS;
	static PixelShaderPtr _convolutePS[6];

	static ID3D11InputLayoutPtr _inputLayout;
};

