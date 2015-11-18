//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include <PCH.h>

#include "Realtime_GI.h"
#include "SharedConstants.h"

#include "resource.h"
#include <InterfacePointers.h>
#include <Window.h>
#include <Graphics\\DeviceManager.h>
#include <Input.h>
#include <Graphics\\SpriteRenderer.h>
#include <Graphics\\Model.h>
#include <Utility.h>
#include <Graphics\\Camera.h>
#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\Profiler.h>
#include <Graphics\\Textures.h>
#include <Graphics\\Sampling.h>

using namespace SampleFramework11;
using std::wstring;

const uint32 WindowWidth = 1280;
const uint32 WindowHeight = 720;
const float WindowWidthF = static_cast<float>(WindowWidth);
const float WindowHeightF = static_cast<float>(WindowHeight);

static const float NearClip = 0.01f;
static const float FarClip = 300.0f;


Realtime_GI::Realtime_GI() :  App(L"Realtime GI (CSCI 580)", MAKEINTRESOURCEW(IDI_DEFAULT)),
                            _camera(WindowWidthF / WindowHeightF, Pi_4 * 0.75f, NearClip, FarClip), 
							_prevForward(0.0f), _prevStrafe(0.0f), _prevAscend(0.0f), _numScenes(0),
							_cubemapGenerator(NearClip, FarClip)
{
    _deviceManager.SetBackBufferWidth(WindowWidth);
    _deviceManager.SetBackBufferHeight(WindowHeight);
    _deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);

    _window.SetClientArea(WindowWidth, WindowHeight);
}

void Realtime_GI::BeforeReset()
{
    App::BeforeReset();
}

void Realtime_GI::AfterReset()
{
	App::AfterReset();

	float aspect = static_cast<float>(_deviceManager.BackBufferWidth()) / _deviceManager.BackBufferHeight();
	_camera.SetAspectRatio(aspect);

	CreateRenderTargets();

	_meshRenderer.CreateReductionTargets(_deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());

	_postProcessor.AfterReset(_deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());
}

