//=================================================================================================
//
//  MSAA Filtering 2.0 Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code licensed under the MIT license
//
//=================================================================================================

#include "PCH.h"

#include "MeshRenderer.h"

#include <Exceptions.h>
#include <Utility.h>
#include <Graphics\\ShaderCompilation.h>
#include <App.h>
#include <Graphics\\Textures.h>

#include "AppSettings.h"
#include "SharedConstants.h"
#include "ShadowMapSettings.h"


// Renders a text string indicating the current progress for compiling shaders
static bool32 RenderShaderProgress(uint32 currShader, uint32 numShaders)
{
	float percent = Round((currShader / static_cast<float>(numShaders)) * 100.0f);
	std::wstring progressText = L"Compiling shaders (" + ToString(percent) + L"%)";
	GlobalApp->RenderCenteredText(progressText);
	return GlobalApp->Window().IsAlive();
}

uint32 MeshRenderer::boolArrToUint32(bool32 *arr, int num)
{
	uint32 bits = 0;
	for (int i = 0; i < num; i++)
	{
		bits |= (arr[i] & 0x1) << i;
	}
	return bits;
}

void MeshRenderer::uint32ToBoolArr(uint32 bits, bool32 *arr)
{
	for (int i = 0; i < 32; i++)
	{
		arr[i] = (bits & (1 << i)) >> i;
	}
}

void MeshRenderer::GenVSShaderPermutations(ID3D11Device *device, const wchar *path, const char *entryName, 
	const char **shaderDescs, int numDescs, std::unordered_map<uint32, VertexShaderPtr> &out)
{
	CompileOptions opts;
	bool32 *states = (bool32 *)malloc(numDescs * sizeof(bool32));
	GenVSRecursive(device, 0, states, &opts, path, entryName, shaderDescs, numDescs, out);
	free(states);
}

void MeshRenderer::GenVSRecursive(ID3D11Device *device, int depth, bool32 *descStates, CompileOptions *opts, 
	const wchar *path, const char *entryName, const char **shaderDescs, int numDescs, std::unordered_map<uint32, VertexShaderPtr> &out)
{
	if (depth == numDescs)
	{
		if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
			return;

		opts->Reset();
		for (int i = 0; i < numDescs; i++)
		{
			opts->Add(shaderDescs[i], descStates[i]);
		}

		uint32 bits = boolArrToUint32(descStates, numDescs);
		VertexShaderPtr ptr = CompileVSFromFile(device, path, entryName, "vs_5_0", *opts);
		out.insert(std::make_pair(bits, ptr));
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			descStates[numDescs - 1 - depth] = i;
			GenVSRecursive(device, depth + 1, descStates, opts, path, entryName, shaderDescs, numDescs, out);
		}
	}
}

void MeshRenderer::GenPSShaderPermutations(ID3D11Device *device, const wchar *path, const char *entryName, 
	const char **shaderDescs, int numDescs, std::unordered_map<uint32, PixelShaderPtr> &out)
{
	CompileOptions opts;
	bool32 *states = (bool32 *)malloc(numDescs * sizeof(bool32));
	GenPSRecursive(device, 0, states, &opts, path, entryName, shaderDescs, numDescs, out);
	free(states);
}

void MeshRenderer::GenPSRecursive(ID3D11Device *device, int depth, bool32 *descStates, CompileOptions *opts,
	const wchar *path, const char *entryName, const char **shaderDescs, int numDescs, std::unordered_map<uint32, PixelShaderPtr> &out)
{
	if (depth == numDescs)
	{
		if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
			return;

		opts->Reset();
		for (int i = 0; i < numDescs; i++)
		{
			opts->Add(shaderDescs[i], descStates[i]);
			/*printf("%s ", shaderDescs[i]);
			printf("%d ", descStates[i]);*/
		}

		//printf("\n");

		uint32 bits = boolArrToUint32(descStates, numDescs);
		//printf("Bits: %d\n", bits);
		PixelShaderPtr ptr = CompilePSFromFile(device, path, entryName, "ps_5_0", *opts);
		out.insert(std::make_pair(bits, ptr));
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			descStates[numDescs - 1 - depth] = i;
			GenPSRecursive(device, depth + 1, descStates, opts, path, entryName, shaderDescs, numDescs, out);
		}
	}
}


