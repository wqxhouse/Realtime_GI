#include "DebugRenderer.h"

void DebugRenderer::Initialize(DeviceManager *deviceManager, ID3D11Device *device, ID3D11DeviceContext *context, Camera *cam)
{
	_deviceManager = deviceManager;
	_device = device;
	_context = context;
	_camera = cam;

	if (!_hasInit)
	{
		_cubeMesh.InitBox(_device, Float3(1, 1, 1), Float3(), Quaternion(), 0);

		_blendStates.Initialize(_device);
		_rasterizerStates.Initialize(_device);
		_depthStencilStates.Initialize(_device);

		_shaderConstants.Initialize(_device, false);

		// Setup debug geom render 
		_debugShaderGeomVS = CompileVSFromFile(_device, L"DebugGeom.hlsl", "DebugGeom_VS", "vs_5_0");
		_debugShaderGeomPS = CompilePSFromFile(_device, L"DebugGeom.hlsl", "DebugGeom_PS", "ps_5_0");

		DXCall(_device->CreateInputLayout(_cubeMesh.InputElements(), 1,
			   _debugShaderGeomVS->ByteCode->GetBufferPointer(), 
			   _debugShaderGeomVS->ByteCode->GetBufferSize(), &_debugGeomInputLayout));

		_hasInit = true;
	}
}

void DebugRenderer::QueueCubeWire(const Float3 &pos, const Float3 &halfLength, const Float4 &color)
{
	
}

void DebugRenderer::QueueCubeTranslucent(const Float3 &pos, const Float3 &halfLength, const Float4 &color)
{

}

void DebugRenderer::QueueBBoxWire(const BBox &bbox, const Float4 &color)
{
	Float4x4 &mat = matModelToWorldFromBBox(bbox);
	_cubeQueueWire.push_back(std::make_tuple(mat, color));
}

void DebugRenderer::QueueBBoxTranslucent(const BBox &bbox, const Float4 &color)
{
	Float4x4 &mat = matModelToWorldFromBBox(bbox);
	_cubeQueueTranslucent.push_back(std::make_tuple(mat, color));
}

void DebugRenderer::FlushDrawQueued()
{
	if (_cubeQueueWire.empty() && _cubeQueueTranslucent.empty())
	{
		return;
	}

	PIXEvent event(L"DebugRenderer Pass");

	_context->VSSetShader(_debugShaderGeomVS, nullptr, 0);
	_context->PSSetShader(_debugShaderGeomPS, nullptr, 0);
	_context->GSSetShader(nullptr, nullptr, 0);
	_context->CSSetShader(nullptr, nullptr, 0);
	_context->HSSetShader(nullptr, nullptr, 0);

	_context->IASetInputLayout(_debugGeomInputLayout);

	ID3D11Buffer* vertexBuffers[1] = { _cubeMesh.VertexBuffer() };
	UINT vertexStrides[1] = { _cubeMesh.VertexStride() };
	UINT offsets[1] = { 0 };
	_context->IASetVertexBuffers(0, 1, vertexBuffers, vertexStrides, offsets);
	_context->IASetIndexBuffer(_cubeMesh.IndexBuffer(), _cubeMesh.IndexBufferFormat(), 0);
	_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	float blendFactor[4] = { 1, 1, 1, 1 };
	_context->OMSetBlendState(_blendStates.AlphaBlend(), blendFactor, 0xFFFFFFFF);
	_context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);

	// TODO: later support render to different render target
	// ID3D11RenderTargetView *rtvs[] = { _deviceManager->BackBuffer() };
	// _context->OMSetRenderTargets(1, rtvs, nullptr);
	// SetViewport(_context, _deviceManager->BackBufferWidth(), _deviceManager->BackBufferHeight());

	// Batch wire call
	_context->RSSetState(_rasterizerStates.Wireframe());
	updateConstantBuffer(true);
	for (size_t i = 0; i < _cubeMesh.MeshParts().size(); i++)
	{
		MeshPart &m = _cubeMesh.MeshParts()[i];
		_context->DrawIndexedInstanced(m.IndexCount, (UINT)_cubeQueueWire.size(), 0, 0, 0);
	}

	// Batch translucent call
	updateConstantBuffer(false);
	_context->RSSetState(_rasterizerStates.NoCull());
	for (size_t i = 0; i < _cubeMesh.MeshParts().size(); i++)
	{
		MeshPart &m = _cubeMesh.MeshParts()[i];
		_context->DrawIndexedInstanced(m.IndexCount, (UINT)_cubeQueueTranslucent.size(), 0, 0, 0);
	}

	// rtvs[0] = nullptr;
	// _context->OMSetRenderTargets(1, rtvs, nullptr);

	// flush
	_cubeQueueWire.clear();
	_cubeQueueTranslucent.clear();
}

Float4x4 DebugRenderer::matModelToWorldFromBBox(const BBox &bbox)
{
	Float3 diff = Float3(bbox.Max) - Float3(bbox.Min);
	Float3 center = Float3(bbox.Max) + Float3(bbox.Min);
	center *= 0.5;

	// non-uniform scale
	Float4x4 scale = Float4x4::ScaleMatrix(diff);
	return scale * Float4x4::TranslationMatrix(center);
}

void DebugRenderer::updateConstantBuffer(bool32 isWire)
{
	if (isWire)
	{
		for (size_t i = 0; i < _cubeQueueWire.size(); i++)
		{
			auto t = _cubeQueueWire[i];
			_shaderConstants.Data.data[i].Transform = Float4x4::Transpose(std::get<0>(t) * _camera->ViewProjectionMatrix());
			_shaderConstants.Data.data[i].Color = std::get<1>(t);
		}
	}
	else
	{
		for (size_t i = 0; i < _cubeQueueTranslucent.size(); i++)
		{
			auto t = _cubeQueueTranslucent[i];

			_shaderConstants.Data.data[i].Transform = Float4x4::Transpose(std::get<0>(t) * _camera->ViewProjectionMatrix());
			_shaderConstants.Data.data[i].Color = std::get<1>(t);
		}
	}

	_shaderConstants.ApplyChanges(_context);
	_shaderConstants.SetVS(_context, 0);
}

// static vars
Mesh DebugRenderer::_cubeMesh;
VertexShaderPtr DebugRenderer::_debugShaderGeomVS;
PixelShaderPtr DebugRenderer::_debugShaderGeomPS;
ID3D11InputLayoutPtr DebugRenderer::_debugGeomInputLayout;
bool32 DebugRenderer::_hasInit;
BlendStates DebugRenderer::_blendStates;
RasterizerStates DebugRenderer::_rasterizerStates;
DepthStencilStates DebugRenderer::_depthStencilStates;