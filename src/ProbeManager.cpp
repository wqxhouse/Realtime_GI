/*
Author: bxs3514
Date:	2015.11
Manage the probes created by Cubemap
*/

#include <limits>

#include "ProbeManager.h"

ProbeManager::ProbeManager()
{
}

void ProbeManager::Initialize(ID3D11Device *device, ID3D11DeviceContext *context)
{
	_device = device;
	_context = context;

	_cubemapArray.Initialize(device, 128, 128, DXGI_FORMAT_R16G16B16A16_FLOAT, 8, 1, 0, TRUE, FALSE, 72, TRUE);

	_cubemapArrDESC.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	_cubemapArrDESC.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
	_cubemapArrDESC.TextureCubeArray.MipLevels = -1;
	_cubemapArrDESC.TextureCubeArray.MostDetailedMip = 0;
	_cubemapArrDESC.TextureCubeArray.NumCubes = 12;
	_cubemapArrDESC.TextureCubeArray.First2DArrayFace = 0;
	
	DXCall(device->CreateShaderResourceView(_cubemapArray.Texture, &_cubemapArrDESC, &_cubemapArrRSV));
}

void ProbeManager::AddProbe(const Float3 &pos, const Float3 &boxSize)
{
	CreateCubemap newCubemap;
	newCubemap.Initialize(_device, _context);
	newCubemap.SetPosition(pos);
	newCubemap.SetBoxSize(boxSize);
	_cubemaps.push_back(newCubemap);
	_probeNum++;
}

uint32 ProbeManager::GetProbeNums()
{
	return _probeNum;
}

void ProbeManager::GetProbe(CreateCubemap **cubemap, uint32 index)
{
	if (index > _cubemaps.size())
	{
		return;
	}

	*cubemap = &(_cubemaps.at(index));
}


int ProbeManager::GetNNProbe(CreateCubemap **nearCubemap, const Float3 &objPos)
{
	uint32 NNIndex = CalNN(objPos);
	if (NNIndex >= _cubemaps.size())
	{
		return -1;
	}

	*nearCubemap = &_cubemaps.at(NNIndex);
	return NNIndex;
}


//void ProbeManager::GetBlendProbes(std::vector<CreateCubemap> &blendCubemaps, Float3 objPos)
//{
//	uint32 firstIndex, secondIndex;
//	CalTwoNN(objPos, firstIndex, secondIndex);
//
//	blendCubemaps.push_back(_cubemaps.at(firstIndex));
//	if (secondIndex == std::numeric_limits<uint32>::max())
//		blendCubemaps.push_back(_cubemaps.at(firstIndex));
//	else
//		blendCubemaps.push_back(_cubemaps.at(secondIndex));
//}

RenderTarget2D ProbeManager::GetProbeArray()
{
	return _cubemapArray;
}

float ProbeManager::CalDistance(const Float3 &probePos, const Float3 &objPos)
{
	float x = probePos[0] - objPos[0];
	float y = probePos[1] - objPos[1];
	float z = probePos[2] - objPos[2];

	return sqrt(x * x + y * y + z * z);
}


uint32 ProbeManager::CalNN(const Float3 &objPos)
{
	float minDis = FLT_MAX;
	uint32 maxIndex = -1;

	for (uint32 probeIndex = 0; probeIndex < _cubemaps.size(); ++probeIndex)
	{
		float distance = CalDistance(_cubemaps.at(probeIndex).GetPosition(), objPos);
		if (minDis >= distance)
		{
			maxIndex = probeIndex;
			minDis = distance;
		}
	}

	return maxIndex;
}


void ProbeManager::CalTwoNN(const Float3 &objPos, uint32 &first, uint32 &second)
{
	if (_cubemaps.size() == 0) return;

	float minDis[2] = { FLT_MAX, FLT_MAX };
	uint32 firstIndex = -1, secondIndex = -1;

	for (uint32 probeIndex = 0; probeIndex < _cubemaps.size(); ++probeIndex)
	{
		float distance = CalDistance(_cubemaps.at(probeIndex).GetPosition(), objPos);
		
		if (distance < minDis[0])
		{
			minDis[1] = minDis[0];
			minDis[0] = distance;
			secondIndex = firstIndex;
			firstIndex = probeIndex;
		}
		else if (distance <= minDis[1])
		{
			minDis[1] = distance;
			secondIndex = probeIndex;
		}
	}

	first = firstIndex;
	second = secondIndex;
}