void MeshRenderer::LoadShaders()
{
	_totalShaderNum = (int)pow(2, 5) + (int)pow(2, 8) + 8;

    CompileOptions opts;

	// Mesh.hlsl
	const char *vsDescs[] = { "UseNormalMapping_", "UseAlbedoMap_", "UseMetallicMap_", "UseRoughnessMap_", "UseEmissiveMap_" };
	const char *psDescs[] = { "UseNormalMapping_", "UseAlbedoMap_", "UseMetallicMap_", "UseRoughnessMap_", "UseEmissiveMap_", "CreateCubemap_", "CentroidSampling_", "IsGBuffer_" };
	GenVSShaderPermutations(_device, L"Mesh.hlsl", "VS", vsDescs, _countof(vsDescs), _meshVertexShaders);
	GenPSShaderPermutations(_device, L"Mesh.hlsl", "PS", psDescs, _countof(psDescs), _meshPixelShaders);

	// DepthOnly.hlsl
	_meshDepthVS = CompileVSFromFile(_device, L"DepthOnly.hlsl", "VS", "vs_5_0");
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

	// EVSMConvert.hlsl
    _fullScreenVS = CompileVSFromFile(_device, L"EVSMConvert.hlsl", "FullScreenVS");
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

    opts.Reset();
    opts.Add("MSAASamples_", ShadowMSAASamples);
    _evsmConvertPS = CompilePSFromFile(_device, L"EVSMConvert.hlsl", "ConvertToEVSM", "ps_5_0", opts);
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

    opts.Reset();
    opts.Add("Horizontal_", 1);
    opts.Add("Vertical_", 0);
    opts.Add("SampleRadius_", SampleRadius);
    _evsmBlurH = CompilePSFromFile(_device, L"EVSMConvert.hlsl", "BlurEVSM", "ps_5_0", opts);
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

    opts.Reset();
    opts.Add("Horizontal_", 0);
    opts.Add("Vertical_", 1);
    opts.Add("SampleRadius_", SampleRadius);
    _evsmBlurV = CompilePSFromFile(_device, L"EVSMConvert.hlsl", "BlurEVSM", "ps_5_0", opts);
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

	// DepthReduction.hlsl
    opts.Reset();
    opts.Add("MSAA_", 0);
    _depthReductionInitialCS[0] = CompileCSFromFile(_device, L"DepthReduction.hlsl", "DepthReductionInitialCS", "cs_5_0", opts);
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

    opts.Reset();
    opts.Add("MSAA_", 1);
    _depthReductionInitialCS[1] = CompileCSFromFile(_device, L"DepthReduction.hlsl", "DepthReductionInitialCS", "cs_5_0", opts);
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;

    _depthReductionCS = CompileCSFromFile(_device, L"DepthReduction.hlsl", "DepthReductionCS");
	if (RenderShaderProgress(_curShaderNum++, _totalShaderNum) == false)
		return;
}

void MeshRenderer::CreateShadowMaps()
{
    // Create the shadow map as a texture array
    _shadowMap.Initialize(_device, ShadowMapSize, ShadowMapSize, DXGI_FORMAT_D24_UNORM_S8_UINT, true,
                         ShadowMSAASamples, 0, 1);

    DXGI_FORMAT smFmt = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uint32 numMips = EnableShadowMips ? 0 : 1;
    _varianceShadowMap.Initialize(_device, ShadowMapSize, ShadowMapSize, smFmt, numMips, 1, 0,
                                 EnableShadowMips, false, NumCascades, false);

    _tempVSM.Initialize(_device, ShadowMapSize, ShadowMapSize, smFmt, 1, 1, 0, false, false, 1, false);
}

void MeshRenderer::SetCubemapCapture(bool32 tf)
{
	if (_drawingCubemap == tf) return;
	_drawingCubemap = tf;
	ReMapMeshShaders();
}

void MeshRenderer::SetDrawGBuffer(bool32 tf)
{
	if (_drawingGBuffer == tf) return;
	_drawingGBuffer = tf;
	ReMapMeshShaders();
}

void MeshRenderer::SetParallaxCorrection(Float3 newProbePosWS, Float3 newMaxbox, Float3 newMinbox){
	probePosWS = newProbePosWS;
	maxbox = newMaxbox;
	minbox = newMinbox;
}

void MeshRenderer::ReMapMeshShaders()
{
	_meshVertexShadersMap.clear();
	_meshPixelShadersMap.clear();

	// re-map shader for cubemap
	for (uint64 i = 0; i < _scene->getNumModels(); i++)
	{
		Model *m = _scene->getModel(i);
		GenMeshShaderMap(m);
	}
}

void MeshRenderer::SetScene(Scene *scene)
{
	// unless the scene json or scene gui option or model has changed, 
	// this function will only be called once
	_scene = scene;
	_meshInputLayouts.clear();
	_meshDepthInputLayouts.clear();

	_meshVertexShadersMap.clear();
	_meshPixelShadersMap.clear();

	// generate input layout for every model's mesh
	for (uint64 i = 0; i < _scene->getNumModels(); i++)
	{
		Model *m = _scene->getModel(i);
		GenMeshShaderMap(m);
		GenAndCacheMeshInputLayout(m);
	}
}

void MeshRenderer::SortSceneObjects(const Float4x4 &viewMatrix)
{
	_scene->sortSceneObjects(viewMatrix);
}

void MeshRenderer::GenMeshShaderMap(const Model *model)
{
	// Map mesh to shaders
	for (uint64 i = 0; i < model->Meshes().size(); ++i)
	{
		const Mesh &mesh = model->Meshes()[i];
		const bool32 *materialFlags = mesh.MaterialFlags();

		// TODO: use AppSettings to automate the process - data driven
		// decouple string dependency
		bool32 arr[8]; // five maps + two modes + if gen gbuffer
		materialFlags[(uint64)MaterialFlag::HasNormalMap] ? arr[0] = true : arr[0] = false;
		materialFlags[(uint64)MaterialFlag::HasAlbedoMap] ? arr[1] = true : arr[1] = false;
		materialFlags[(uint64)MaterialFlag::HasRoughnessMap] ? arr[2] = true : arr[2] = false;
		materialFlags[(uint64)MaterialFlag::HasMetallicMap] ? arr[3] = true : arr[3] = false;
		materialFlags[(uint64)MaterialFlag::HasEmissiveMap] ? arr[4] = true : arr[4] = false;

		// TODO: the following case somehow defeats the purpose of the design
		_drawingCubemap ? arr[5] = true : arr[5] = false;
		AppSettings::CentroidSampling ? arr[6] = true : arr[6] = false;

		_drawingGBuffer ? arr[7] = true : arr[7] = false;

		uint32 vsbits = boolArrToUint32(arr, 5);
		uint32 psbits = boolArrToUint32(arr, 8);

		VertexShaderPtr vs = _meshVertexShaders[vsbits];
		PixelShaderPtr ps = _meshPixelShaders[psbits];

		_meshVertexShadersMap.insert(std::make_pair(&mesh, vs));
		_meshPixelShadersMap.insert(std::make_pair(&mesh, ps));
	}
}

