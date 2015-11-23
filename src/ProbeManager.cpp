/*
	Author: bxs3514
	Date:	2015.11
	Manage the probes created by Cubemap
*/


#include "ProbeManager.h"


ProbeManager::ProbeManager()
{

}


void ProbeManager::Initialize(ID3D11Device *device)
{
	
}


void ProbeManager::AddSingleProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position)
{

}


void ProbeManager::AddProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions)
{

}


void ProbeManager::RemoveProbe(uint32 index)
{
	if (index >= 0 && index <_cubemaps.size())
		_cubemaps.erase(_cubemaps.begin() + index);
}


void ProbeManager::RemoveProbes(uint32 start, uint32 end)
{
	if (start >= 0 && end < _cubemaps.size() && start <= end)
		_cubemaps.erase(_cubemaps.begin() + start, _cubemaps.begin() + end);
}


void ProbeManager::ClearProbes()
{
	_cubemaps.clear();
}


void ProbeManager::GetProbe(CreateCubemap &cubemap, uint32 index)
{
	cubemap = _cubemaps.at(index);
}


void ProbeManager::GetNNProbe(CreateCubemap &nearCubemap, Float3 objPos)
{

}


void ProbeManager::GetBlendProbe(CreateCubemap &blendCubemap)
{

}

float ProbeManager::CalDistance(Float3 probePos, Float3 objPos)
{
	float x = probePos[0] - objPos[0];
	float y = probePos[1] - objPos[1];
	float z = probePos[2] - objPos[2];

	return sqrt(x * x + y * y + z * z);
}


uint32 ProbeManager::CalNN(Float3 objPos)
{
	float maxDis = 0;
	uint32 maxIndex = -1;

	for (uint32 probeIndex = 0; probeIndex < _cubemaps.size(); ++probeIndex)
	{
		if (maxDis < CalDistance(_cubemaps.at(probeIndex).GetPosition(), objPos))
			maxIndex = probeIndex;
	}

	return maxIndex;
}


void ProbeManager::BlendCubemaps(Float3 objPos)
{

}


ProbeManager::~ProbeManager()
{
	if (_resCubemap != nullptr)
	{
		delete _resCubemap;
		_resCubemap = nullptr;
	}

	ClearProbes();
}