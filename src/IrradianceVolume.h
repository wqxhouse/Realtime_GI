#include "PCH.h"

#include <InterfacePointers.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\DeviceStates.h>
#include <Graphics\\Camera.h>

#include "Light.h"
#include "LightClusters.h"

using namespace SampleFramework11;

class Scene;
class MeshRenderer;
class DebugRenderer;
class IrradianceVolume
{
public:
	IrradianceVolume();
	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, Camera *camera, 
		StructuredBuffer *_pointLightBuffer, LightClusters *clusters, DebugRenderer *debugRenderer);
	
	void SetScene(Scene *scene);
	void SetProbeDensity(float unitsBetweenProbes);

	void RenderSceneAtlasGBuffer();
	void RenderSceneAtlasProxyMeshTexcoord();

	void Update();
	void MainRender();

	const std::vector<SHProbeLight> &getSHProbeLights() { return _probeLights; }

	const std::vector<Float3> &getPositionList() { return _positionList; }

	StructuredBuffer *getRelightSHStructuredBufferPtr() { return &_relightSHBuffer; }

	static const int MAX_PROBE_NUM = 16384 / 32; // 16384, texture max height / texcoord height per probe

private:
	void setupResourcesForScene();
	void createCubemapAtlasRTs();
	void createSHComputeBuffers();
	void renderProxyModel();

	struct ProxyMeshVSContants
	{
		Float4x4 WorldViewProjection;
	};

	ConstantBuffer<ProxyMeshVSContants> _proxyMeshVSConstants;
	VertexShaderPtr _proxyMeshVS;
	PixelShaderPtr  _proxyMeshPS;
	ID3D11InputLayoutPtr _proxyMeshInputLayout;

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	uint32 _cubemapSizeGBuffer;
	uint32 _cubemapSizeTexcoord;
	uint32 _cubemapNum;

	float _refUnitsBetweenProbes;
	float _calculatedUnitsBetweenProbes;

	RenderTarget2D _albedoCubemapRT;
	RenderTarget2D _normalCubemapRT;
	RenderTarget2D _proxyMeshTexCoordCubemapRT;
	DepthStencilBuffer _depthBufferGBufferRT;
	DepthStencilBuffer _depthBufferTexcoordRT;

	Scene *_scene;
	PerspectiveCamera _cubemapCamera;
	MeshRenderer *_meshRenderer;
	DebugRenderer *_debugRenderer;

	Camera *_mainCamera;

	static const struct CameraStruct
	{
		Float3 LookAt;
		Float3 Up;
	} _CubemapCameraStruct[6];

	std::vector<Float3> _positionList;

private:
	// ===============================
	void renderProxyMeshDirectLighting();
	void renderProxyMeshShadowMap();
	void renderRelightCubemap();

	RenderTarget2D _relightCubemapRT;
	RenderTarget2D _dirLightDiffuseBufferRT;
	RenderTarget2D _indirectLightDiffuseBufferRT;

	DepthStencilBuffer _dirLightProxyMeshShadowMapBuffer;
	ID3D11InputLayoutPtr _proxyMeshDepthInputLayout;

    RasterizerStates _rasterizerStates;
	DepthStencilStates _depthStencilStates;
	SamplerStates _samplerStates;
	
    VertexShaderPtr _meshDepthVS;

	VertexShaderPtr _dirLightDiffuseVS;
	PixelShaderPtr _dirLightDiffusePS;

	VertexShaderPtr _relightCubemapVS;
	PixelShaderPtr _relightCubemapPS;

	struct DirectDiffuseConstants
	{
		Float4x4 ModelToWorld;
		Float4x4 WorldToLightProjection;

		Float4Align Float3 ClusterScale;
		Float4Align Float3 ClusterBias;
	};

	ConstantBuffer<DirectDiffuseConstants> _directDiffuseConstants;

	OrthographicCamera _dirLightCam;

	LightClusters *_clusters;
	StructuredBuffer *_pointLightBuffer;

	uint32 _dirLightMapSize;
	uint32 _indirectLightMapSize;

	// SH /////////////////////
	void IntegrateSH();

	struct PackedSH9
	{
		Float4 chunk0;
		Float4 chunk1;
		Float4 chunk2;
	};

	struct PaddedSH9Color
	{
		Float4 sh9[9];
	};
	
	struct SHIntegrationConstants
	{
		Float4Align float FinalWeight;
	};
	
	float _weightSum;

	// TODO: calc actual world matrix in the shader to save upload and memory cost
	StructuredBuffer _viewToWorldMatrixPalette;

	ConstantBuffer<SHIntegrationConstants> _shIntegrationConstants;

	ComputeShaderPtr _relightSHIntegrateCS;
	ComputeShaderPtr _relightSHReductionCS;

	StructuredBuffer _relightIntegrationBuffer;
	StructuredBuffer _relightSHBuffer;

	std::vector<SHProbeLight> _probeLights;
};