void MeshRenderer::GenAndCacheMeshInputLayout(const Model* model)
{
	// TODO: optimize this; group meshes with the same input layout to reduce api calls
    for(uint64 i = 0; i < model->Meshes().size(); ++i)
    {
        const Mesh& mesh = model->Meshes()[i];
		VertexShaderPtr vs = _meshVertexShadersMap[&mesh];

		ID3D11InputLayoutPtr inputLayout;

		if (_meshInputLayouts.find(&mesh) == _meshInputLayouts.end())
		{
			DXCall(_device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
				vs->ByteCode->GetBufferPointer(), vs->ByteCode->GetBufferSize(), &inputLayout));
			_meshInputLayouts.insert(std::make_pair(&mesh, inputLayout));
		}

		if (_meshDepthInputLayouts.find(&mesh) == _meshDepthInputLayouts.end())
		{
			DXCall(_device->CreateInputLayout(mesh.InputElements(), mesh.NumInputElements(),
				_meshDepthVS->ByteCode->GetBufferPointer(), _meshDepthVS->ByteCode->GetBufferSize(), &inputLayout));
			_meshDepthInputLayouts.insert(std::make_pair(&mesh, inputLayout));
		}
    }
}

// Loads resources
void MeshRenderer::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
    this->_device = device;

	this->_drawingCubemap = false;

	_drawingCubemap = false;
	_drawingGBuffer = false;


	_blendStates.Initialize(device);
	_rasterizerStates.Wireframe();
    _rasterizerStates.Initialize(device);

    _depthStencilStates.Initialize(device);
    _samplerStates.Initialize(device);

    _meshVSConstants.Initialize(device);
    _meshPSConstants.Initialize(device);
    _evsmConstants.Initialize(device);
    _reductionConstants.Initialize(device);

	_curShaderNum = 0;
	_totalShaderNum = 0;

    LoadShaders();

    D3D11_RASTERIZER_DESC rsDesc = RasterizerStates::NoCullDesc();
    rsDesc.DepthClipEnable = false;
    DXCall(device->CreateRasterizerState(&rsDesc, &_shadowRSState));

    D3D11_SAMPLER_DESC sampDesc = SamplerStates::AnisotropicDesc();
    sampDesc.MaxAnisotropy = ShadowAnisotropy;
    DXCall(device->CreateSamplerState(&sampDesc, &_evsmSampler));

    // Create the staging textures for reading back the reduced depth buffer
    for(uint32 i = 0; i < ReadbackLatency; ++i)
        _reductionStagingTextures[i].Initialize(device, 1, 1, DXGI_FORMAT_R16G16_UNORM);

    _specularLookupTexture = LoadTexture(device, L"..\\Content\\Textures\\SpecularLookup.dds");

    CreateShadowMaps();
}

void MeshRenderer::Update()
{
}

void MeshRenderer::CreateReductionTargets(uint32 width, uint32 height)
{
    _depthReductionTargets.clear();

    uint32 w = width;
    uint32 h = height;

    while(w > 1 || h > 1)
    {
        w = DispatchSize(ReductionTGSize, w);
        h = DispatchSize(ReductionTGSize, h);

        RenderTarget2D rt;
        rt.Initialize(_device, w, h, DXGI_FORMAT_R16G16_UNORM, 1, 1, 0, false, true);
        _depthReductionTargets.push_back(rt);
    }
}

void MeshRenderer::ReduceDepth(ID3D11DeviceContext* context, DepthStencilBuffer& depthBuffer,
                                const Camera& camera)
{
    PIXEvent event(L"Depth Reduction");

    _reductionConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
    _reductionConstants.Data.NearClip = camera.NearClip();
    _reductionConstants.Data.FarClip = camera.FarClip();
    _reductionConstants.Data.TextureSize.x = depthBuffer.Width;
    _reductionConstants.Data.TextureSize.y = depthBuffer.Height;
    _reductionConstants.Data.NumSamples = depthBuffer.MultiSamples;
    _reductionConstants.ApplyChanges(context);
    _reductionConstants.SetCS(context, 0);

    ID3D11RenderTargetView* rtvs[1] = { NULL };
    context->OMSetRenderTargets(1, rtvs, NULL);

    ID3D11UnorderedAccessView* uavs[1] = { _depthReductionTargets[0].UAView };
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    ID3D11ShaderResourceView* srvs[1] = { depthBuffer.SRView };
    context->CSSetShaderResources(0, 1, srvs);

    const bool msaa = depthBuffer.MultiSamples > 1;

    ID3D11ComputeShader* shader = _depthReductionInitialCS[msaa ? 1 : 0];
    context->CSSetShader(shader, NULL, 0);

    uint32 dispatchX = _depthReductionTargets[0].Width;
    uint32 dispatchY = _depthReductionTargets[0].Height;
    context->Dispatch(dispatchX, dispatchY, 1);

    uavs[0] = NULL;
    context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

    srvs[0] = NULL;
    context->CSSetShaderResources(0, 1, srvs);

    context->CSSetShader(_depthReductionCS, NULL, 0);

    for(uint32 i = 1; i < _depthReductionTargets.size(); ++i)
    {
        RenderTarget2D& srcTexture = _depthReductionTargets[i - 1];
        _reductionConstants.Data.TextureSize.x = srcTexture.Width;
        _reductionConstants.Data.TextureSize.y = srcTexture.Height;
        _reductionConstants.Data.NumSamples = srcTexture.MultiSamples;
        _reductionConstants.ApplyChanges(context);

        uavs[0] = _depthReductionTargets[i].UAView;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = srcTexture.SRView;
        context->CSSetShaderResources(0, 1, srvs);

        dispatchX = _depthReductionTargets[i].Width;
        dispatchY = _depthReductionTargets[i].Height;
        context->Dispatch(dispatchX, dispatchY, 1);

        uavs[0] = NULL;
        context->CSSetUnorderedAccessViews(0, 1, uavs, NULL);

        srvs[0] = NULL;
        context->CSSetShaderResources(0, 1, srvs);
    }

    // Copy to a staging texture
    ID3D11Texture2D* lastTarget = _depthReductionTargets[_depthReductionTargets.size() - 1].Texture;
    context->CopyResource(_reductionStagingTextures[_currFrame % ReadbackLatency].Texture, lastTarget);

	context->CSSetShader(nullptr, nullptr, 0);

    ++_currFrame;

    if(_currFrame >= ReadbackLatency)
    {
        StagingTexture2D& stagingTexture = _reductionStagingTextures[_currFrame % ReadbackLatency];

        uint32 pitch;
        const uint16* texData = reinterpret_cast<uint16*>(stagingTexture.Map(context, 0, pitch));
        _shadowDepthBounds.x = texData[0] / static_cast<float>(0xffff);
        _shadowDepthBounds.y = texData[1] / static_cast<float>(0xffff);

        stagingTexture.Unmap(context, 0);
    }
    else
    {
        _shadowDepthBounds = Float2(0.0f, 1.0f);
    }
}

