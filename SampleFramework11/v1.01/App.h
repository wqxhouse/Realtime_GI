//=================================================================================================
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#pragma once

#include "PCH.h"

#include "Window.h"
#include "Graphics\\DeviceManager.h"
#include "Graphics\\DeviceStates.h"
#include "Timer.h"
#include "Graphics\\SpriteFont.h"
#include "Graphics\\SpriteRenderer.h"

namespace SampleFramework11
{

class App
{

public:

    App(const wchar* appName, const wchar* iconResource = NULL);
    virtual ~App();

    int32 Run();

    void RenderText(const std::wstring& text, Float2 pos);
    void RenderCenteredText(const std::wstring& text);

protected:

    virtual void Initialize();
    virtual void Update(const Timer& timer) = 0;
    virtual void Render(const Timer& timer) = 0;

    virtual void BeforeReset();
    virtual void AfterReset();

    void Exit();
    void ToggleFullScreen(bool fullScreen);
    void CalculateFPS();

    static LRESULT OnWindowResized(void* context, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    Window _window;
    DeviceManager _deviceManager;
    Timer _timer;

    BlendStates _blendStates;
    RasterizerStates _rasterizerStates;
    DepthStencilStates _depthStencilStates;
    SamplerStates _samplerStates;

    SpriteFont _font;
    SpriteRenderer _spriteRenderer;

    static const uint32 NumTimeDeltaSamples = 64;
    float timeDeltaBuffer[NumTimeDeltaSamples];
    uint32 _currentTimeDeltaSample;
    uint32 _fps;

    TwBar* _tweakBar;

    std::wstring _applicationName;

    bool _createConsole;
    bool _showWindow;
    int32 _returnCode;

public:

    // Accessors
    Window& Window() { return _window; }
    DeviceManager& DeviceManager() { return _deviceManager; }
    SpriteFont& Font() { return _font; }
    SpriteRenderer& SpriteRenderer() { return _spriteRenderer; }
    TwBar* TweakBar() { return _tweakBar; }
};

extern App* GlobalApp;

}