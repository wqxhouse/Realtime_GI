#include "PCH.h"

#include <Graphics\\DeviceStates.h>
#include <InterfacePointers.h>
#include <Graphics\\Camera.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\ShaderCompilation.h>

using namespace SampleFramework11;

class SSR
{
public:
	SSR();
	void Initialize(ID3D11Device *device, ID3D11DeviceContext *context, Camera *camera, RenderTarget2D *clusteredColorRT,
		RenderTarget2D *rmeGBufferRT, RenderTarget2D *normalGBufferRT, DepthStencilBuffer *depthGBuffferRT, RenderTarget2D *ssrTarget,
		ID3D11BufferPtr quadVB, ID3D11BufferPtr quadIB);

	void MainRender();

private:
	void RenderRayTracing();

	RenderTarget2D *_clusteredColorRT;
	RenderTarget2D *_rmeGBufferRT;	// roughness/metallic/emissive
	RenderTarget2D *_normalGBufferRT;
	DepthStencilBuffer *_depthGBuffferRT;

	RenderTarget2D *_ssrTarget;

	VertexShaderPtr _ssrVS;
	PixelShaderPtr  _ssrPS;

	ID3D11Device *_device;
	ID3D11DeviceContext *_context;

	Camera *_camera;

	// Full screen quad
	ID3D11BufferPtr _ssrquadVB;
	ID3D11BufferPtr _ssrquadIB;
	ID3D11InputLayoutPtr _ssrquadInputLayout;

	RasterizerStates _rasterizerStates;
	DepthStencilStates _depthStencilStates;

	struct SSRPassConstants
	{
		Float4Align Float4x4 ProjectionToWorld;
		Float4Align Float4x4 ViewToWorld;
		Float4Align Float4x4 WorldToView;
		Float4Align Float4x4 ViewToProjection;

		Float3 CameraPosWS;
		float NearPlane;
		Float3 CameraZAxisWS;
		float FarPlane;

		Float3 ClusterScale;
		float ProjTermA;
		Float3 ClusterBias;
		float ProjTermB;

	};

	ConstantBuffer<SSRPassConstants> _ssrPassConstants;

};