// Computes shadow depth bounds on the CPU using the mesh vertex positions
void MeshRenderer::ComputeShadowDepthBoundsCPU(const Camera& camera)
{
    Float4x4 viewMatrix = camera.ViewMatrix();
    const float nearClip = camera.NearClip();
    const float farClip = camera.FarClip();
    const float clipDist = farClip - nearClip;

    float minDepth = 1.0f;
    float maxDepth = 0.0f;
	for (int i = 0; i < _scene->getNumModels(); i++)
	{
		Model *model = _scene->getModel(i);
		const uint64 numMeshes = model->Meshes().size();
		for (uint64 meshIdx = 0; meshIdx < numMeshes; ++meshIdx)
		{
			const Mesh& mesh = model->Meshes()[meshIdx];
			const uint64 numVerts = mesh.NumVertices();
			const uint64 stride = mesh.VertexStride();
			const uint8* vertices = mesh.Vertices();
			for (uint64 i = 0; i < numVerts; ++i)
			{
				const Float3& position = *reinterpret_cast<const Float3*>(vertices);
				float viewSpaceZ = Float3::Transform(position, viewMatrix).z;
				float depth = Saturate((viewSpaceZ - nearClip) / clipDist);
				minDepth = std::min(minDepth, depth);
				maxDepth = std::max(maxDepth, depth);
				vertices += stride;
			}
		}
	}
    
    _shadowDepthBounds = Float2(minDepth, maxDepth);
}

// Convert to an EVSM map
void MeshRenderer::ConvertToEVSM(ID3D11DeviceContext* context, uint32 cascadeIdx, Float3 cascadeScale)
{
    PIXEvent event(L"EVSM Conversion");

    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->RSSetState(_rasterizerStates.NoCull());
    context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);
    ID3D11Buffer* vbs[1] = { NULL };
    uint32 strides[1] = { 0 };
    uint32 offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetIndexBuffer(NULL, DXGI_FORMAT_R32_UINT, 0);
    context->IASetInputLayout(NULL);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.Width = static_cast<float>(_varianceShadowMap.Width);
    vp.Height = static_cast<float>(_varianceShadowMap.Height);
    context->RSSetViewports(1, &vp);

    _evsmConstants.Data.PositiveExponent = PositiveExponent;
    _evsmConstants.Data.NegativeExponent = NegativeExponent;
    _evsmConstants.Data.CascadeScale = _meshPSConstants.Data.CascadeScales[cascadeIdx].To3D();
    _evsmConstants.Data.FilterSize = 1.0f;
    _evsmConstants.Data.ShadowMapSize.x = float(_varianceShadowMap.Width);
    _evsmConstants.Data.ShadowMapSize.y = float(_varianceShadowMap.Height);
    _evsmConstants.ApplyChanges(context);
    _evsmConstants.SetPS(context, 0);

    context->VSSetShader(_fullScreenVS, NULL, 0);
    context->PSSetShader(_evsmConvertPS, NULL, 0);

    ID3D11RenderTargetView* rtvs[1] = { _varianceShadowMap.RTVArraySlices[cascadeIdx] };
    context->OMSetRenderTargets(1, rtvs, NULL);

    ID3D11ShaderResourceView* srvs[1] = { _shadowMap.SRView };
    context->PSSetShaderResources(0, 1, srvs);

    context->Draw(3, 0);

    srvs[0] = NULL;
    context->PSSetShaderResources(0, 1, srvs);

    const float FilterSizeU = std::max(FilterSize * cascadeScale.x, 1.0f);
    const float FilterSizeV = std::max(FilterSize * cascadeScale.y, 1.0f);

    if(FilterSizeU > 1.0f || FilterSizeV > 1.0f)
    {
        // Horizontal pass
        _evsmConstants.Data.FilterSize = FilterSizeU;
        _evsmConstants.ApplyChanges(context);

        uint32 sampleRadiusU = static_cast<uint32>((FilterSizeU / 2) + 0.499f);

        rtvs[0] = _tempVSM.RTView;
        context->OMSetRenderTargets(1, rtvs, NULL);

        srvs[0] = _varianceShadowMap.SRVArraySlices[cascadeIdx];
        context->PSSetShaderResources(0, 1, srvs);

        context->PSSetShader(_evsmBlurH, NULL, 0);

        context->Draw(3, 0);

        srvs[0] = NULL;
        context->PSSetShaderResources(0, 1, srvs);

        // Vertical pass
        _evsmConstants.Data.FilterSize = FilterSizeV;
        _evsmConstants.ApplyChanges(context);

        uint32 sampleRadiusV = static_cast<uint32>((FilterSizeV / 2) + 0.499f);

        rtvs[0] = _varianceShadowMap.RTVArraySlices[cascadeIdx];
        context->OMSetRenderTargets(1, rtvs, NULL);

        srvs[0] = _tempVSM.SRView;
        context->PSSetShaderResources(0, 1, srvs);

        context->PSSetShader(_evsmBlurV, NULL, 0);

        context->Draw(3, 0);

        srvs[0] = NULL;
        context->PSSetShaderResources(0, 1, srvs);
    }

    if(EnableShadowMips && cascadeIdx == NumCascades - 1)
        context->GenerateMips(_varianceShadowMap.SRView);
}