void Realtime_GI::LoadScenes(ID3D11DevicePtr device)
{
	// TODO: put the following in json
	static const wstring ModelPaths[uint64(Scenes::NumValues)] =
	{
		// L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaPBR_Textures\\Converted\\sponza.obj",
		//L"C:\\Users\\wqxho_000\\Downloads\\Cerberus_by_Andrew_Maximov\\Cerberus_by_Andrew_Maximov\\testfbxascii.fbx",
		L"..\\Content\\Models\\CornellBox\\CornellBox_fbx.FBX",
		// L"..\\Content\\Models\\CornellBox\\CornellBox_Max.obj",
		//L"C:\\Users\\wqxho_000\\Downloads\\SponzaPBR_Textures\\SponzaNon_PBR\\Converted\\sponza.obj",
		// L"..\\Content\\Models\\Powerplant\\Powerplant.sdkmesh",
		// L"..\\Content\\Models\\RoboHand\\RoboHand.meshdata",
		// L"",
	};

	Scene *scene = nullptr;


	/// Scene 1 /////////////////////////////////////////////////////////////
	_scenes[_numScenes] = Scene();
	scene = &_scenes[_numScenes];
	scene->Initialize(device);

	Model *m = scene->addModel(ModelPaths[0]);
	scene->addStaticOpaqueObject(m, 0.1f, Float3(0, 0, 0), Quaternion());
	_numScenes++;

	/// Scene 2 /////////////////////////////////////////////////////////////
	_scenes[_numScenes] = Scene();
	scene = &_scenes[_numScenes];
	scene->Initialize(device);

	scene->addDynamicOpaqueBoxObject(1.0f, Float3(-1.0f, 0.0f, 0.0f), Quaternion(0.0f, 1.0f, 0.0f, 0.3f));
	scene->addDynamicOpaqueBoxObject(1.5f, Float3(0.0f, 0.0f, 0.0f), Quaternion(-0.7f, 1.0f, 0.0f, 0.3f));
	scene->addDynamicOpaqueBoxObject(2.0f, Float3(1.0f, 0.0f, 0.0f), Quaternion(0.0f, 1.0f, 0.7f, 0.3f));
	_numScenes++;

	/// Scene 3 /////////////////////////////////////////////////////////////
	_scenes[_numScenes] = Scene();
	scene = &_scenes[_numScenes];
	scene->Initialize(device);

	scene->addDynamicOpaquePlaneObject(10.0f, Float3(0.0f, 0.0f, 0.0f), Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
	_numScenes++;
}

void Realtime_GI::LoadShaders(ID3D11DevicePtr device)
{
	// Load shaders
	for (uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
	{
		CompileOptions opts;
		opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
		_resolvePS[msaaMode] = CompilePSFromFile(device, L"Resolve.hlsl", "ResolvePS", "ps_5_0", opts);
	}

	_resolveVS = CompileVSFromFile(device, L"Resolve.hlsl", "ResolveVS");

	_backgroundVelocityVS = CompileVSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityVS");
	_backgroundVelocityPS = CompilePSFromFile(device, L"BackgroundVelocity.hlsl", "BackgroundVelocityPS");

}

void Realtime_GI::Initialize()
{
    App::Initialize();

    ID3D11DevicePtr device = _deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = _deviceManager.ImmediateContext();

    // Create a font + SpriteRenderer
    _font.Initialize(L"Arial", 18, SpriteFont::Regular, true, device);
    _spriteRenderer.Initialize(device);

    // Camera setup
    _camera.SetPosition(Float3(0.0f, 2.5f, -10.0f));

	LoadScenes(device);

    _meshRenderer.Initialize(device, _deviceManager.ImmediateContext());
    _meshRenderer.SetScene(&_scenes[0]);

    _skybox.Initialize(device);

    _envMap = LoadTexture(device, L"..\\Content\\EnvMaps\\Ennis.dds");

    FileReadSerializer serializer(L"..\\Content\\EnvMaps\\Ennis.shdata");
    SerializeItem(serializer, _envMapSH);

	LoadShaders(device);
   
    _resolveConstants.Initialize(device);
    _backgroundVelocityConstants.Initialize(device);

    // Init the post processor
    _postProcessor.Initialize(device);

}

// Creates all required render targets
void Realtime_GI::CreateRenderTargets()
{
    ID3D11Device* device = _deviceManager.Device();
    uint32 width = _deviceManager.BackBufferWidth();
    uint32 height = _deviceManager.BackBufferHeight();

    const uint32 NumSamples = AppSettings::NumMSAASamples();
    const uint32 Quality = NumSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    _colorTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples, Quality);
    _depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D32_FLOAT, true, NumSamples, Quality);
    _velocityTarget.Initialize(device, width, height, DXGI_FORMAT_R16G16_FLOAT, true, NumSamples, Quality);

	// TODO: remove multi sample for cubemap
	if (_firstFrame)
	{
		_cubemapGenerator.Initialize(device, 1, NumSamples, Quality);
	}

    if(_resolveTarget.Width != width || _resolveTarget.Height != height)
    {
        _resolveTarget.Initialize(device, width, height, _colorTarget.Format);
        _velocityResolveTarget.Initialize(device, width, height, _velocityTarget.Format);
        _prevFrameTarget.Initialize(device, width, height, _colorTarget.Format);
    }
}

void Realtime_GI::ApplyMomentum(float &prevVal, float &val, float deltaTime)
{
	float blendedValue;
	if (fabs(val) > fabs(prevVal))
		blendedValue = Lerp(val, prevVal, pow(0.6f, deltaTime * 60.0f));
	else
		blendedValue = Lerp(val, prevVal, pow(0.8f, deltaTime * 60.0f));
	prevVal = blendedValue;
	val = blendedValue;
}

