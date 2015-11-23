#pragma once
#include "PCH.h"
#include <InterfacePointers.h>
#include <Graphics//GraphicsTypes.h>

using namespace SampleFramework11;

class Scene;
class LightClusters
{
public:

	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context);
	void SetScene(Scene *scene);
	void AssignLightToClusters();
	void UploadClustersData();

	inline ID3D11ShaderResourceView *getClusterTexSRV() { return _clusterTexShaderView; }
	inline ID3D11ShaderResourceView *getLightIndicesListSRV() { return _lightIndicesList.SRView; }

private:
	static const int CX = 32;
	static const int CY = 16;
	static const int CZ = 32;
	static const int NUM_LIGHTS_PER_CLUSTER_MAX = 10;
	static const int NUM_LIGHT_INDICES_MAX = CX * CY * CZ * NUM_LIGHTS_PER_CLUSTER_MAX;

private:
	struct ClusterData
	{
		uint32 offset;
		uint32 counts;
	};

	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	ID3D11Texture3DPtr _clusterTex;
	ID3D11ShaderResourceViewPtr _clusterTexShaderView;

	StructuredBuffer _lightIndicesList;

	uint32 _numLightIndices;
	uint32 _lightIndices[NUM_LIGHT_INDICES_MAX];
	ClusterData _clusters[CZ][CY][CX];

	Scene *_scene;
};