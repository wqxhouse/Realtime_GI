/*
Author: bxs3514
Date:	2015.11
Manage the probes created by Cubemap
*/


#include "ProbeManager.h"

#define DefaultNearClip 0.01f
#define DefaultFarClip 300.0f


ProbeManager::ProbeManager()
{

}

void ProbeManager::Initialize(ID3D11Device *device, const std::vector<CameraClips> cameraClipVector)
{
	if (cameraClipVector.size() == 0) return;
	uint32 probeNum = (uint32)cameraClipVector.size();

	for (uint32 probeIndex = 0; probeIndex < probeNum; ++probeIndex)
	{
		CreateCubemap newCubemap = CreateCubemap(cameraClipVector.at(probeIndex).NearClip,
			cameraClipVector.at(probeIndex).FarClip);
		newCubemap.Initialize(device);
		_cubemaps.push_back(newCubemap);
	}
}


void ProbeManager::CreateProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position, uint32 index = 0)
{
	//if (probeNum == 0) return;
	CreateCubemap *selectedCubemap = &_cubemaps.at(index);
	RenderTarget2D cubemapRenderTarget;

	selectedCubemap->SetPosition(position);

	selectedCubemap->Create(deviceManager, meshRenderer, sceneTransform, environmentMap,
		environmentMapSH, jitterOffset, skybox);
	selectedCubemap->GetTargetViews(cubemapRenderTarget);
	deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);

	selectedCubemap->RenderPrefilterCubebox(deviceManager, sceneTransform);
	selectedCubemap->GetPreFilterTargetViews(cubemapRenderTarget);
	deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);

	++probeNum;
	selectedCubemap = nullptr;
}

void ProbeManager::CreateProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions)
{
	if (positions.size() == 0) return;

	CreateCubemap *selectedCubemap = nullptr;
	RenderTarget2D cubemapRenderTarget;
	probeNum = (uint32)positions.size();

	for (uint32 probeIndex = 0; probeIndex < probeNum; ++probeIndex)
	{
		selectedCubemap = &_cubemaps.at(probeIndex);

		selectedCubemap->SetPosition(positions.at(probeIndex));

		selectedCubemap->Create(deviceManager, meshRenderer, sceneTransform, environmentMap,
			environmentMapSH, jitterOffset, skybox);
		selectedCubemap->GetTargetViews(cubemapRenderTarget);
		deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);

		selectedCubemap->RenderPrefilterCubebox(deviceManager, sceneTransform);
		selectedCubemap->GetPreFilterTargetViews(cubemapRenderTarget);
		deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);
	}

	selectedCubemap = nullptr;
}

void ProbeManager::AddProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position, const CameraClips cameraClips)
{
	CreateCubemap newCubemap = CreateCubemap(cameraClips.NearClip, cameraClips.FarClip);
	newCubemap.Initialize(deviceManager.Device());

	RenderTarget2D cubemapRenderTarget;
	newCubemap.SetPosition(position);

	newCubemap.Create(deviceManager, meshRenderer, sceneTransform, environmentMap,
		environmentMapSH, jitterOffset, skybox);
	newCubemap.GetTargetViews(cubemapRenderTarget);
	deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);

	newCubemap.RenderPrefilterCubebox(deviceManager, sceneTransform);
	newCubemap.GetPreFilterTargetViews(cubemapRenderTarget);
	deviceManager.ImmediateContext()->GenerateMips(cubemapRenderTarget.SRView);

	_cubemaps.push_back(newCubemap);
	++probeNum;
}


void ProbeManager::AddProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
	const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions, const std::vector<CameraClips> cameraClips)
{
	size_t posSize = positions.size();
	size_t clipsSize = cameraClips.size();

	for (size_t probeIndex = 0; probeIndex < posSize; ++probeIndex)
	{
		if (probeIndex < clipsSize)
			AddProbe(deviceManager, meshRenderer, sceneTransform, environmentMap, environmentMapSH, jitterOffset, skybox, positions.at(probeIndex), cameraClips.at(probeIndex));
		else
		{
			CameraClips tempCameraClips;
			tempCameraClips.NearClip = DefaultNearClip;
			tempCameraClips.FarClip = DefaultFarClip;
			AddProbe(deviceManager, meshRenderer, sceneTransform, environmentMap, environmentMapSH, jitterOffset, skybox, positions.at(probeIndex), tempCameraClips);
		}

	}

}


void ProbeManager::RemoveProbe(uint32 index)
{
	if (probeNum == 0) return;
	if (index >= 0 && index <_cubemaps.size())
		_cubemaps.erase(_cubemaps.begin() + index);
}


void ProbeManager::RemoveProbes(uint32 start, uint32 end)
{
	if (probeNum == 0) return;
	if (start >= 0 && end < _cubemaps.size() && start <= end)
		_cubemaps.erase(_cubemaps.begin() + start, _cubemaps.begin() + end);
}


void ProbeManager::ClearProbes()
{
	_probePositions.clear();
	_cubemaps.clear();
	probeNum = 0;
}


uint32 ProbeManager::GetProbeNums()
{
	return probeNum;
}


void ProbeManager::GetProbe(CreateCubemap &cubemap, uint32 index)
{
	cubemap = _cubemaps.at(index);
}


void ProbeManager::GetNNProbe(CreateCubemap &nearCubemap, Float3 objPos)
{
	uint32 NNIndex = CalNN(objPos);
	nearCubemap = _cubemaps.at(NNIndex);
}


void ProbeManager::GetBlendProbes(std::vector<CreateCubemap> &blendCubemaps, Float3 objPos)
{
	uint32 firstIndex, secondIndex;
	CalTwoNN(objPos, firstIndex, secondIndex);

	blendCubemaps.push_back(_cubemaps.at(firstIndex));
	blendCubemaps.push_back(_cubemaps.at(secondIndex));
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


void ProbeManager::CalTwoNN(Float3 objPos, uint32 &first, uint32 &second)
{
	if (_cubemaps.size() == 0) return;

	float minDis[2] = { FLT_MAX, FLT_MAX };
	uint32 firstIndex = -1, secondIndex = -1;

	for (uint32 probeIndex = 0; probeIndex < _cubemaps.size(); ++probeIndex)
	{
		float distance = CalDistance(_cubemaps.at(probeIndex).GetPosition(), objPos);
		if (probeIndex == 0)
		{
			minDis[0] = minDis[1] = distance;
			firstIndex = secondIndex = 0;
		}
		else{
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
	}

	first = firstIndex;
	second = secondIndex;
}


ProbeManager::~ProbeManager()
{
	ClearProbes();
}