// Renders all meshes in the model, with shadows
void MeshRenderer::Render(ID3D11DeviceContext* context, const Camera& camera, const Float4x4& world,
                          ID3D11ShaderResourceView* envMap, const SH9Color& envMapSH,
                          Float2 jitterOffset)
{
    PIXEvent event(L"Mesh Rendering");

	DoSceneObjectsFrustumTests(camera, false);

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(_blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);

	// TODO: this is super bad coupling... but we are out of time
	if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		context->OMSetDepthStencilState(_depthStencilStates.DepthEnabled(), 0);
	}
	else if (AppSettings::CurrentShadingTech == ShadingTech::Clustered_Deferred)
	{
		context->OMSetDepthStencilState(_depthStencilStates.DepthWriteEnabled(), 0);
	}
    context->RSSetState(_rasterizerStates.BackFaceCull());

    ID3D11SamplerState* sampStates[3] = {
        _samplerStates.Anisotropic(),
        _evsmSampler,
        _samplerStates.LinearClamp(),
    };

    context->PSSetSamplers(0, 3, sampStates);

	// set PS constants
	// TODO: make deferred a unique structure
	//if (AppSettings::CurrentShadingTech == ShadingTech::Forward)
	{
		_meshPSConstants.Data.CameraPosWS = camera.Position();
		_meshPSConstants.Data.ProbePosWS = probePosWS;
		_meshPSConstants.Data.OffsetScale = OffsetScale;
		_meshPSConstants.Data.PositiveExponent = PositiveExponent;
		_meshPSConstants.Data.NegativeExponent = NegativeExponent;
		_meshPSConstants.Data.LightBleedingReduction = LightBleedingReduction;
		_meshPSConstants.Data.View = Float4x4::Transpose(camera.ViewMatrix());
		_meshPSConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
		_meshPSConstants.Data.EnvironmentSH = envMapSH;
		_meshPSConstants.Data.RTSize.x = float(GlobalApp->DeviceManager().BackBufferWidth());
		_meshPSConstants.Data.RTSize.y = float(GlobalApp->DeviceManager().BackBufferHeight());
		_meshPSConstants.Data.JitterOffset = jitterOffset;
		_meshPSConstants.ApplyChanges(context);
		_meshPSConstants.SetPS(context, 0);
		_meshPSConstants.Data.MaxBox = maxbox;
		_meshPSConstants.Data.MinBox = minbox;
	}

    // Set shaders
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);

	_scene->sortSceneObjects(camera.ViewMatrix());
	RenderSceneObjects(context, world, camera, envMap, envMapSH, jitterOffset, _scene->getStaticOpaqueObjectsPtr(), _scene->getNumStaticOpaqueObjects());
	RenderSceneObjects(context, world, camera, envMap, envMapSH, jitterOffset, _scene->getDynamicOpaqueObjectsPtr(), _scene->getNumDynamicOpaueObjects());

	// TODO: refactor
    ID3D11ShaderResourceView* nullSRVs[8] = { nullptr };
    context->PSSetShaderResources(0, 8, nullSRVs);
}

