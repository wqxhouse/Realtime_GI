#include "BoundUtils.h"

Float3 Float3Max(const Float3 &a, const Float3 &b)
{
	return Float3(a.x > b.x ? a.x : b.x,
		a.y > b.y ? a.y : b.y,
		a.z > b.z ? a.z : b.z);
}

Float3 Float3Min(const Float3 &a, const Float3 &b)
{
	return Float3(a.x < b.x ? a.x : b.x,
		a.y < b.y ? a.y : b.y,
		a.z < b.z ? a.z : b.z);
}

BBox GetTransformedBBox(const BBox &bbox, const Float4x4 &transform)
{
	/*Float3 vmax = Float3(bbox.Max);
	Float3 vmin = Float3(bbox.Min);

	vmax = Float3::Transform(vmax, transform);
	vmin = Float3::Transform(vmin, transform);

	XMFLOAT3 points[2] = { *(XMFLOAT3 *)&vmax, *(XMFLOAT3 *)&vmin };
	return ComputeBoundingBoxFromPoints(points, 2, sizeof(XMFLOAT3));*/

	Float3 xa = transform.Right() * bbox.Min.x;
	Float3 xb = transform.Right() * bbox.Max.x;

	Float3 ya = transform.Up() * bbox.Min.y;
	Float3 yb = transform.Up() * bbox.Max.y;

	Float3 za = transform.Forward() * bbox.Min.z;
	Float3 zb = transform.Forward() * bbox.Max.z;
		
	BBox out;
	out.Max = *(XMFLOAT3 *)&(Float3Max(xa, xb) + Float3Max(ya, yb) + Float3Max(za, zb) + transform.Translation());
	out.Min = *(XMFLOAT3 *)&(Float3Min(xa, xb) + Float3Min(ya, yb) + Float3Min(za, zb) + transform.Translation());

	return out;
}

BSphere GetTransformedBSphere(const BSphere &bsphere, const Float4x4 &transform)
{
	Float3 center = bsphere.Center;
	center = Float3::Transform(center, transform);

	BSphere out;
	out.Center = *(XMFLOAT3 *)&center;

	// TODO: assume uniform scaling
	float scale_x = sqrt(transform._11 * transform._11 + transform._12 * transform._12 + transform._13 * transform._13);

	out.Radius = bsphere.Radius * scale_x;
	return out;
}

BBox MergeBoundingBoxes(const std::vector<BBox> &bboxes)
{
	// get bounding box points
	size_t num = bboxes.size();
	std::vector<XMFLOAT3> points(num * 2);
	for (uint64 i = 0; i < num; i++)
	{
		points[i * 2] = bboxes[i].Max;
		points[i * 2 + 1] = bboxes[i].Min;
	}
	
	return ComputeBoundingBoxFromPoints(&points[0], (uint32)num * 2, sizeof(XMFLOAT3));
}

BSphere MergeBoundingSpheres(const std::vector<BSphere> &bspheres)
{
	BSphere sphere = bspheres[0];
	
	for (uint32 i = 1; i < bspheres.size(); i++)
	{
		BSphere &sphere1 = sphere;
		const BSphere &sphere2 = bspheres[i];

		Float3 sphere2Center(sphere2.Center);
		Float3 sphere1Center(sphere1.Center);
		Float3 difference = sphere2Center - sphere1Center;

		float length = difference.Length();
		float radius = sphere1.Radius;
		float radius2 = sphere2.Radius;

		if (radius + radius2 >= length)
		{
			if (radius - radius2 >= length)
				return sphere1;

			if (radius2 - radius >= length)
				return sphere2;
		}

		Float3 vector = difference * (1.0f / length);
		float min = Min(-radius, length - radius2);
		float max = (Max(radius, length + radius2) - min) * 0.5f;

		sphere.Center = *(XMFLOAT3 *)&(sphere1Center + vector * (max + min));
		sphere.Radius = max;
	}

	return sphere;
}

BBox ComputeBoundingBoxFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride)
{
	BBox out;
	XMVECTOR MinX, MaxX, MinY, MaxY, MinZ, MaxZ;
	GetBoundCornersFromPoints(points, 2, sizeof(XMFLOAT3), MinX, MaxX, MinY, MaxY, MinZ, MaxZ);

	float maxx = XMVectorGetX(MaxX);
	float maxy = XMVectorGetY(MaxY);
	float maxz = XMVectorGetZ(MaxZ);

	float minx = XMVectorGetX(MinX);
	float miny = XMVectorGetY(MinY);
	float minz = XMVectorGetZ(MinZ);

	out.Max.x = maxx;
	out.Max.y = maxy;
	out.Max.z = maxz;

	out.Min.x = minx;
	out.Min.y = miny;
	out.Min.z = minz;

	return out;
}

void GetBoundCornersFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride, 
	XMVECTOR &minx, XMVECTOR &maxx, XMVECTOR &miny, XMVECTOR &maxy, XMVECTOR &minz, XMVECTOR &maxz)
{
	Assert_(numPoints > 0);
	Assert_(points);

	minx = maxx = miny = maxy = minz = maxz = XMLoadFloat3(points);

	for (uint32 i = 1; i < numPoints; i++)
	{
		XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)points + i * stride));

		float px = XMVectorGetX(Point);
		float py = XMVectorGetY(Point);
		float pz = XMVectorGetZ(Point);

		if (px < XMVectorGetX(minx))
			minx = Point;

		if (px > XMVectorGetX(maxx))
			maxx = Point;

		if (py < XMVectorGetY(miny))
			miny = Point;

		if (py > XMVectorGetY(maxy))
			maxy = Point;

		if (pz < XMVectorGetZ(minz))
			minz = Point;

		if (pz > XMVectorGetZ(maxz))
			maxz = Point;
	}
}

BSphere ComputeBoundingSphereFromPoints(const XMFLOAT3* points, uint32 numPoints, uint32 stride, BBox *optionalBBoxOut)
{
	BSphere sphere;

	Assert_(numPoints > 0);
	Assert_(points);

	// Find the points with minimum and maximum x, y, and z
	XMVECTOR MinX, MaxX, MinY, MaxY, MinZ, MaxZ;

	MinX = MaxX = MinY = MaxY = MinZ = MaxZ = XMLoadFloat3(points);
	GetBoundCornersFromPoints(points, numPoints, stride, MinX, MaxX, MinY, MaxY, MinZ, MaxZ);

	/*for (uint32 i = 1; i < numPoints; i++)
	{
		XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)points + i * stride));

		float px = XMVectorGetX(Point);
		float py = XMVectorGetY(Point);
		float pz = XMVectorGetZ(Point);

		if (px < XMVectorGetX(MinX))
			MinX = Point;

		if (px > XMVectorGetX(MaxX))
			MaxX = Point;

		if (py < XMVectorGetY(MinY))
			MinY = Point;

		if (py > XMVectorGetY(MaxY))
			MaxY = Point;

		if (pz < XMVectorGetZ(MinZ))
			MinZ = Point;

		if (pz > XMVectorGetZ(MaxZ))
			MaxZ = Point;
	}*/

	if (optionalBBoxOut != NULL)
	{
		float maxx = XMVectorGetX(MaxX);
		float maxy = XMVectorGetY(MaxY);
		float maxz = XMVectorGetZ(MaxZ);

		float minx = XMVectorGetX(MinX);
		float miny = XMVectorGetY(MinY);
		float minz = XMVectorGetZ(MinZ);

		optionalBBoxOut->Max.x = maxx;
		optionalBBoxOut->Max.y = maxy;
		optionalBBoxOut->Max.z = maxz;

		optionalBBoxOut->Min.x = minx;
		optionalBBoxOut->Min.y = miny;
		optionalBBoxOut->Min.z = minz;
	}

	// Use the min/max pair that are farthest apart to form the initial sphere.
	XMVECTOR DeltaX = MaxX - MinX;
	XMVECTOR DistX = XMVector3Length(DeltaX);

	XMVECTOR DeltaY = MaxY - MinY;
	XMVECTOR DistY = XMVector3Length(DeltaY);

	XMVECTOR DeltaZ = MaxZ - MinZ;
	XMVECTOR DistZ = XMVector3Length(DeltaZ);

	XMVECTOR Center;
	XMVECTOR Radius;

	if (XMVector3Greater(DistX, DistY))
	{
		if (XMVector3Greater(DistX, DistZ))
		{
			// Use min/max x.
			Center = (MaxX + MinX) * 0.5f;
			Radius = DistX * 0.5f;
		}
		else
		{
			// Use min/max z.
			Center = (MaxZ + MinZ) * 0.5f;
			Radius = DistZ * 0.5f;
		}
	}
	else // Y >= X
	{
		if (XMVector3Greater(DistY, DistZ))
		{
			// Use min/max y.
			Center = (MaxY + MinY) * 0.5f;
			Radius = DistY * 0.5f;
		}
		else
		{
			// Use min/max z.
			Center = (MaxZ + MinZ) * 0.5f;
			Radius = DistZ * 0.5f;
		}
	}

	// Add any points not inside the sphere.
	for (uint32 i = 0; i < numPoints; i++)
	{
		XMVECTOR Point = XMLoadFloat3((XMFLOAT3*)((BYTE*)points + i * stride));

		XMVECTOR Delta = Point - Center;

		XMVECTOR Dist = XMVector3Length(Delta);

		if (XMVector3Greater(Dist, Radius))
		{
			// Adjust sphere to include the new point.
			Radius = (Radius + Dist) * 0.5f;
			Center += (XMVectorReplicate(1.0f) - Radius * XMVectorReciprocal(Dist)) * Delta;
		}
	}

	XMStoreFloat3(&sphere.Center, Center);
	XMStoreFloat(&sphere.Radius, Radius);

	return sphere;
}


