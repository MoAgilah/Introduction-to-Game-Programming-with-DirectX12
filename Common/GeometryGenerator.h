//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Defines a static class for procedurally generating the geometry of
// common mathematical objects.
//
// All triangles are generated "outward" facing.  If you want "inward"
// facing triangles (for example, if you want to place the camera inside
// a sphere to simulate a sky), you will need to:
//   1. Change the Direct3D cull mode or manually reverse the winding order.
//   2. Invert the normal.
//   3. Update the texture coordinates and tangent vectors.
//***************************************************************************************
#pragma once

// As a learning opportunity I'd like to spend some time understanding and rewriting the code found within.

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

class GeometryGenerator
{
public:
	struct Vertex
	{
		Vertex();
		Vertex(const DirectX::XMFLOAT3& pos,
			const DirectX::XMFLOAT3& norm,
			const DirectX::XMFLOAT3& tang,
			const DirectX::XMFLOAT2& uv);
		Vertex(float px, float py, float pz,
			float nx, float ny, float nz,
			float tx, float ty, float tz,
			float u, float v);

		DirectX::XMFLOAT3 m_position;
		DirectX::XMFLOAT3 m_normal;
		DirectX::XMFLOAT3 m_tangentU;
		DirectX::XMFLOAT2 m_texC;
	};

	struct MeshData
	{
		std::vector<Vertex> m_vertices;
		std::vector<uint32> m_indices32;

		std::vector<uint16>& GetIndices16();

	private:
		std::vector<uint16> m_indices16;
	};

	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	MeshData CreateGrid(float width, float depth, uint32 m, uint32 n);

	/// Creates a box centered at the origin with the given dimensions, where each
	/// face has m rows and n columns of vertices.
	MeshData CreateBox(float width, float height, float depth, uint32 numSubdivisions);

	/// Creates a sphere centered at the origin with the given radius.  The
	/// slices and stacks parameters control the degree of tessellation.
	MeshData CreateSphere(float radius, uint32 sliceCount, uint32 stackCount);

	/// Creates a geosphere centered at the origin with the given radius.  The
	/// depth controls the level of tessellation.
	MeshData CreateGeosphere(float radius, uint32 numSubdivisions);

	/// Creates an mxn grid in the xz-plane with m rows and n columns, centered
	/// at the origin with the specified width and depth.
	MeshData CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount);

	/// Creates a quad aligned with the screen.  This is useful for postprocessing and screen effects.
	MeshData CreateQuad(float x, float y, float w, float h, float depth);

private:

	void Subdivide(MeshData& meshData);
	Vertex MidPoint(const Vertex& v0, const Vertex& v1);
	void BuildCylinderTopCap(float topRadius, float height, uint32 sliceCount, MeshData& meshData);
	void BuildCylinderBottomCap(float bottomRadius, float height, uint32 sliceCount, MeshData& meshData);
};