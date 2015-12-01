#include "SSR.h"

SSR::SSR()
{
}

void SSR::Initialize(ID3D11Device *device, ID3D11DeviceContext *context, Camera *camera, RenderTarget2D *clusteredColorRT,
	RenderTarget2D *rmeGBufferRT, RenderTarget2D *normalGBufferRT, DepthStencilBuffer *depthGBuffferRT, RenderTarget2D *ssrTarget,
	ID3D11BufferPtr quadVB, ID3D11BufferPtr quadIB)
{
	_device = device;
	_context = context;
	_camera = camera;

	_clusteredColorRT = clusteredColorRT;
	_rmeGBufferRT = rmeGBufferRT;
	_normalGBufferRT = normalGBufferRT;
	_depthGBuffferRT = depthGBuffferRT;

	_ssrTarget = ssrTarget;

	_ssrVS = CompileVSFromFile(_device, L"RayTracing.hlsl", "VS");
	_ssrPS = CompilePSFromFile(_device, L"RayTracing.hlsl", "PS");

	_ssrquadVB = quadVB;
	_ssrquadIB = quadIB;

	_rasterizerStates.Initialize(_device);
	_depthStencilStates.Initialize(_device);

	_ssrPassConstants.Initialize(_device);
}


Float4 CreateInvDeviceZToWorldZTransformA(Float4x4 const & ProjMatrix)
{
	// The depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	// 
	// Note a reversed Z projection matrix will have A=0. 
	//
	// Done in shader as:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	float DepthMul = ProjMatrix.m[2][2];
	float DepthAdd = ProjMatrix.m[3][2];

	if (DepthAdd == 0.f)
	{
		// Avoid dividing by 0 in this case
		DepthAdd = 0.00000001f;
	}

	float SubtractValue = DepthMul / DepthAdd;

	// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
	// This fixes fog not being applied to the black background in the editor.
	SubtractValue -= 0.00000001f;

	return Float4(
		0.0f,			// Unused
		0.0f,			// Unused
		1.f / DepthAdd,
		SubtractValue
		);
}


void SSR::MainRender()
{
	RenderRayTracing();
}

void SSR::RenderRayTracing()
{
	PIXEvent event(L"Render RayTracing");

	ID3D11DeviceContextPtr context = _context;

	SetViewport(context, _ssrTarget->Width, _ssrTarget->Height);

	// Create the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	DXCall(_device->CreateInputLayout(layout, 1, _ssrVS->ByteCode->GetBufferPointer(),
		_ssrVS->ByteCode->GetBufferSize(), &_ssrquadInputLayout));

	context->IASetInputLayout(_ssrquadInputLayout);

	// Set the vertex buffer
	uint32 stride = sizeof(XMFLOAT4);
	uint32 offset = 0;
	ID3D11Buffer* vertexBuffers[1] = { _ssrquadVB.GetInterfacePtr() };
	context->IASetVertexBuffers(0, 1, vertexBuffers, &stride, &offset);
	context->IASetIndexBuffer(_ssrquadIB, DXGI_FORMAT_R16_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->OMSetDepthStencilState(_depthStencilStates.DepthDisabled(), 0);
	context->RSSetState(_rasterizerStates.NoCull());

	ID3D11RenderTargetView* rtvs[1] = { _ssrTarget->RTView };
	context->OMSetRenderTargets(1, rtvs, 0);

	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->ClearRenderTargetView(_ssrTarget->RTView, clearColor);

	context->VSSetShader(_ssrVS, nullptr, 0);
	context->PSSetShader(_ssrPS, nullptr, 0);
	context->GSSetShader(nullptr, nullptr, 0);
	context->HSSetShader(nullptr, nullptr, 0);
	context->DSSetShader(nullptr, nullptr, 0);

	// Update constant buffer
	_ssrPassConstants.Data.CameraPosWS = _camera->Position();
	_ssrPassConstants.Data.ProjectionToWorld = Float4x4::Transpose(Float4x4::Invert(_camera->ViewProjectionMatrix()));
	_ssrPassConstants.Data.ViewToWorld = Float4x4::Transpose(_camera->WorldMatrix());
	_ssrPassConstants.Data.WorldToView = Float4x4::Transpose(_camera->ViewMatrix());
	_ssrPassConstants.Data.ViewToProjection = Float4x4::Transpose(_camera->ProjectionMatrix());
	
	_ssrPassConstants.Data.NearPlane = 0.01f;
	_ssrPassConstants.Data.CameraZAxisWS = _camera->WorldMatrix().Forward();
	_ssrPassConstants.Data.FarPlane = 300.0f;
	_ssrPassConstants.Data.ProjTermA = 300.0f / (300.0f - 0.01f);
	_ssrPassConstants.Data.ProjTermB = (-300.0f * 0.01f) / (300.0f - 0.01f);
	_ssrPassConstants.Data.invToWorldZ = CreateInvDeviceZToWorldZTransformA(_camera->ProjectionMatrix());
	_ssrPassConstants.ApplyChanges(context);
	_ssrPassConstants.SetVS(context, 0);
	_ssrPassConstants.SetPS(context, 0);

	// Bind shader resources
	ID3D11ShaderResourceView* ssrResources[] = 
	{
		_clusteredColorRT->SRView,
		_rmeGBufferRT->SRView,
		_normalGBufferRT->SRView,
		_depthGBuffferRT->SRView
	};

	// Render
	context->PSSetShaderResources(0, _countof(ssrResources), ssrResources);
	context->DrawIndexed(6, 0, 0);

	// UnBind
	memset(ssrResources, 0, sizeof(ssrResources));
	context->PSSetShaderResources(0, _countof(ssrResources), ssrResources);
	context->VSSetShader(nullptr, nullptr, 0);
	context->PSSetShader(nullptr, nullptr, 0);

	rtvs[0] = nullptr;
	context->OMSetRenderTargets(1, rtvs, nullptr);
}