void Realtime_GI::Update(const Timer& timer)
{
    AppSettings::UpdateUI();

    MouseState mouseState = MouseState::GetMouseState(_window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(_window);

    if(kbState.IsKeyDown(KeyboardState::Escape))
        _window.Destroy();

	float deltaSec = timer.DeltaSecondsF();;
	float CamMoveSpeed = 5.0f * deltaSec;
	const float CamRotSpeed = 0.180f * deltaSec;
	const float MeshRotSpeed = 0.180f * deltaSec;

    // Move the camera with keyboard input
    if(kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

	float forward = CamMoveSpeed *
		((kbState.IsKeyDown(KeyboardState::W) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::S) ? -1 : 0.0f));

	float strafe = CamMoveSpeed *
		((kbState.IsKeyDown(KeyboardState::A) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::D) ? -1 : 0.0f));

	float ascend = CamMoveSpeed * 
		((kbState.IsKeyDown(KeyboardState::Q) ? 1 : 0.0f) +
		(kbState.IsKeyDown(KeyboardState::E) ? -1 : 0.0f));

    Float3 camPos = _camera.Position();

	ApplyMomentum(_prevForward, forward, deltaSec);
	ApplyMomentum(_prevStrafe, strafe, deltaSec);
	ApplyMomentum(_prevAscend, ascend, deltaSec);

	camPos += _camera.Forward() * forward;
	camPos += _camera.Left() * strafe;
	camPos += _camera.Up() * ascend;

    _camera.SetPosition(camPos);

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = _camera.XRotation();
        float yRot = _camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        _camera.SetXRotation(xRot);
        _camera.SetYRotation(yRot);
    }

    // Reset the camera projection
    _camera.SetAspectRatio(_camera.AspectRatio());
    Float2 jitter = 0.0f;
    if (AppSettings::EnableTemporalAA 
	 && AppSettings::EnableJitter() 
	 && AppSettings::UseStandardResolve == false)
    {
        const float jitterScale = 0.5f;

        if(AppSettings::JitterMode == JitterModes::Uniform2x)
        {
            jitter = _frameCount % 2 == 0 ? -0.5f : 0.5f;
        }
        else if(AppSettings::JitterMode == JitterModes::Hammersly16)
        {
            uint64 idx = _frameCount % 16;
            jitter = Hammersley2D(idx, 16) - Float2(0.5f);
        }

        jitter *= jitterScale;

        const float offsetX = jitter.x * (1.0f / _colorTarget.Width);
        const float offsetY = jitter.y * (1.0f / _colorTarget.Height);
        Float4x4 offsetMatrix = Float4x4::TranslationMatrix(Float3(offsetX, -offsetY, 0.0f));
        _camera.SetProjection(_camera.ProjectionMatrix() * offsetMatrix);
    }

    _jitterOffset = (jitter - _prevJitter) * 0.5f;
    _prevJitter = jitter;

	// Set centroid sampling mode - change shader bindings
	if (AppSettings::CentroidSampling.Changed())
	{
		_meshRenderer.ReMapMeshShaders();
	}

    // Toggle VSYNC
    if(kbState.RisingEdge(KeyboardState::V))
        _deviceManager.SetVSYNCEnabled(!_deviceManager.VSYNCEnabled());

    _deviceManager.SetNumVSYNCIntervals(AppSettings::DoubleSyncInterval ? 2 : 1);

    if(AppSettings::CurrentScene.Changed())
    {
		Scene *currScene = &_scenes[AppSettings::CurrentScene];
		_meshRenderer.SetScene(currScene);
        AppSettings::SceneOrientation.SetValue(currScene->getSceneOrientation());
    }

	Quaternion orientation = AppSettings::SceneOrientation;
	orientation = orientation * Quaternion::FromAxisAngle(Float3(0.0f, 1.0f, 0.0f), 
		AppSettings::ModelRotationSpeed * timer.DeltaSecondsF());

	AppSettings::SceneOrientation.SetValue(orientation);
	_globalTransform = orientation.ToFloat4x4() *
		Float4x4::ScaleMatrix(_scenes[AppSettings::CurrentScene].getSceneScale()) *
		Float4x4::TranslationMatrix(_scenes[AppSettings::CurrentScene].getSceneTranslation());
}

