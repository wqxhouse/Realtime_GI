#pragma once

#include "PCH.h"
#include <SF11_Math.h>
#include <Graphics\\GraphicsTypes.h>
#include <Graphics\\Model.h>

using namespace SampleFramework11;

struct BSphere
{
	XMFLOAT3 Center;
	float Radius;
};

struct BBox
{
	XMFLOAT3 Max;
	XMFLOAT3 Min;
};

// Represents the 6 planes of a frustum
Float4Align struct Frustum
{
	XMVECTOR Planes[6];
};

BBox GetTransformedBBox(const BBox &bbox, const Float4x4 &transform);
BSphere GetTransformedBSphere(const BSphere &bsphere, const Float4x4 &transform);

BSphere MergeBoundingSpheres(const std::vector<BSphere> &bspheres);
BBox MergeBoundingBoxes(const std::vector<BBox> &bboxes);
BBox ComputeBoundingBoxFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride);

// Finds the approximate smallest enclosing bounding sphere for a set of points. Based on
// "An Efficient Bounding Sphere", by Jack Ritter.
BSphere ComputeBoundingSphereFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride, BBox *optionalBBoxOut=NULL);


void GetBoundCornersFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride, XMVECTOR &minx, XMVECTOR &maxx, XMVECTOR &miny, XMVECTOR &maxy, XMVECTOR &minz, XMVECTOR &maxz);

// Calculates the frustum planes given a view * projection matrix
void ComputeFrustum(const XMMATRIX& viewProj, Frustum& frustum);

// Tests a frustum for intersection with a sphere
uint32 TestFrustumSphere(const Frustum& frustum, const BSphere& sphere, bool ignoreNearZ);

// Calculates the bounding sphere for each MeshPart
void ComputeModelBounds(ID3D11Device* device, ID3D11DeviceContext* context,
	// const Float4x4& world, 
	Model* model,
	std::vector<BSphere>& boundingSpheres, 
	std::vector<BBox> &boundingBoxes);