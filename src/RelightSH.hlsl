// Modified from MJP's DX11 Radiosity integration shader - Robin Wu

//======================================================================================
// Constant buffers
//======================================================================================
cbuffer SHIntegrationConstants : register(b0)
{
	float FinalWeight;
}

//======================================================================================
// Resources
//======================================================================================
struct PackedSH9
{
	float4 chunk0;
	float4 chunk1;
	float4 chunk2;
};

Texture2D<float4> RelightMap : register(t0);
StructuredBuffer<float4x4> ViewToWorldMatrixPalette : register(t1);
RWStructuredBuffer<PackedSH9> PackedSH9OutputBuffer : register(u0);

RWStructuredBuffer<PackedSH9> InputBuffer : register(u0);
RWBuffer<float4> SH9OutputBuffer : register(u1);

//-------------------------------------------------------------------------------------------------
// Projects a direction onto SH and convolves with a cosine kernel to compute irradiance
//-------------------------------------------------------------------------------------------------
void ProjectOntoSH(in float3 n, in float3 color, out float3 sh[9])
{
	// Cosine kernel
	const float A0 = 3.141593f;
    const float A1 = 2.095395f;
    const float A2 = 0.785398f;

    // Band 0
    sh[0] = 0.282095f * A0 * color;

    // Band 1
    sh[1] = 0.488603f * n.y * A1 * color;
    sh[2] = 0.488603f * n.z * A1 * color;
    sh[3] = 0.488603f * n.x * A1 * color;

    // Band 2
    sh[4] = 1.092548f * n.x * n.y * A2 * color;
    sh[5] = 1.092548f * n.y * n.z * A2 * color;
    sh[6] = 0.315392f * (3.0f * n.z * n.z - 1.0f) * A2 * color;
    sh[7] = 1.092548f * n.x * n.z * A2 * color;
    sh[8] = 0.546274f * (n.x * n.x - n.y * n.y) * A2 * color;
}

// Shared memory for summing SH-Basis coefficients for a row
groupshared float3 RowSHBasis[CubemapSize_][9];

//=================================================================================================
// Performs the initial integration/weighting for each pixel and sums together all SH coefficients
// for a row. The integration is based on the "Projection from Cube Maps" section of Peter Pike
// Sloan's "Stupid Spherical Harmonics Tricks".
//=================================================================================================

