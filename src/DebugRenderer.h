#pragma once
#include "PCH.h"
#include <Graphics/Model.h>
#include <Graphics/ShaderCompilation.h>
#include <Graphics/DeviceStates.h>
#include <Graphics/GraphicsTypes.h>
#include <Graphics/DeviceManager.h>
#include <Graphics/Camera.h>
#include <Graphics/SpriteRenderer.h>
#include <Graphics/Model.h>
#include <Utility.h>
#include "BoundUtils.h"

using namespace SampleFramework11;

class DebugRenderer
{
public:
	void Initialize(DeviceManager *deviceManager, ID3D11Device *device, ID3D11DeviceContext *context, Camera *cam);
	void QueueCubeWire(const Float3 &pos, const Float3 &halfLength, const Float4 &color);
	void QueueCubeTranslucent(const Float3 &pos, const Float3 &halfLength, const Float4 &color);
	void QueueBBoxWire(const BBox &bbox, const Float4 &color);
	void QueueBBoxTranslucent(const BBox &bbox, const Float4 &color);
	void QueueSprite(ID3D11ShaderResourceView *texture, const Float3 &pos, const Float4 &color);
	void QueueLightSphere(const Float3 &pos, const Float4 &color, float radius);

	void FlushDrawQueued();

private:
	static const int _maxBatchSize = 819;
	struct DebugGeomPerInstanceData
	{
		Float4x4 Transform;
		Float4 Color;
	};

	struct DebugGeomConstants
	{
		DebugGeomPerInstanceData data[_maxBatchSize];
	};

	Camera *_camera;
	DeviceManager *_deviceManager;
	ID3D11DevicePtr _device;
	ID3D11DeviceContextPtr _context;
	ConstantBuffer<DebugGeomConstants> _shaderConstants;
	std::vector<std::tuple<Float4x4, Float4> > _cubeQueueWire;
	std::vector<std::tuple<Float4x4, Float4> > _cubeQueueTranslucent;

	std::vector<std::tuple<ID3D11ShaderResourceView *, SpriteRenderer::SpriteDrawData> > _spriteData;
	std::vector<std::tuple<Float4x4, Float4> > _sphereQueue;

	Float4x4 matModelToWorldFromBBox(const BBox &bbox);
	void updateConstantBufferCube(bool32 isWire);
	void updateConstantBufferSphere();

private:
	static Mesh _cubeMesh;
	static VertexShaderPtr _debugShaderGeomVS;
	static PixelShaderPtr _debugShaderGeomPS;
	static ID3D11InputLayoutPtr _debugGeomInputLayout;
	static bool32 _hasInit;
	static BlendStates _blendStates;
	static RasterizerStates _rasterizerStates;
	static DepthStencilStates _depthStencilStates;

	static SpriteRenderer _spriteRenderer;
	static ID3D11ShaderResourceViewPtr _spriteTexture;
	static Model _sphereModel;
};