
//
//  MJP's DX11 Sample Framework
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "App.h"
#include "Exceptions.h"
#include "Graphics\\Profiler.h"
#include "SF11_Math.h"
#include "FileIO.h"
#include "Settings.h"
#include "TwHelper.h"

// AppSettings framework
namespace AppSettings
{
    void Initialize(ID3D11Device* device);
    void Update();
    void UpdateCBuffer(ID3D11DeviceContext* context);
}

namespace SampleFramework11
{

App* GlobalApp = nullptr;

App::App(const wchar* appName, const wchar* iconResource) :   _window(NULL, appName, WS_OVERLAPPEDWINDOW,
                                                                     WS_EX_APPWINDOW, 1280, 720, iconResource, iconResource),
                                                              _currentTimeDeltaSample(0),
                                                              _fps(0), _tweakBar(nullptr), _applicationName(appName),
                                                              _createConsole(true), _showWindow(true), _returnCode(0)
{
    GlobalApp = this;
    for(uint32 i = 0; i < NumTimeDeltaSamples; ++i)
        timeDeltaBuffer[i] = 0;
}

App::~App()
{

}

int32 App::Run()
{
    try
    {
        if(_createConsole)
        {
            Win32Call(AllocConsole());
            Win32Call(SetConsoleTitle(_applicationName.c_str()));
            FILE* consoleFile = nullptr;
            freopen_s(&consoleFile, "CONOUT$", "wb", stdout);
        }

        if(_showWindow)
            _window.ShowWindow();

        _deviceManager.Initialize(_window);

        _blendStates.Initialize(_deviceManager.Device());
        _rasterizerStates.Initialize(_deviceManager.Device());
        _depthStencilStates.Initialize(_deviceManager.Device());
        _samplerStates.Initialize(_deviceManager.Device());

        // Create a font + SpriteRenderer
        _font.Initialize(L"Arial", 18, SpriteFont::Regular, true, _deviceManager.Device());
        _spriteRenderer.Initialize(_deviceManager.Device());

        Profiler::GlobalProfiler.Initialize(_deviceManager.Device(), _deviceManager.ImmediateContext());

        _window.RegisterMessageCallback(WM_SIZE, OnWindowResized, this);

        // Initialize AntTweakBar
        TwCall(TwInit(TW_DIRECT3D11, _deviceManager.Device()));

        // Create a tweak bar
        _tweakBar = TwNewBar("Settings");
        TwCall(TwDefine(" GLOBAL help='MJPs sample framework for DX11' "));
        TwCall(TwDefine(" GLOBAL fontsize=3 "));

        Settings.Initialize(_tweakBar);

        TwHelper::SetValuesWidth(Settings.TweakBar(), 120, false);

        AppSettings::Initialize(_deviceManager.Device());

        Initialize();

        AfterReset();

        while(_window.IsAlive())
        {
            if(!_window.IsMinimized())
            {
                _timer.Update();
                Settings.Update();

                CalculateFPS();

                AppSettings::Update();

                Update(_timer);

                UpdateShaders(_deviceManager.Device());

				// TODO: only update this giant cbuffer if gui value has changed
                AppSettings::UpdateCBuffer(_deviceManager.ImmediateContext());

                Render(_timer);

                // Render the profiler text
                _spriteRenderer.Begin(_deviceManager.ImmediateContext(), SpriteRenderer::Point);
                Profiler::GlobalProfiler.EndFrame(_spriteRenderer, _font);
                _spriteRenderer.End();

                {
                    PIXEvent pixEvent(L"Ant Tweak Bar");

                    // Render the TweakBar UI
                    TwCall(TwDraw());
                }

                _deviceManager.Present();
            }

            _window.MessageLoop();
        }
    }
    catch(SampleFramework11::Exception exception)
    {
        exception.ShowErrorMessage();
        return -1;
    }

    ShutdownShaders();

    TwCall(TwTerminate());

    if(_createConsole)
    {
        fclose(stdout);
        FreeConsole();
    }

    return _returnCode;
}

void App::CalculateFPS()
{
    timeDeltaBuffer[_currentTimeDeltaSample] = _timer.DeltaSecondsF();
    _currentTimeDeltaSample = (_currentTimeDeltaSample + 1) % NumTimeDeltaSamples;

    float averageDelta = 0;
    for(UINT i = 0; i < NumTimeDeltaSamples; ++i)
        averageDelta += timeDeltaBuffer[i];
    averageDelta /= NumTimeDeltaSamples;

    _fps = static_cast<UINT>(std::floor((1.0f / averageDelta) + 0.5f));
}

LRESULT App::OnWindowResized(void* context, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    App* app = reinterpret_cast<App*>(context);

    if(!app->_deviceManager.FullScreen() && wParam != SIZE_MINIMIZED)
    {
        int width, height;
        app->_window.GetClientArea(width, height);

        if(width != app->_deviceManager.BackBufferWidth() || height != app->_deviceManager.BackBufferHeight())
        {
            app->BeforeReset();

            app->_deviceManager.SetBackBufferWidth(width);
            app->_deviceManager.SetBackBufferHeight(height);
            app->_deviceManager.Reset();

            app->AfterReset();
        }
    }

    return 0;
}

void App::Exit()
{
    _window.Destroy();
}

void App::Initialize()
{
}

void App::BeforeReset()
{
}

void App::AfterReset()
{
    const uint32 width = _deviceManager.BackBufferWidth();
    const uint32 height = _deviceManager.BackBufferHeight();

    TwHelper::SetSize(_tweakBar, 375, _deviceManager.BackBufferHeight());
    TwHelper::SetPosition(_tweakBar, _deviceManager.BackBufferWidth() - 375, 0);
}

void App::ToggleFullScreen(bool fullScreen)
{
    if(fullScreen != _deviceManager.FullScreen())
    {
        BeforeReset();

        _deviceManager.SetFullScreen(fullScreen);
        _deviceManager.Reset();

        AfterReset();
    }
}

void App::RenderText(const std::wstring& text, Float2 pos)
{
    ID3D11DeviceContext* context = _deviceManager.ImmediateContext();

    // Set the backbuffer and viewport
    ID3D11RenderTargetView* rtvs[1] = { _deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, rtvs, NULL);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(rtvs[0], clearColor);

    SetViewport(context, _deviceManager.BackBufferWidth(), _deviceManager.BackBufferHeight());

    // Draw the text
    Float4x4 transform;
    transform.SetTranslation(Float3(pos.x, pos.y,0.0f));
    _spriteRenderer.Begin(context, SpriteRenderer::Point);
    _spriteRenderer.RenderText(_font, text.c_str(), transform.ToSIMD());
    _spriteRenderer.End();

    // Present
    _deviceManager.SwapChain()->Present(0, 0);

    // Pump the message loop
    _window.MessageLoop();
}

void App::RenderCenteredText(const std::wstring& text)
{

    // Measure the text
    Float2 textSize = _font.MeasureText(text.c_str());

    // Position it in the middle
    Float2 textPos;
    textPos.x = Round((_deviceManager.BackBufferWidth() / 2.0f) - (textSize.x / 2.0f));
    textPos.y = Round((_deviceManager.BackBufferHeight() / 2.0f) - (textSize.y / 2.0f));

    RenderText(text, textPos);
}

}