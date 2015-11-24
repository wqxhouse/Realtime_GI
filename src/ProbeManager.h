/*
Author: bxs3514
Date:	2015.11
Manage the probes created by Cubemap(header)
*/

#pragma once

#include "CreateCubemap.h"

#define ORIGIN_PROBE 0
#define MAX_PROBE_MANAGER 1024

class ProbeManager
{
public:
	ProbeManager();

	struct CameraClips
	{
		float NearClip;
		float FarClip;
	};

	void Initialize(ID3D11Device *device, const std::vector<CameraClips> cameraClipVector);

	//Create a new probe for the cubemap queue from on the index position.
	void CreateProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position, uint32 index);
	//Create new probes for the cubemap queue from start index to end index.
	void CreateProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions);

	void AddProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position, const CameraClips cameraClips);
	void AddProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions, const std::vector<CameraClips> cameraClips);

	void RemoveProbe(uint32 index);
	void RemoveProbes(uint32 start, uint32 end);
	void ClearProbes();

	uint32 GetProbeNums();
	void GetProbe(CreateCubemap &cubemap, uint32 index);
	void GetNNProbe(CreateCubemap &nearCubemap, Float3 objPos);
	void GetBlendProbe(CreateCubemap &blendCubemap);

	~ProbeManager();
private:
	float CalDistance(Float3 probePos, Float3 objPos);//Calculate distance between object and a single probe.
	uint32 CalNN(Float3 objPos);
	void BlendCubemaps(Float3 objPos);

	CreateCubemap *_resCubemap = nullptr;

	std::vector<Float3> _probePositions;
	std::vector<CreateCubemap> _cubemaps;
	std::vector<CameraClips> _cameraClipVector;

	uint32 probeNum = 0;
};

