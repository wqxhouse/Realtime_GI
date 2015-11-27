#pragma once
#include "PCH.h"
#include <InterfacePointers.h>
#include <Graphics//GraphicsTypes.h>
#include "BoundUtils.h"

using namespace SampleFramework11;

class IrradianceVolume;
class Scene;
class LightClusters
{
public:

	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, IrradianceVolume *irradianceVolume);
	void SetScene(Scene *scene);
	void AssignLightToClusters();
	void UploadClustersData();

	inline Float3 getClusterScale() { return _clusterScale; }
	inline Float3 getClusterBias() { return _clusterBias; }

	inline ID3D11ShaderResourceView *getClusterTexSRV() { return _clusterTexShaderView; }
	inline ID3D11ShaderResourceView *getLightIndicesListSRV() { return _lightIndicesList.SRView; }

	~LightClusters();

private:
	//static const int CX = 32;
	//static const int CY = 16;
	//static const int CZ = 32;
	//static const int NUM_LIGHTS_PER_CLUSTER_MAX = 10;
	//static const int NUM_LIGHT_INDICES_MAX = CX * CY * CZ * NUM_LIGHTS_PER_CLUSTER_MAX;

private:
	void genClusterResources();
	void assignPointLightToCluster(int index, bool isSHProbeLight, const Float3 &pos, float radius, 
		uint16 *pointLightIndexInCluster, uint16 *shProbeLightIndexInCluster, 
		uint16 *numPointLightsInCluster, uint16 *numSHProbeLightInCluster, Float3 &inv_scale);

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
	uint32 _maxNumLightIndicesPerCluster;
	//uint32 _lightIndices[NUM_LIGHT_INDICES_MAX];
	//ClusterData _clusters[CZ][CY][CX];

	uint32 *_lightIndices;
	ClusterData *_clusters;

	// cluster size
	uint32 _cx;
	uint32 _cy;
	uint32 _cz;
	BBox _clustersWSAABB;

	float _referenceScale;
	uint32 _referenceNumLightIndices;

	Scene *_scene;
	Float3 _clusterScale;
	Float3 _clusterBias;

	IrradianceVolume *_irradianceVolume;
};