void Realtime_GI::RenderAA()
{
    PIXEvent pixEvent(L"MSAA Resolve + Temporal AA");

    ID3D11DeviceContext* context = _deviceManager.ImmediateContext();

    ID3D11ShaderResourceView* velocitySRV = _velocityTarget.SRView;
    if(AppSettings::NumMSAASamples() > 1)
    {
        context->ResolveSubresource(_velocityResolveTarget.Texture, 0, _velocityTarget.Texture, 0, _velocityTarget.Format);
        velocitySRV = _velocityResolveTarget.SRView;
    }


    ID3D11RenderTargetView* rtvs[1] = { _resolveTarget.RTView };

    context->OMSetRenderTargets(1, rtvs, nullptr);

    if(AppSettings::UseStandardResolve)
    {
        if(AppSettings::MSAAMode == 0)
            context->CopyResource(_resolveTarget.Texture, _colorTarget.Texture);
        else
            context->ResolveSubresource(_resolveTarget.Texture, 0, _colorTarget.Texture, 0, _colorTarget.Format);
        return;
    }

    const uint32 SampleRadius = static_cast<uint32>((AppSettings::FilterSize / 2.0f) + 0.499f);
    ID3D11PixelShader* pixelShader = _resolvePS[AppSettings::MSAAMode];
    context->PSSetShader(pixelShader, nullptr, 0);
    context->VSSetShader(_resolveVS, nullptr, 0);

    _resolveConstants.Data.TextureSize = Float2(static_cast<float>(_colorTarget.Width), 
		static_cast<float>(_colorTarget.Height));
    _resolveConstants.Data.SampleRadius = SampleRadius;;
    _resolveConstants.ApplyChanges(context);
    _resolveConstants.SetPS(context, 0);

    ID3D11ShaderResourceView* srvs[3] = { _colorTarget.SRView, _prevFrameTarget.SRView, velocitySRV };
    context->PSSetShaderResources(0, 3, srvs);

    ID3D11SamplerState* samplers[1] = { _samplerStates.LinearClamp() };
    context->PSSetSamplers(0, 1, samplers);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);

    srvs[0] = srvs[1] = srvs[2] = nullptr;
    context->PSSetShaderResources(0, 3, srvs);

    context->CopyResource(_prevFrameTarget.Texture, _resolveTarget.Texture);
}

void Realtime_GI::RenderSceneCubemaps()
{
	_meshRenderer.SetCubemapCapture(true);

	// TODO: make a separate cubemap manager to set different locations
	_cubemapGenerator.SetPosition(float3(0.0f, 0.0f, 0.0f));
	_cubemapGenerator.Create(_deviceManager, &_meshRenderer, _globalTransform, _envMap, _envMapSH, _jitterOffset, &_skybox);
	_meshRenderer.SetCubemapCapture(false);
}