[numthreads(CubemapSize_, 1, 1)]
void IntegrateCS(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID,
				 uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
{
	// Gather RGB from the texels
	const uint cubemapID = GroupID.x;
	const uint rowIndex = GroupID.y;
	const uint cubeFace = GroupID.z;
	const uint columnIndex = GroupThreadID.x;

	const int2 location = int2(cubeFace * CubemapSize_ + columnIndex, cubemapID * CubemapSize_ + rowIndex);
	float3 radiance = RelightMap.Load(int3(location.xy, 0)).xyz;

	// Calculate the location in [-1, 1] texture space
	float u = (location.x / float(CubemapSize_)) * 2.0f - 1.0f;
	float v = -((location.y / float(CubemapSize_)) * 2.0f - 1.0f);

	// Calculate weight
	float temp = 1.0f + u * u + v * v;
	float weight = 4.0f / (sqrt(temp) * temp);
	radiance *= weight;

	// Extract direction from texel u,v
	float3 dirVS = normalize(float3(u, v, 1.0f));
	float3 dirWS = mul(dirVS, (float3x3)ViewToWorldMatrixPalette[cubemapID * 6 + cubeFace]);

	// Project onto SH
	float3 sh[9];
	ProjectOntoSH(dirWS, radiance, sh);

	// Store in shared memory
	RowSHBasis[columnIndex][0] = sh[0];
	RowSHBasis[columnIndex][1] = sh[1];
	RowSHBasis[columnIndex][2] = sh[2];
	RowSHBasis[columnIndex][3] = sh[3];
	RowSHBasis[columnIndex][4] = sh[4];
	RowSHBasis[columnIndex][5] = sh[5];
	RowSHBasis[columnIndex][6] = sh[6];
	RowSHBasis[columnIndex][7] = sh[7];
	RowSHBasis[columnIndex][8] = sh[8];
	GroupMemoryBarrierWithGroupSync();

	// Sum the coefficients for the row
	[unroll(CubemapSize_)]
	for(uint s = CubemapSize_ / 2; s > 0; s >>= 1)
	{
		if(columnIndex < s)
		{
			RowSHBasis[columnIndex][0] += RowSHBasis[columnIndex + s][0];
			RowSHBasis[columnIndex][1] += RowSHBasis[columnIndex + s][1];
			RowSHBasis[columnIndex][2] += RowSHBasis[columnIndex + s][2];
			RowSHBasis[columnIndex][3] += RowSHBasis[columnIndex + s][3];
			RowSHBasis[columnIndex][4] += RowSHBasis[columnIndex + s][4];
			RowSHBasis[columnIndex][5] += RowSHBasis[columnIndex + s][5];
			RowSHBasis[columnIndex][6] += RowSHBasis[columnIndex + s][6];
			RowSHBasis[columnIndex][7] += RowSHBasis[columnIndex + s][7];
			RowSHBasis[columnIndex][8] += RowSHBasis[columnIndex + s][8];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	// Have the first thread write out to the output texture
	if(GroupThreadID.x == 0)
	{
		[unroll]
		for(uint i = 0; i < 3; ++i)
		{
			PackedSH9 packed;
			packed.chunk0 = float4(RowSHBasis[0][0][i], RowSHBasis[0][1][i], RowSHBasis[0][2][i], 0.0f); 
			packed.chunk1 = float4(RowSHBasis[0][3][i], RowSHBasis[0][4][i], RowSHBasis[0][5][i], 0.0f); 
			packed.chunk2 = float4(RowSHBasis[0][6][i], RowSHBasis[0][7][i], RowSHBasis[0][8][i], 0.0f); 

			PackedSH9OutputBuffer[
				GroupID.y + 
				CubemapSize_ * i + 
				CubemapSize_ * 3 * cubeFace + 
				CubemapSize_ * 3 * 6 * cubemapID] = packed;
		}
	}
}

//// Shared memory for reducing H-Basis coefficients
//groupshared float4 ColumnHBasis[CubemapSize_][3][NumFaces_];

////======================================================================================
//// Reduces to a 1x1 buffer
////======================================================================================
//[numthreads(CubemapSize_, 1, NumFaces_)]
//void ReductionCS(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID,
//					uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
//{
//	const int3 location = int3(GroupThreadID.x, GroupID.y, GroupThreadID.z);

//	// Store in shared memory
//	ColumnHBasis[location.x][location.y][location.z] = InputBuffer[location.x + CubemapSize_ * location.y + CubemapSize_ * 3 * location.z];
//	GroupMemoryBarrierWithGroupSync();

//	// Sum the coefficients for the column
//	[unroll(CubemapSize_)]
//	for(uint s = CubemapSize_ / 2; s > 0; s >>= 1)
//	{
//		if(GroupThreadID.x < s)
//			ColumnHBasis[location.x][location.y][location.z] += ColumnHBasis[location.x + s][location.y][location.z];

//		GroupMemoryBarrierWithGroupSync();
//	}

//	// Have the first thread write out to the output buffer
//	if (GroupThreadID.x == 0 && GroupThreadID.z == 0)
//	{
//		float4 output = 0.0f;
//		[unroll(NumFaces_)]
//		for(uint i = 0; i < NumFaces_; ++i)
//			output += ColumnHBasis[location.x][location.y][i];
//		output *= FinalWeight;
//		OutputBuffer[VertexIndex * 3 + location.y] = output;
//	}
//}

////======================================================================================
//// Sums the result of two bounce passes
////======================================================================================
//[numthreads(NumBounceSumThreads_, 1, 1)]
//void SumBouncesCS(uint3 GroupID : SV_GroupID, uint3 DispatchThreadID : SV_DispatchThreadID,
//					uint3 GroupThreadID : SV_GroupThreadID, uint GroupIndex : SV_GroupIndex)
//{
//	const uint index = GroupThreadID.x + GroupID.x * NumBounceSumThreads_;
//	if(index < NumElements)
//		OutputBuffer[index] = InputBuffer0[index] + InputBuffer1[index];
//}