void MeshRenderer::RenderSceneObjects(ID3D11DeviceContext* context, const Float4x4 &world, const Camera& camera,
	ID3D11ShaderResourceView* envMap, const SH9Color& envMapSH,
	Float2 jitterOffset, SceneObject *sceneObjectsArr, int numSceneObjs)
{
	for (uint64 objIndex = 0; objIndex < numSceneObjs; objIndex++)
	{
		// Frustum culling on scene object bound
		if (!sceneObjectsArr[objIndex].bound->frustumTest)
		{
			continue;
		}

		ModelPartsBound *partsBound = sceneObjectsArr[objIndex].bound->modelPartsBound;
		Float4x4 worldMat = *sceneObjectsArr[objIndex].base * world;
		Model *model = sceneObjectsArr[objIndex].model;

		// Set VS constant buffer
		_meshVSConstants.Data.World = Float4x4::Transpose(worldMat);
		_meshVSConstants.Data.View = Float4x4::Transpose(camera.ViewMatrix());
		_meshVSConstants.Data.WorldViewProjection = Float4x4::Transpose(worldMat * camera.ViewProjectionMatrix());
		_meshVSConstants.Data.PrevWorldViewProjection = *sceneObjectsArr[objIndex].prevWVP;
		_meshVSConstants.ApplyChanges(context);
		_meshVSConstants.SetVS(context, 0);

		*sceneObjectsArr[objIndex].prevWVP = _meshVSConstants.Data.WorldViewProjection;

		// Draw all meshes
		uint32 partCount = 0;
		for (uint64 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
		{
			const Mesh& mesh = model->Meshes()[meshIdx];

			if (partsBound->BoundingSpheres.size() == 1) // which means the mesh has only one part - which is the assimp-type mesh
			{
				if (!partsBound->FrustumTests[0])
				{
					continue;
				}
			}

			// Set per mesh shaders
			// TODO: sort by view depth
			// TODO: batch draw calls for static object
			context->VSSetShader(_meshVertexShadersMap[&mesh], nullptr, 0);
			context->PSSetShader(_meshPixelShadersMap[&mesh],  nullptr, 0);
			
			// Set the vertices and indices
			ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
			UINT vertexStrides[1] = { mesh.VertexStride() };
			UINT offsets[1] = { 0 };
			context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
			context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Set the input layout
			// context->IASetInputLayout(_meshInputLayouts[meshIdx]);
			context->IASetInputLayout(_meshInputLayouts[&mesh]);

			// Draw all parts
			for (uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
			{
				const MeshPart& part = mesh.MeshParts()[partIdx];
				const MeshMaterial& material = model->Materials()[part.MaterialIdx];

				// Frustum culling on parts
				if (partsBound->FrustumTests[partCount++])
				{
					const MeshPart& part = mesh.MeshParts()[partIdx];
					const MeshMaterial& material = model->Materials()[part.MaterialIdx];

					// Set the textures
					// TODO : strip out unnecessary cases for unique
					ID3D11ShaderResourceView* psTextures[] =
					{
						material.DiffuseMap,
						material.NormalMap,
						_varianceShadowMap.SRView,
						envMap,
						_specularLookupTexture,
						material.RoughnessMap, 
						material.MetallicMap, 
						material.EmissiveMap
					};

					context->PSSetShaderResources(0, _countof(psTextures), psTextures);
					context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
				}
			}
		}
	}
}

// Renders all meshes using depth-only rendering
void MeshRenderer::RenderDepth(ID3D11DeviceContext* context, const Camera& camera,
    const Float4x4& world, bool shadowRendering)
{
    PIXEvent event(L"Mesh Depth Rendering");

	DoSceneObjectsFrustumTests(camera, shadowRendering);

    // Set states
    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(_blendStates.ColorWriteDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(_depthStencilStates.DepthWriteEnabled(), 0);

    if(shadowRendering)
        context->RSSetState(_shadowRSState);
    else
        context->RSSetState(_rasterizerStates.BackFaceCull());

    // Set shaders
    context->VSSetShader(_meshDepthVS, nullptr, 0);
    context->PSSetShader(nullptr, nullptr, 0);
    context->GSSetShader(nullptr, nullptr, 0);
    context->DSSetShader(nullptr, nullptr, 0);
    context->HSSetShader(nullptr, nullptr, 0);

	_scene->sortSceneObjects(camera.ViewMatrix());
	RenderDepthSceneObjects(context, world, camera, _scene->getStaticOpaqueObjectsPtr(), _scene->getNumStaticOpaqueObjects());
	RenderDepthSceneObjects(context, world, camera, _scene->getDynamicOpaqueObjectsPtr(), _scene->getNumDynamicOpaueObjects());
}

void MeshRenderer::RenderDepthSceneObjects(ID3D11DeviceContext* context, const Float4x4 &world, 
	const Camera& camera, SceneObject *sceneObjectsArr, int numSceneObjs)
{
	for (uint64 objIndex = 0; objIndex < numSceneObjs; objIndex++)
	{
		// Frustum culling on scene object bound
		if (!sceneObjectsArr[objIndex].bound->frustumTest)
		{
			continue;
		}

		ModelPartsBound *partsBound = sceneObjectsArr[objIndex].bound->modelPartsBound;
		Float4x4 worldMat = *sceneObjectsArr[objIndex].base * world;
		Model *model = sceneObjectsArr[objIndex].model;

		// Set constant buffers
		_meshVSConstants.Data.World = Float4x4::Transpose(worldMat);
		_meshVSConstants.Data.View = Float4x4::Transpose(camera.ViewMatrix());
		_meshVSConstants.Data.WorldViewProjection = Float4x4::Transpose(worldMat * camera.ViewProjectionMatrix());
		_meshVSConstants.ApplyChanges(context);
		_meshVSConstants.SetVS(context, 0);

		uint32 partCount = 0;
		for (uint32 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
		{
			const Mesh& mesh = model->Meshes()[meshIdx];

			// Set the vertices and indices
			ID3D11Buffer* vertexBuffers[1] = { mesh.VertexBuffer() };
			UINT vertexStrides[1] = { mesh.VertexStride() };
			UINT offsets[1] = { 0 };
			context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
			context->IASetIndexBuffer(mesh.IndexBuffer(), mesh.IndexBufferFormat(), 0);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// Set the input layout
			context->IASetInputLayout(_meshDepthInputLayouts[&mesh]);

			// Draw all parts
			for (uint64 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
			{
				// Frustum culling on parts
				if (partsBound->FrustumTests[partCount++])
				{
					const MeshPart& part = mesh.MeshParts()[partIdx];
					context->DrawIndexed(part.IndexCount, part.IndexStart, 0);
				}
			}
		}
	}
}

// Renders meshes using cascaded shadow mapping
void MeshRenderer::RenderShadowMap(ID3D11DeviceContext* context, const Camera& camera,
                                   const Float4x4& world)
{
    PIXEvent event(L"Mesh Shadow Map Rendering");

    // Get the current render targets + viewport
    ID3D11RenderTargetView* renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
    ID3D11DepthStencilView* depthStencil = NULL;
    context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargets, &depthStencil);

    uint32 numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    D3D11_VIEWPORT oldViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
    context->RSGetViewports(&numViewports, oldViewports);

    const float sMapSize = static_cast<float>(ShadowMapSize);

    const float MinDistance = _shadowDepthBounds.x;
    const float MaxDistance = _shadowDepthBounds.y;

    // Compute the split distances based on the partitioning mode
    float CascadeSplits[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

    {
        float lambda = 1.0f;

        float nearClip = camera.NearClip();
        float farClip = camera.FarClip();
        float clipRange = farClip - nearClip;

        float minZ = nearClip + MinDistance * clipRange;
        float maxZ = nearClip + MaxDistance * clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        for(uint32 i = 0; i < NumCascades; ++i)
        {
            float p = (i + 1) / static_cast<float>(NumCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = lambda * (log - uniform) + uniform;
            CascadeSplits[i] = (d - nearClip) / clipRange;
        }
    }

    Float3 c0Extents;
    Float4x4 c0Matrix;

    // Render the meshes to each cascade
    for(uint32 cascadeIdx = 0; cascadeIdx < NumCascades; ++cascadeIdx)
    {
        PIXEvent cascadeEvent((L"Rendering Shadow Map Cascade " + ToString(cascadeIdx)).c_str());

        // Set the viewport
        D3D11_VIEWPORT viewport;
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = sMapSize;
        viewport.Height = sMapSize;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        context->RSSetViewports(1, &viewport);

        // Set the shadow map as the depth target
        ID3D11DepthStencilView* dsv = _shadowMap.DSView;
        ID3D11RenderTargetView* nullRenderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { NULL };
        context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, nullRenderTargets, dsv);
        context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Get the 8 points of the view frustum in world space
        XMVECTOR frustumCornersWS[8] =
        {
            XMVectorSet(-1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet( 1.0f,  1.0f, 0.0f, 1.0f),
            XMVectorSet( 1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f),
            XMVectorSet(-1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet( 1.0f,  1.0f, 1.0f, 1.0f),
            XMVectorSet( 1.0f, -1.0f, 1.0f, 1.0f),
            XMVectorSet(-1.0f, -1.0f, 1.0f, 1.0f),
        };

        float prevSplitDist = cascadeIdx == 0 ? MinDistance : CascadeSplits[cascadeIdx - 1];
        float splitDist = CascadeSplits[cascadeIdx];

        XMVECTOR det;
        XMMATRIX invViewProj = XMMatrixInverse(&det, camera.ViewProjectionMatrix().ToSIMD());
        for(uint32 i = 0; i < 8; ++i)
            frustumCornersWS[i] = XMVector3TransformCoord(frustumCornersWS[i], invViewProj);

        // Get the corners of the current cascade slice of the view frustum
        for(uint32 i = 0; i < 4; ++i)
        {
            XMVECTOR cornerRay = XMVectorSubtract(frustumCornersWS[i + 4], frustumCornersWS[i]);
            XMVECTOR nearCornerRay = XMVectorScale(cornerRay, prevSplitDist);
            XMVECTOR farCornerRay = XMVectorScale(cornerRay, splitDist);
            frustumCornersWS[i + 4] = XMVectorAdd(frustumCornersWS[i], farCornerRay);
            frustumCornersWS[i] = XMVectorAdd(frustumCornersWS[i], nearCornerRay);
        }

        // Calculate the centroid of the view frustum slice
        XMVECTOR frustumCenterVec = XMVectorZero();
        for(uint32 i = 0; i < 8; ++i)
            frustumCenterVec = XMVectorAdd(frustumCenterVec, frustumCornersWS[i]);
        frustumCenterVec = XMVectorScale(frustumCenterVec, 1.0f / 8.0f);
        Float3 frustumCenter = frustumCenterVec;

        // Pick the up vector to use for the light camera
        Float3 upDir = camera.Right();

        Float3 minExtents;
        Float3 maxExtents;

        {
            // Create a temporary view matrix for the light
            Float3 lightCameraPos = frustumCenter;
            Float3 lookAt = frustumCenter - AppSettings::LightDirection;
            XMMATRIX lightView = XMMatrixLookAtLH(lightCameraPos.ToSIMD(), lookAt.ToSIMD(), upDir.ToSIMD());

            // Calculate an AABB around the frustum corners
            XMVECTOR mins = XMVectorSet(REAL_MAX, REAL_MAX, REAL_MAX, REAL_MAX);
            XMVECTOR maxes = XMVectorSet(-REAL_MAX, -REAL_MAX, -REAL_MAX, -REAL_MAX);
            for(uint32 i = 0; i < 8; ++i)
            {
                XMVECTOR corner = XMVector3TransformCoord(frustumCornersWS[i], lightView);
                mins = XMVectorMin(mins, corner);
                maxes = XMVectorMax(maxes, corner);
            }

            minExtents = mins;
            maxExtents = maxes;
        }

        // Adjust the min/max to accommodate the filtering size
        float scale = (ShadowMapSize + FilterSize) / static_cast<float>(ShadowMapSize);
        minExtents.x *= scale;
        minExtents.y *= scale;
        maxExtents.x *= scale;
        maxExtents.x *= scale;

        Float3 cascadeExtents = maxExtents - minExtents;

        // Get position of the shadow camera
        Float3 shadowCameraPos = frustumCenter + AppSettings::LightDirection.Value() * -minExtents.z;

        // Come up with a new orthographic camera for the shadow caster
        OrthographicCamera shadowCamera(minExtents.x, minExtents.y, maxExtents.x,
            maxExtents.y, 0.0f, cascadeExtents.z);
        shadowCamera.SetLookAt(shadowCameraPos, frustumCenter, upDir);

        // Draw the mesh with depth only, using the new shadow camera
        RenderDepth(context, shadowCamera, world, true);

        // Apply the scale/offset matrix, which transforms from [-1,1]
        // post-projection space to [0,1] UV space
        XMMATRIX texScaleBias;
        texScaleBias.r[0] = XMVectorSet(0.5f,  0.0f, 0.0f, 0.0f);
        texScaleBias.r[1] = XMVectorSet(0.0f, -0.5f, 0.0f, 0.0f);
        texScaleBias.r[2] = XMVectorSet(0.0f,  0.0f, 1.0f, 0.0f);
        texScaleBias.r[3] = XMVectorSet(0.5f,  0.5f, 0.0f, 1.0f);
        XMMATRIX shadowMatrix = shadowCamera.ViewProjectionMatrix().ToSIMD();
        shadowMatrix = XMMatrixMultiply(shadowMatrix, texScaleBias);

        // Store the split distance in terms of view space depth
        const float clipDist = camera.FarClip() - camera.NearClip();
        _meshPSConstants.Data.CascadeSplits[cascadeIdx] = camera.NearClip() + splitDist * clipDist;

        if(cascadeIdx == 0)
        {
            c0Extents = cascadeExtents;
            c0Matrix = shadowMatrix;
            _meshPSConstants.Data.ShadowMatrix = XMMatrixTranspose(shadowMatrix);
            _meshPSConstants.Data.CascadeOffsets[0] = Float4(0.0f, 0.0f, 0.0f, 0.0f);
            _meshPSConstants.Data.CascadeScales[0] = Float4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        else
        {
            // Calculate the position of the lower corner of the cascade partition, in the UV space
            // of the first cascade partition
            Float4x4 invCascadeMat = Float4x4::Invert(shadowMatrix);
            Float3 cascadeCorner = Float3::Transform(Float3(0.0f, 0.0f, 0.0f), invCascadeMat);
            cascadeCorner = Float3::Transform(cascadeCorner, c0Matrix);

            // Do the same for the upper corner
            Float3 otherCorner = Float3::Transform(Float3(1.0f, 1.0f, 1.0f), invCascadeMat);
            otherCorner = Float3::Transform(otherCorner, c0Matrix);

            // Calculate the scale and offset
            Float3 cascadeScale = Float3(1.0f, 1.0f, 1.f) / (otherCorner - cascadeCorner);
            _meshPSConstants.Data.CascadeOffsets[cascadeIdx] = Float4(-cascadeCorner, 0.0f);
            _meshPSConstants.Data.CascadeScales[cascadeIdx] = Float4(cascadeScale, 1.0f);
        }

        ConvertToEVSM(context, cascadeIdx, _meshPSConstants.Data.CascadeScales[cascadeIdx].To3D());
    }

    // Restore the previous render targets and viewports
    context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, renderTargets, depthStencil);
    context->RSSetViewports(D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE, oldViewports);

    for(uint32 i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        if(renderTargets[i] != NULL)
            renderTargets[i]->Release();
    if(depthStencil != NULL)
        depthStencil->Release();
}

void MeshRenderer::DoSceneObjectModelPartsFrustumTests(const Frustum &frustum, const Camera& camera, bool ignoreNearZ, ModelPartsBound& mesh)
{
	mesh.FrustumTests.clear();
	mesh.NumSuccessfulTests = 0;

	for (uint32 i = 0; i < mesh.BoundingSpheres.size(); ++i)
	{
		const BSphere& sphere = mesh.BoundingSpheres[i];
		uint32 test = TestFrustumSphere(frustum, sphere, ignoreNearZ);
		mesh.FrustumTests.push_back(test);
		mesh.NumSuccessfulTests += test;
	}
}

void MeshRenderer::DoSceneObjectFrustumTest(SceneObject *obj, const Camera &camera, bool ignoreNearZ)
{
	Frustum frustum;
	ComputeFrustum(camera.ViewProjectionMatrix().ToSIMD(), frustum);

	uint32 test = TestFrustumSphere(frustum, *obj->bound->bsphere, ignoreNearZ);
	obj->bound->frustumTest = test > 0;
	
	// early out - do not transform if the scene obj bounding box is not in view
	if (test)
	{
		DoSceneObjectModelPartsFrustumTests(frustum, camera, ignoreNearZ, *obj->bound->modelPartsBound);
	}
}

void MeshRenderer::DoSceneObjectsFrustumTests(const Camera &camera, bool ignoreNearZ)
{
	for (uint64 i = 0; i < _scene->getNumStaticOpaqueObjects(); i++)
	{
		SceneObject *obj = &_scene->getStaticOpaqueObjectsPtr()[i];
		DoSceneObjectFrustumTest(obj, camera, ignoreNearZ);
	}

	for (uint64 i = 0; i < _scene->getNumDynamicOpaueObjects(); i++)
	{
		SceneObject *obj = &_scene->getDynamicOpaqueObjectsPtr()[i];
		DoSceneObjectFrustumTest(obj, camera, ignoreNearZ);
	}
}