void Realtime_GI::Render(const Timer& timer)
{
    if(AppSettings::MSAAMode.Changed())
        CreateRenderTargets();

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();

    AppSettings::UpdateCBuffer(context);

	if (_firstFrame || AppSettings::CurrentScene.Changed() || AppSettings::EnableRealtimeCubemap)
	{
		// render cubemap every time scene has changed
		RenderSceneCubemaps();
	}

	// _scenes[AppSettings::CurrentScene].sortSceneObjects(_camera.ViewMatrix());

    RenderScene();

    RenderBackgroundVelocity();

    RenderAA();

    {
        // Kick off post-processing
        PIXEvent pixEvent(L"Post Processing");
        _postProcessor.Render(context, _resolveTarget.SRView, _deviceManager.BackBuffer(), timer.DeltaSecondsF());
    }

    ID3D11RenderTargetView* renderTargets[1] = { _deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    SetViewport(context, _deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());
    RenderHUD();

    ++_frameCount; 
	_firstFrame = false;
}

void Realtime_GI::RenderScene()
{
    PIXEvent event(L"Render Scene");

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();
	RenderTarget2D cubemapRenderTarget;

    SetViewport(context, _colorTarget.Width, _colorTarget.Height);

	_cubemapGenerator.GetTargetViews(cubemapRenderTarget);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(_colorTarget.RTView, clearColor);
    context->ClearRenderTargetView(_velocityTarget.RTView, clearColor);
    context->ClearDepthStencilView(_depthBuffer.DSView, D3D11_CLEAR_DEPTH, 1.0f, 0);

    ID3D11RenderTargetView* renderTargets[2] = { nullptr, nullptr };
    context->OMSetRenderTargets(1, renderTargets, _depthBuffer.DSView);
	
	// TODO: reduce api calls
    _meshRenderer.RenderDepth(context, _camera, _globalTransform, false);

    _meshRenderer.ReduceDepth(context, _depthBuffer, _camera);
    _meshRenderer.RenderShadowMap(context, _camera, _globalTransform);

    renderTargets[0] = _colorTarget.RTView;
    renderTargets[1] = _velocityTarget.RTView;
    context->OMSetRenderTargets(2, renderTargets, _depthBuffer.DSView);

    // _meshRenderer.Render(context, _camera, _globalTransform, _envMap, _envMapSH, _jitterOffset);
	_meshRenderer.Render(context, _camera, _globalTransform, cubemapRenderTarget.SRView, _envMapSH, _jitterOffset);

    renderTargets[0] = _colorTarget.RTView;
    renderTargets[1] = nullptr;
	context->OMSetRenderTargets(2, renderTargets, _depthBuffer.DSView);

    if(AppSettings::RenderBackground)
        _skybox.RenderEnvironmentMap(context, _envMap, _camera.ViewMatrix(), _camera.ProjectionMatrix(),
		Float3(std::exp2(AppSettings::ExposureScale)));

    renderTargets[0] = renderTargets[1] = nullptr;
    context->OMSetRenderTargets(2, renderTargets, nullptr);
}

void Realtime_GI::RenderBackgroundVelocity()
{
    PIXEvent pixEvent(L"Render Background Velocity");

    ID3D11DeviceContextPtr context = _deviceManager.ImmediateContext();

    SetViewport(context, _velocityTarget.Width, _velocityTarget.Height);

    // Don't use camera translation for background velocity
    FirstPersonCamera tempCamera = _camera;

    _backgroundVelocityConstants.Data.InvViewProjection = Float4x4::Transpose(Float4x4::Invert(tempCamera.ViewProjectionMatrix()));
    _backgroundVelocityConstants.Data.PrevViewProjection = Float4x4::Transpose(_prevViewProjection);
    _backgroundVelocityConstants.Data.RTSize.x = float(_velocityTarget.Width);
    _backgroundVelocityConstants.Data.RTSize.y = float(_velocityTarget.Height);
    _backgroundVelocityConstants.Data.JitterOffset = _jitterOffset;
    _backgroundVelocityConstants.ApplyChanges(context);
    _backgroundVelocityConstants.SetPS(context, 0);

    _prevViewProjection = tempCamera.ViewProjectionMatrix();

    float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(_depthStencilStates.DepthEnabled(), 0);
    context->RSSetState(_rasterizerStates.NoCull());

    ID3D11RenderTargetView* rtvs[1] = { _velocityTarget.RTView };
    context->OMSetRenderTargets(1, rtvs, _depthBuffer.DSView);

    context->VSSetShader(_backgroundVelocityVS, nullptr, 0);
    context->PSSetShader(_backgroundVelocityPS, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);
}

void Realtime_GI::RenderHUD()
{
    PIXEvent pixEvent(L"HUD Pass");

    _spriteRenderer.Begin(_deviceManager.ImmediateContext(), SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText(L"FPS: ");
    fpsText += ToString(_fps) + L" (" + ToString(1000.0f / _fps) + L"ms)";
    _spriteRenderer.RenderText(_font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    transform._42 += 25.0f;
    wstring vsyncText(L"VSYNC (V): ");
    vsyncText += _deviceManager.VSYNCEnabled() ? L"Enabled" : L"Disabled";
    _spriteRenderer.RenderText(_font, vsyncText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    Profiler::GlobalProfiler.EndFrame(_spriteRenderer, _font);

    _spriteRenderer.End();
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Realtime_GI app;
    app.Run();
}