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
#include "LightClusters.h"

#include "CreateCubemap.h"
#include "IrradianceVolume.h"
#include "DebugRenderer.h"

using namespace SampleFramework11;
class Realtime_GI : public App
{
public:
	void AddScene(SceneScript *sceneScript);

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

	// Deferred - GBuffer
	RenderTarget2D _rt0Target;  // albedo 
	RenderTarget2D _rt1Target;	// roughness/metallic/emissive
	RenderTarget2D _rt2Target;  // compressed normal + velocity

    uint64 _frameCount = 0;
	bool32 _firstFrame = 1;

	Scene _scenes[MAX_SCENES];
	uint32 _numScenes;
	Scene *_prevScene;

    MeshRenderer _meshRenderer;

    Float4x4 _globalTransform;

    ID3D11ShaderResourceViewPtr _envMap;
    SH9Color _envMapSH;

    VertexShaderPtr _resolveVS;
    PixelShaderPtr _resolvePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr _backgroundVelocityVS;
    PixelShaderPtr _backgroundVelocityPS;
    Float4x4 _prevViewProjection;

	VertexShaderPtr _clusteredDeferredVS;
	PixelShaderPtr _clusteredDeferredPS;

    Float2 _jitterOffset;
    Float2 _prevJitter;

	// Full screen quad
	ID3D11BufferPtr _quadVB;
	ID3D11BufferPtr _quadIB;
	ID3D11InputLayoutPtr _quadInputLayout;

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

	static const int NumCascades = 4;
	struct DeferredPassConstants
	{
		Float4Align Float4x4 ShadowMatrix;
		Float4Align float CascadeSplits[NumCascades];
		Float4Align Float4 CascadeOffsets[NumCascades];
		Float4Align Float4 CascadeScales[NumCascades];

		float OffsetScale;
		float PositiveExponent;
		float NegativeExponent;
		float LightBleedingReduction;

		Float4Align Float4x4 ProjectionToWorld;
		Float4Align Float4x4 ViewToWorld;
		Float4Align ShaderSH9Color EnvironmentSH;

		Float3 CameraPosWS;
		float NearPlane;
		Float3 CameraZAxisWS;
		float FarPlane;

		Float3 ClusterScale;
		float ProjTermA;
		Float3 ClusterBias;
		float ProjTermB;
	};


    ConstantBuffer<ResolveConstants> _resolveConstants;
    ConstantBuffer<BackgroundVelocityConstants> _backgroundVelocityConstants;
	ConstantBuffer<DeferredPassConstants> _deferredPassConstants;

	StructuredBuffer _pointLightBuffer;
	StructuredBuffer _shProbeLightBuffer;

	SamplerStates _samplerStates;

	// Defined outside of the class;
	void LoadScenes();

    virtual void Initialize() override;
	void LoadShaders(ID3D11DevicePtr device);

    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();
	void CreateLightBuffers();
	void CreateQuadBuffers();

    void RenderSceneForward();
	void RenderSceneCubemaps();
    void RenderBackgroundVelocity();
    void RenderAA();
    void RenderHUD();

	void RenderSceneGBuffer();
	void RenderLightsDeferred();

	void UploadLights();
	void AssignLightAndUploadClusters();

	void ApplyMomentum(float &prevVal, float &val, float deltaTime);
	void QueueDebugCommands();


	CreateCubemap _cubemapGenerator;
	DebugRenderer _debugRenderer;
	LightClusters _lightClusters;
	IrradianceVolume _irradianceVolume;

public:

    Realtime_GI();
};
