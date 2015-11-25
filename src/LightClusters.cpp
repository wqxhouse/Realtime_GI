#include "LightClusters.h"
#include "Scene.h"
#include "BoundUtils.h"

LightClusters::~LightClusters()
{
	_clusters != nullptr ? free(_clusters) : 0;
	_lightIndices != nullptr ? free(_lightIndices) : 0;
}

void LightClusters::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_context = context;
	_device = device;
	_scene = nullptr;

	_referenceScale = 0.05f;
	_referenceNumLightIndices = 32 * 8 * 32 * 128;

	_clusters = nullptr;
	_lightIndices = nullptr;
}

void LightClusters::genClusterResources()
{
	const BBox &sceneBound = _scene->getSceneBoundingBox();
	Float3 bMin = Float3(sceneBound.Min) - 1.0f;
	Float3 bMax = Float3(sceneBound.Max) + 1.0f;

	memcpy(&_clustersWSAABB.Max, &bMax, sizeof(XMFLOAT3));
	memcpy(&_clustersWSAABB.Min, &bMin, sizeof(XMFLOAT3));

	//_clustersWSAABB.Max.x = bMax.x;
	//_clustersWSAABB.Max.y = bMax.y;
	//_clustersWSAABB.Max.z = bMax.z;

	//_clustersWSAABB.Min.x = bMin.x;
	//_clustersWSAABB.Min.y = bMin.y;
	//_clustersWSAABB.Min.z = bMin.z;

	Float3 size = bMax - bMin;
	Float3 clusterSize = size * _referenceScale;
	_cx = Max((int)ceilf(clusterSize.x), 4);
	_cy = Max((int)ceilf(clusterSize.y), 4);
	_cz = Max((int)ceilf(clusterSize.z), 4);

	_clusterScale = Float3((float)_cx, (float)_cy, (float)_cz) / size;
	_clusterBias = -_clusterScale * Float3(_clustersWSAABB.Min);

	int dim = _cx * _cy * _cz;
	_clusters = (ClusterData *)realloc(_clusters, sizeof(ClusterData) * dim);
	_lightIndices = (uint32 *)realloc(_lightIndices, sizeof(uint32) * _referenceNumLightIndices);

	_maxNumLightIndicesPerCluster = _referenceNumLightIndices / dim;
	_lightIndicesList.Initialize(_device, sizeof(uint32), _referenceNumLightIndices, true);

	// Allocate 3D Texture for clusters
	{
		D3D11_TEXTURE3D_DESC desc;
		desc.Width = _cx;
		desc.Height = _cy;
		desc.Depth = _cz;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R32G32_UINT;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = 0;
		HRESULT hr = _device->CreateTexture3D(&desc, NULL, &_clusterTex);
		assert(SUCCEEDED(hr));

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

		srvDesc.Format = DXGI_FORMAT_R32G32_UINT;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
		srvDesc.Texture3D.MostDetailedMip = 0;
		srvDesc.Texture3D.MipLevels = 1;

		hr = _device->CreateShaderResourceView(_clusterTex, &srvDesc, &_clusterTexShaderView);
		assert(SUCCEEDED(hr));
	}
}

void LightClusters::SetScene(Scene *scene)
{
	_scene = scene;

	genClusterResources();
}

