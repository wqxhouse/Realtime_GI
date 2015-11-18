//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include <PCH.h>

#include <App.h>
#include <InterfacePointers.h>
#include <Graphics\\Camera.h>
#include <Graphics\\Model.h>
#include <Graphics\\SpriteFont.h>
#include <Graphics\\SpriteRenderer.h>
#include <Graphics\\Skybox.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\ShaderCompilation.h>

#include "PostProcessor.h"
#include "MeshRenderer.h"
#include "Scene.h"

#include "CreateCubemap.h"

using namespace SampleFramework11;

class Realtime_GI : public App
{

protected:

	// Caution: too large will stack overflow
	static const uint32 MAX_SCENES = 5;

    FirstPersonCamera _camera;

    SpriteFont _font;
    SampleFramework11::SpriteRenderer _spriteRenderer;
    Skybox _skybox;

    PostProcessor _postProcessor;
    DepthStencilBuffer _depthBuffer;
    RenderTarget2D _colorTarget;
    RenderTarget2D _resolveTarget;
    RenderTarget2D _prevFrameTarget;
    RenderTarget2D _velocityTarget;
    RenderTarget2D _velocityResolveTarget;
    uint64 _frameCount = 0;
	bool32 _firstFrame = 1;

	Scene _scenes[MAX_SCENES];
	uint32 _numScenes;

    MeshRenderer _meshRenderer;

    Float4x4 _globalTransform;

    ID3D11ShaderResourceViewPtr _envMap;
    SH9Color _envMapSH;

    VertexShaderPtr _resolveVS;
    PixelShaderPtr _resolvePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr _backgroundVelocityVS;
    PixelShaderPtr _backgroundVelocityPS;
    Float4x4 _prevViewProjection;

    Float2 _jitterOffset;
    Float2 _prevJitter;

	// Camera momentum
	float _prevForward;
	float _prevStrafe;
	float _prevAscend;

    struct ResolveConstants
    {
        uint32 SampleRadius;
        Float2 TextureSize;
    };

    struct BackgroundVelocityConstants
    {
        Float4x4 InvViewProjection;
        Float4x4 PrevViewProjection;
        Float2 RTSize;
        Float2 JitterOffset;
    };

    ConstantBuffer<ResolveConstants> _resolveConstants;
    ConstantBuffer<BackgroundVelocityConstants> _backgroundVelocityConstants;

    virtual void Initialize() override;
	void LoadScenes(ID3D11DevicePtr device);
	void LoadShaders(ID3D11DevicePtr device);

    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();

    void RenderScene();
	void RenderSceneCubemaps();
    void RenderBackgroundVelocity();
    void RenderAA();
    void RenderHUD();

	void ApplyMomentum(float &prevVal, float &val, float deltaTime);

	//Create Cubemap
	CreateCubemap _cubemapGenerator;
public:

    Realtime_GI();
};
