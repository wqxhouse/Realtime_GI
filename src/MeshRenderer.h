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

#include <Graphics\\Model.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\DeviceStates.h>
#include <Graphics\\Camera.h>
#include <Graphics\\SH.h>
#include <Graphics\\ShaderCompilation.h>

#include "AppSettings.h"

using namespace SampleFramework11;

struct BakeData;

class MeshRenderer
{

protected:

    // Constants
    static const uint32 NumCascades = 4;
    static const uint32 ReadbackLatency = 1;

public:

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void SetModel(const Model* model);

    void RenderDepth(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                     bool shadowRendering);
    void Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                ID3D11ShaderResourceView* envMap, const SH9Color& envMapSH,
                Float2 JitterOffset);

    void Update();

    void CreateReductionTargets(uint32 width, uint32 height);

    void ReduceDepth(ID3D11DeviceContext* context, DepthStencilBuffer& depthBuffer,
                     const Camera& camera);

    void ComputeShadowDepthBounds(const Camera& camera);

    void RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                         const Float4x4& world);

protected:

    void LoadShaders();
    void CreateShadowMaps();
    void ConvertToEVSM(ID3D11DeviceContext* context, uint32 cascadeIdx, Float3 cascadeScale);

    ID3D11DevicePtr _device;

    BlendStates _blendStates;
    RasterizerStates _rasterizerStates;
    DepthStencilStates _depthStencilStates;
    SamplerStates _samplerStates;

    const Model* _model = nullptr;

    DepthStencilBuffer _shadowMap;
    RenderTarget2D  _varianceShadowMap;
    RenderTarget2D _tempVSM;

    ID3D11RasterizerStatePtr _shadowRSState;
    ID3D11SamplerStatePtr _evsmSampler;

    std::vector<ID3D11InputLayoutPtr> _meshInputLayouts;
    VertexShaderPtr _meshVS[2];
    PixelShaderPtr _meshPS[2][2];

    std::vector<ID3D11InputLayoutPtr> _meshDepthInputLayouts;
    VertexShaderPtr _meshDepthVS;

    VertexShaderPtr _fullScreenVS;
    PixelShaderPtr _evsmConvertPS;
    PixelShaderPtr _evsmBlurH;
    PixelShaderPtr _evsmBlurV;

    ComputeShaderPtr _depthReductionInitialCS[2];
    ComputeShaderPtr _depthReductionCS;
    std::vector<RenderTarget2D> _depthReductionTargets;
    StagingTexture2D _reductionStagingTextures[ReadbackLatency];
    uint32 _currFrame = 0;

    Float2 _shadowDepthBounds = Float2(0.0f, 1.0f);

    ID3D11ShaderResourceViewPtr _specularLookupTexture;

    Float4x4 _prevWVP;

    // Constant buffers
    struct MeshVSConstants
    {
        Float4Align Float4x4 World;
        Float4Align Float4x4 View;
        Float4Align Float4x4 WorldViewProjection;
        Float4Align Float4x4 PrevWorldViewProjection;
    };

    struct MeshPSConstants
    {
        Float4Align Float3 CameraPosWS;

        Float4Align Float4x4 ShadowMatrix;
        Float4Align float CascadeSplits[NumCascades];

        Float4Align Float4 CascadeOffsets[NumCascades];
        Float4Align Float4 CascadeScales[NumCascades];

        float OffsetScale;
        float PositiveExponent;
        float NegativeExponent;
        float LightBleedingReduction;

        Float4Align Float4x4 Projection;

        Float4Align ShaderSH9Color EnvironmentSH;

        Float2 RTSize;
        Float2 JitterOffset;
    };

	// struct 

    struct EVSMConstants
    {
        Float3 CascadeScale;
        float PositiveExponent;
        float NegativeExponent;
        float FilterSize;
        Float2 ShadowMapSize;
    };

    struct ReductionConstants
    {
        Float4x4 Projection;
        float NearClip;
        float FarClip;
        Uint2 TextureSize;
        uint32 NumSamples;
    };

    ConstantBuffer<MeshVSConstants> _meshVSConstants;
    ConstantBuffer<MeshPSConstants> _meshPSConstants;
	//ConstantBuffer<MeshPSConstants> 
    ConstantBuffer<EVSMConstants> _evsmConstants;
    ConstantBuffer<ReductionConstants> _reductionConstants;
};