void ComputeFrustum(const XMMATRIX& viewProj, Frustum& frustum)
{
	XMVECTOR det;
	XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

	// Corners in homogeneous clip space
	XMVECTOR corners[8] =
	{                                               //                         7--------6
		XMVectorSet(1.0f, -1.0f, 0.0f, 1.0f),      //                        /|       /|
		XMVectorSet(-1.0f, -1.0f, 0.0f, 1.0f),      //     Y ^               / |      / |
		XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f),      //     | _              3--------2  |
		XMVectorSet(-1.0f, 1.0f, 0.0f, 1.0f),      //     | /' Z           |  |     |  |
		XMVectorSet(1.0f, -1.0f, 1.0f, 1.0f),      //     |/               |  5-----|--4
		XMVectorSet(-1.0f, -1.0f, 1.0f, 1.0f),      //     + ---> X         | /      | /
		XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f),      //                      |/       |/
		XMVectorSet(-1.0f, 1.0f, 1.0f, 1.0f),      //                      1--------0
	};

	// Convert to world space
	for (uint32 i = 0; i < 8; ++i)
		corners[i] = XMVector3TransformCoord(corners[i], invViewProj);

	// Calculate the 6 planes
	frustum.Planes[0] = XMPlaneFromPoints(corners[0], corners[4], corners[2]);
	frustum.Planes[1] = XMPlaneFromPoints(corners[1], corners[3], corners[5]);
	frustum.Planes[2] = XMPlaneFromPoints(corners[3], corners[2], corners[7]);
	frustum.Planes[3] = XMPlaneFromPoints(corners[1], corners[5], corners[0]);
	frustum.Planes[4] = XMPlaneFromPoints(corners[5], corners[7], corners[4]);
	frustum.Planes[5] = XMPlaneFromPoints(corners[1], corners[0], corners[3]);
}

// Tests a frustum for intersection with a sphere
uint32 TestFrustumSphere(const Frustum& frustum, const BSphere& sphere, bool ignoreNearZ)
{
	XMVECTOR sphereCenter = XMLoadFloat3(&sphere.Center);

	uint32 result = 1;
	uint32 numPlanes = ignoreNearZ ? 5 : 6;
	for (uint32 i = 0; i < numPlanes; i++) {
		float distance = XMVectorGetX(XMPlaneDotCoord(frustum.Planes[i], sphereCenter));

		if (distance < -sphere.Radius)
			return 0;
		else if (distance < sphere.Radius)
			result = 1;
	}

	return result;
}


// Calculates the bounding sphere for each MeshPart
void ComputeModelBounds(ID3D11Device* device, ID3D11DeviceContext* context,
	// const Float4x4& world, 
	Model* model,
	std::vector<BSphere>& boundingSpheres, 
	std::vector<BBox> &boundingBoxes)
{
	boundingSpheres.clear();
	boundingBoxes.clear();

	for (uint32 meshIdx = 0; meshIdx < model->Meshes().size(); ++meshIdx)
	{
		Mesh& mesh = model->Meshes()[meshIdx];

		// Create staging buffers for copying the vertex/index data to
		ID3D11BufferPtr stagingVB;
		ID3D11BufferPtr stagingIB;

		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.BindFlags = 0;
		bufferDesc.ByteWidth = mesh.NumVertices() * mesh.VertexStride();
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = 0;
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		DXCall(device->CreateBuffer(&bufferDesc, nullptr, &stagingVB));

		uint32 indexSize = mesh.IndexBufferType() == IndexType::Index16Bit ? 2 : 4;
		bufferDesc.ByteWidth = mesh.NumIndices() * indexSize;
		DXCall(device->CreateBuffer(&bufferDesc, nullptr, &stagingIB));

		context->CopyResource(stagingVB, mesh.VertexBuffer());
		context->CopyResource(stagingIB, mesh.IndexBuffer());

		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(stagingVB, 0, D3D11_MAP_READ, 0, &mapped);
		const BYTE* verts = reinterpret_cast<const BYTE*>(mapped.pData);
		uint32 stride = mesh.VertexStride();

		context->Map(stagingIB, 0, D3D11_MAP_READ, 0, &mapped);
		const BYTE* indices = reinterpret_cast<const BYTE*>(mapped.pData);
		const uint32* indices32 = reinterpret_cast<const uint32*>(mapped.pData);
		const WORD* indices16 = reinterpret_cast<const WORD*>(mapped.pData);

		for (uint32 partIdx = 0; partIdx < mesh.MeshParts().size(); ++partIdx)
		{
			const MeshPart& part = mesh.MeshParts()[partIdx];

			std::vector<XMFLOAT3> points;
			for (uint32 i = 0; i < part.IndexCount; ++i)
			{
				uint32 index = indexSize == 2 ? indices16[part.IndexStart + i] : indices32[part.IndexStart + i];
				Float3 point = *reinterpret_cast<const Float3*>(verts + (index * stride));
				// point = Float3::Transform(point, world);
				points.push_back(*reinterpret_cast<XMFLOAT3*>(&point));
			}
			BBox bbox;
			BSphere sphere = ComputeBoundingSphereFromPoints(&points[0], static_cast<uint32>(points.size()), sizeof(XMFLOAT3), &bbox);
			boundingSpheres.push_back(sphere);
			boundingBoxes.push_back(bbox);
		}

		context->Unmap(stagingVB, 0);
		context->Unmap(stagingIB, 0);
	}
}

