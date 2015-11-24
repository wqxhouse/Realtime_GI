#include "LightClusters.h"
#include "Scene.h"
#include "BoundUtils.h"

void LightClusters::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_context = context;
	_device = device;
	_scene = nullptr;

	_lightIndicesList.Initialize(_device, sizeof(uint32), NUM_LIGHT_INDICES_MAX, true);

	// Allocate 3D Texture for clusters
	{
		D3D11_TEXTURE3D_DESC desc;
		desc.Width = CX;
		desc.Height = CY;
		desc.Depth = CZ;
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
}

void LightClusters::AssignLightToClusters()
{
	if (_scene == nullptr) return;

	// Reset clusters and indices
	memset(_clusters, 0, sizeof(_clusters));
	memset(_lightIndices, 0, sizeof(_lightIndices));
	_numLightIndices = 0;

	// PointLights
	uint16 pointLightIndexInCluster[CZ][CY][CX][NUM_LIGHTS_PER_CLUSTER_MAX];
	uint16 numPointLightsInCluster[CZ][CY][CX];
	memset(pointLightIndexInCluster, 0, sizeof(pointLightIndexInCluster));
	memset(numPointLightsInCluster, 0, sizeof(numPointLightsInCluster));

	const BBox &sceneBound = _scene->getSceneBoundingBox();
	Float3 bMin = Float3(sceneBound.Min);
	Float3 bMax = Float3(sceneBound.Max);
	Float3 size = bMax - bMin;

	Float3 scale = Float3((float)CX, (float)CY, (float)CZ) / size;
	Float3 inv_scale = Float3(1.0f) / scale;

	// Point light assignment
	for (int i = 0; i < _scene->getNumPointLights(); i++)
	{
		PointLight *pl = &_scene->getPointLightPtr()[i];

		Float3 posBoundCoord = (pl->cPos - bMin);
	
		// Assert_(posBoundCoord.x >= 0.0f && posBoundCoord.y >= 0.0f && posBoundCoord.z >= 0.0f);
		if (posBoundCoord.x >= 0.0f && posBoundCoord.y >= 0.0f && posBoundCoord.z >= 0.0f)
		{
			continue; // skip lights that are out of bounds
		}

		Float3 posClusterCoord = posBoundCoord * scale;
		Float3 minPosClusterCoord = (posBoundCoord - pl->cRadius) * scale;
		Float3 maxPosClusterCoord = (posBoundCoord + pl->cRadius) * scale;
		
		Uint3 posClusterIntCoord = Uint3((uint32)floorf(posClusterCoord.x), (uint32)floorf(posClusterCoord.y), (uint32)floorf(posClusterCoord.z));

		uint32 posXClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.x, 0.0f));
		uint32 posXClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.x, (float)CX));
		uint32 posYClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.y, 0.0f));
		uint32 posYClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.y, (float)CY));
		uint32 posZClusterIntCoordMin = (uint32)floorf(Max(minPosClusterCoord.z, 0.0f));
		uint32 posZClusterIntCoordMax = (uint32)ceilf(Min(maxPosClusterCoord.z, (float)CZ));

		Uint3 minPosClusterIntCoord = Uint3(posXClusterIntCoordMin, posYClusterIntCoordMin, posZClusterIntCoordMin);
		Uint3 maxPosClusterIntCoord = Uint3(posXClusterIntCoordMax, posYClusterIntCoordMax, posZClusterIntCoordMax);

		const float radius_sqr = pl->cRadius * pl->cRadius;

		for (uint32 z = minPosClusterIntCoord.z; z < maxPosClusterIntCoord.z; z++)
		{
			float dz = (posClusterIntCoord.z == z) ? 0.0f : bMin.z + (posClusterIntCoord.z < z ? z : z + 1) * inv_scale.z - pl->cPos.z;
			dz *= dz;

			for (uint32 y = minPosClusterIntCoord.y; y < maxPosClusterIntCoord.y; y++)
			{
				float dy = (posClusterIntCoord.y == y) ? 0.0f : bMin.y + (posClusterIntCoord.y < y ? y : y + 1) * inv_scale.y - pl->cPos.y;
				dy *= dy;
				dy += dz;

				for (uint32 x = minPosClusterIntCoord.x; x < maxPosClusterIntCoord.x; x++)
				{
					float dx = (posClusterIntCoord.x == x) ? 0.0f : bMin.x + (posClusterIntCoord.x < x ? x : x + 1) * inv_scale.x - pl->cPos.x;
					dx *= dx;
					dx += dy;

					if (dx < radius_sqr)
					{
						Assert_(numPointLightsInCluster[z][y][x] < NUM_LIGHTS_PER_CLUSTER_MAX);

						int curPtCount = _clusters[z][y][x].counts >> 16;
						curPtCount++;

						// clear original 
						_clusters[z][y][x].counts &= 0xFFFF;
						_clusters[z][y][x].counts |= (curPtCount << 16);

						pointLightIndexInCluster[z][y][x][numPointLightsInCluster[z][y][x]++] = i;
					}
				}
			}
		}
	}

	int curLightIndicesArrIndex = 0;
	for (int z = 0; z < CZ; z++)
	{
		for (int y = 0; y < CY; y++)
		{
			for (int x = 0; x < CX; x++)
			{
				// TODO: make this work with spot light
				_clusters[z][y][x].offset = curLightIndicesArrIndex;

				for (int k = 0; k < numPointLightsInCluster[z][y][x]; k++)
				{
					_lightIndices[curLightIndicesArrIndex++] = pointLightIndexInCluster[z][y][x][k];
				}
			}
		}
	}

	_numLightIndices = curLightIndicesArrIndex;
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
	for (int z = 0; z < CZ; z++)
	{
		for (int y = 0; y < CY; y++)
		{
			memcpy(mappedData + z * mappedResource.DepthPitch
				+ y * mappedResource.RowPitch,
				_clusters[z][y], CX * sizeof(ClusterData));
		}
	}
	_context->Unmap(_clusterTex, 0);
}