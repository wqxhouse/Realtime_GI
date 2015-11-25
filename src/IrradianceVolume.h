#include "PCH.h"

#include <InterfacePointers.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\ShaderCompilation.h>
#include <Graphics\\Camera.h>


using namespace SampleFramework11;

class Scene;
class MeshRenderer;
class DebugRenderer;
class IrradianceVolume
{
public:
	IrradianceVolume();
	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, Camera *camera, DebugRenderer *debugRenderer);
	
	void SetScene(Scene *scene);
	void SetProbeDensity(float unitsBetweenProbes);

	void RenderSceneAtlasGBuffer();
	void RenderSceneAtlasProxyMeshTexcoord();

	const std::vector<Float3> &getPositionList() { return _positionList; }

private:
	void setupResourcesForScene();
	void createCubemapAtlasRTs();

	struct ProxyMeshVSContants
	{
		Float4x4 WorldViewProjection;
	};

	static const int MAX_PROBE_NUM = 16384 / 32; // 16384, texture max height / texcoord height per probe

	ConstantBuffer<ProxyMeshVSContants> _proxyMeshVSConstants;
	VertexShaderPtr _proxyMeshVS;
	PixelShaderPtr  _proxyMeshPS;
	ID3D11InputLayoutPtr _proxyMeshInputLayout;

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	uint32 _cubemapSizeGBuffer;
	uint32 _cubemapSizeTexcoord;
	uint32 _cubemapNum;

	float _unitsBetweenProbes;

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

	// ===============================
	RenderTarget2D _relightCubemapRT;
	RenderTarget2D _dirLightDiffuseBufferRT;
	RenderTarget2D _indirectLightDiffuseBufferRT;

};