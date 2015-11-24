#include "PCH.h"

#include <InterfacePointers.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\Camera.h>


using namespace SampleFramework11;

class Scene;
class MeshRenderer;
class DebugRenderer;
class IrradianceVolume
{
public:
	IrradianceVolume();
	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, MeshRenderer *meshRenderer, DebugRenderer *debugRenderer);
	
	void SetScene(Scene *scene);
	void SetProbeDensity(float unitsBetweenProbes);

	void RenderSceneAtlasGBuffer();
	void RenderSceneAtlasProxyGeomTexcoord();

	const std::vector<Float3> &getPositionList() { return _positionList; }

private:
	void setupResourcesForScene();
	void createCubemapAtlasRTs();

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	uint32 _cubemapSizeGBuffer;
	uint32 _cubemapSizeTexcoord;
	uint32 _cubemapNum;

	float _unitsBetweenProbes;

	RenderTarget2D _albedoCubemapRT;
	RenderTarget2D _normalCubemapRT;
	RenderTarget2D _proxyGeomTexCoordCubemapRT;
	DepthStencilBuffer _depthBufferGBufferRT;
	DepthStencilBuffer _depthBufferTexcoordRT;

	Scene *_scene;
	PerspectiveCamera _cubemapCamera;
	MeshRenderer *_meshRenderer;
	DebugRenderer *_debugRenderer;

	static const struct CameraStruct
	{
		Float3 LookAt;
		Float3 Up;
	} _CubemapCameraStruct[6];

	std::vector<Float3> _positionList;
};