void LightClusters::AssignLightToClusters()
{
	if (_scene == nullptr) return;

	int dim = _cx * _cy * _cz;

	// Reset clusters and indices
	memset(&_clusters[0], 0, sizeof(ClusterData) * dim);
	memset(&_lightIndices[0], 0, sizeof(uint32) * _referenceNumLightIndices);
	_numLightIndices = 0;

	// PointLights - // [CZ][CY][CX][NUM_LIGHTS_PER_CLUSTER_MAX];
	uint16 *pointLightIndexInCluster = (uint16 *)calloc(dim * _maxNumLightIndicesPerCluster, sizeof(uint16));
	uint16 *numPointLightsInCluster = (uint16 *)calloc(dim, sizeof(uint16)); // [CZ][CY][CX]; // TODO: chekc uint16 is enough
	//memset(pointLightIndexInCluster, 0, sizeof(pointLightIndexInCluster));
	//memset(numPointLightsInCluster, 0, sizeof(numPointLightsInCluster));

	Float3 inv_scale = Float3(1.0f) / _clusterScale;

	// Point light assignment
	for (int i = 0; i < _scene->getNumPointLights(); i++)
	{
		PointLight *pl = &_scene->getPointLightPtr()[i];

		Float3 posBoundCoord = (pl->cPos - _clustersWSAABB.Min);
	
		// Assert_(posBoundCoord.x >= 0.0f && posBoundCoord.y >= 0.0f && posBoundCoord.z >= 0.0f);
		if (posBoundCoord.x < 0.0f && posBoundCoord.y < 0.0f && posBoundCoord.z < 0.0f)
		{
			continue; // skip lights that are out of bounds
		}

		Float3 posClusterCoord = posBoundCoord * _clusterScale;
		Float3 minPosClusterCoord = (posBoundCoord - pl->cRadius) * _clusterScale;
		Float3 maxPosClusterCoord = (posBoundCoord + pl->cRadius) * _clusterScale;
		
		Uint3 posClusterIntCoord = Uint3((uint32)floorf(posClusterCoord.x), (uint32)floorf(posClusterCoord.y), (uint32)floorf(posClusterCoord.z));

		uint32 posXClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.x, 0.0f));
		uint32 posXClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.x, (float)_cx));
		uint32 posYClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.y, 0.0f));
		uint32 posYClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.y, (float)_cy));
		uint32 posZClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.z, 0.0f));
		uint32 posZClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.z, (float)_cz));

		Uint3 minPosClusterIntCoord = Uint3(posXClusterIntCoordMin, posYClusterIntCoordMin, posZClusterIntCoordMin);
		Uint3 maxPosClusterIntCoord = Uint3(posXClusterIntCoordMax, posYClusterIntCoordMax, posZClusterIntCoordMax);

		const float radius_sqr = pl->cRadius * pl->cRadius;

		for (uint32 z = minPosClusterIntCoord.z; z < maxPosClusterIntCoord.z; z++)
		{
			float dz = (posClusterIntCoord.z == z) ? 0.0f : _clustersWSAABB.Min.z + (posClusterIntCoord.z < z ? z : z + 1) * inv_scale.z - pl->cPos.z;
			dz *= dz;

			for (uint32 y = minPosClusterIntCoord.y; y < maxPosClusterIntCoord.y; y++)
			{
				float dy = (posClusterIntCoord.y == y) ? 0.0f : _clustersWSAABB.Min.y + (posClusterIntCoord.y < y ? y : y + 1) * inv_scale.y - pl->cPos.y;
				dy *= dy;
				dy += dz;

				for (uint32 x = minPosClusterIntCoord.x; x < maxPosClusterIntCoord.x; x++)
				{
					float dx = (posClusterIntCoord.x == x) ? 0.0f : _clustersWSAABB.Min.x + (posClusterIntCoord.x < x ? x : x + 1) * inv_scale.x - pl->cPos.x;
					dx *= dx;
					dx += dy;

					if (dx < radius_sqr)
					{
						int clusterIndex = z * _cy * _cx + y * _cx + x;

						// Assert_(numPointLightsInCluster[z][y][x] < NUM_LIGHTS_PER_CLUSTER_MAX);
						// if (numPointLightsInCluster[z][y][x] >= _maxNumLightIndicesPerCluster)
						if (numPointLightsInCluster[clusterIndex] >= _maxNumLightIndicesPerCluster)
						{
							printf("numPointLightInCluster reached maximum\n");
							continue;
						}

						//int curPtCount = _clusters[clusterIndex].counts >> 16;
						//int curPtCount = _clusters[z][y][x].counts >> 16;
						//curPtCount++;

						// clear original 
						//_clusters[z][y][x].counts &= 0xFFFF;
						//_clusters[z][y][x].counts |= (curPtCount << 16);

						/*_clusters[clusterIndex].counts &= 0xFFFF;
						_clusters[clusterIndex].counts |= (curPtCount << 16);*/

						_clusters[clusterIndex].counts++;

						int numPt = numPointLightsInCluster[clusterIndex]++;

						int pointLightIndexInClusterIndex = 
							z * _cy * _cx * _maxNumLightIndicesPerCluster +
							y * _cx * _maxNumLightIndicesPerCluster + 
							x * _maxNumLightIndicesPerCluster + numPt;
						// pointLightIndexInCluster[z][y][x][numPointLightsInCluster[z][y][x]++] = i;
						pointLightIndexInCluster[pointLightIndexInClusterIndex] = i;
					}
				}
			}
		}
	}

	int curLightIndicesArrIndex = 0;
	for (uint32 z = 0; z < _cz; z++)
	{
		for (uint32 y = 0; y < _cy; y++)
		{
			for (uint32 x = 0; x < _cx; x++)
			{
				uint32 clusterIndex = z * _cy * _cx + y * _cx + x;

				// TODO: make this work with spot light
				// _clusters[z][y][x].offset = curLightIndicesArrIndex;
				_clusters[clusterIndex].offset = curLightIndicesArrIndex;

				// for (int k = 0; k < numPointLightsInCluster[z][y][x]; k++)
				for (int k = 0; k < numPointLightsInCluster[clusterIndex]; k++)
				{
					int pointLightIndexInClusterIndex = z * _cy * _cx * _maxNumLightIndicesPerCluster
						+ y * _cx * _maxNumLightIndicesPerCluster
						+ x * _maxNumLightIndicesPerCluster
						+ k;

					// _lightIndices[curLightIndicesArrIndex++] = pointLightIndexInCluster[z][y][x][k];
					_lightIndices[curLightIndicesArrIndex++] = pointLightIndexInCluster[pointLightIndexInClusterIndex];
				}
			}
		}
	}

	_numLightIndices = curLightIndicesArrIndex;

	free(numPointLightsInCluster);
	free(pointLightIndexInCluster);
}

void LightClusters::UploadClustersData()
{
	if (_scene == nullptr) return;

	D3D11_MAPPED_SUBRESOURCE mappedResource;

	// Upload lightIndices structured buffer
	BYTE* mappedData = NULL;
	_context->Map(_lightIndicesList.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	memcpy(mappedData, _lightIndices, sizeof(uint32) * _numLightIndices);
	_context->Unmap(_lightIndicesList.Buffer, 0);

	// Upload 3D Texture
	_context->Map(_clusterTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	mappedData = reinterpret_cast<BYTE*>(mappedResource.pData);
	for (uint32 z = 0; z < _cz; z++)
	{
		for (uint32 y = 0; y < _cy; y++)
		{
			int clusterIndex = z * _cy * _cx + y * _cx;
			// TODO: check for bug
			memcpy(mappedData + z * mappedResource.DepthPitch
				+ y * mappedResource.RowPitch,
				// _clusters[z][y], _cx* sizeof(ClusterData));
				&_clusters[clusterIndex], _cx * sizeof(ClusterData));
		}
	}
	_context->Unmap(_clusterTex, 0);
}