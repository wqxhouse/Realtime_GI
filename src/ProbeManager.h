/*
	Author: bxs3514
	Date:	2015.11
	Manage the probes created by Cubemap(header)
*/

#include "CreateCubemap.h"

class ProbeManager
{
public:
	ProbeManager();

	void Initialize(ID3D11Device *device);
	
	void AddSingleProbe(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, Float3 position);
	void AddProbes(const DeviceManager &deviceManager, MeshRenderer *meshRenderer, const Float4x4 &sceneTransform, ID3D11ShaderResourceView *environmentMap,
		const SH9Color &environmentMapSH, const Float2 &jitterOffset, Skybox *skybox, std::vector<Float3> positions);
	void RemoveProbe(uint32 index);
	void RemoveProbes(uint32 start, uint32 end);
	void ClearProbes();

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
};