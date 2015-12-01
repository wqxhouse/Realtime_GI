/*
Author: bxs3514
Date:	2015.11
Manage the probes created by Cubemap(header)
*/

#pragma once

#include "CreateCubemap.h"

class ProbeManager
{
public:
	ProbeManager();

	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context);

	void AddProbe(const Float3 &pos, const Float3 &boxSize);
	uint32 GetProbeNums();
	void GetProbe(CreateCubemap **cubemap, uint32 index);
	int GetNNProbe(CreateCubemap **nearCubemap, const Float3 &objPos);
	RenderTarget2D &GetProbeArray();
	// void GetBlendProbes(std::vector<CreateCubemap> &blendCubemaps, Float3 objPos);

	struct Probe
	{
		Float3 pos;
		Float3 BoxSize;
	};

	std::vector<Probe> _probes;
private:
	float CalDistance(const Float3 &probePos, const Float3 &objPos); //Calculate distance between object and a single probe.
	uint32 CalNN(const Float3 &objPos);
	void CalTwoNN(const Float3 &objPos, uint32 &first, uint32 &second);

	std::vector<Float3> _probePositions;

	RenderTarget2D _cubemapArray;
	ID3D11ShaderResourceViewPtr _cubemapArrRSV;
	D3D11_SHADER_RESOURCE_VIEW_DESC _cubemapArrDESC;

	std::vector<CreateCubemap> _cubemaps;
	uint32 _probeNum = 0;

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;
};

