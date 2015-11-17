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

#include "CreateCubemap.h"

using namespace SampleFramework11;

class Realtime_GI : public App
{

protected:

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

    // Model
    Model _models[uint64(Scenes::NumValues)];
    MeshRenderer _meshRenderer;

    Float4x4 _modelTransform;
    Quaternion _modelOrientations[uint64(Scenes::NumValues)];

    ID3D11ShaderResourceViewPtr _envMap;
    SH9Color _envMapSH;

    VertexShaderPtr _resolveVS;
    PixelShaderPtr _resolvePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr _backgroundVelocityVS;
    PixelShaderPtr _backgroundVelocityPS;
    Float4x4 _prevViewProjection;

    Float2 _jitterOffset;
    Float2 _prevJitter;

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
    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();

    void RenderScene();
    void RenderBackgroundVelocity();
    void RenderAA();
    void RenderHUD();

	//Create Cubemap
	CreateCubemap createCubeMap;
public:

    Realtime_GI();
};
