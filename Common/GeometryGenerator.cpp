#include "GeometryGenerator.h"

GeometryGenerator::Vertex::Vertex()
	: m_position(0, 0, 0),
	m_normal(0, 0, 0),
	m_tangentU(0, 0, 0),
	m_texC(0, 0)
{
}

GeometryGenerator::Vertex::Vertex(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm, const DirectX::XMFLOAT3& tang, const DirectX::XMFLOAT2& uv)
	: m_position(pos),
	m_normal(norm),
	m_tangentU(tang),
	m_texC(uv)
{
}

GeometryGenerator::Vertex::Vertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
	: m_position(px, py, pz),
	m_normal(nx, ny, nz),
	m_tangentU(tx, ty, tz),
	m_texC(u,v)
{
}

std::vector<uint16>& GeometryGenerator::MeshData::GetIndices16()
{
	if (m_indices16.empty())
	{
		m_indices16.resize(m_indices32.size());
		for (size_t i = 0; i < m_indices32.size(); ++i)
			m_indices16[i] = static_cast<uint16>(m_indices32[i]);
	}

	return m_indices16;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float width, float depth, uint32 m, uint32 n)
{
	MeshData meshData;

	uint32 vertexCount = m * n;
	uint32 faceCount = (m - 1) * (n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f * width;
	float halfDepth = 0.5f * depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	meshData.m_vertices.resize(vertexCount);
	for (uint32 i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			meshData.m_vertices[i * n + j].m_position = DirectX::XMFLOAT3(x, 0.0f, z);
			meshData.m_vertices[i * n + j].m_normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.m_vertices[i * n + j].m_tangentU = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			meshData.m_vertices[i * n + j].m_texC.x = j * du;
			meshData.m_vertices[i * n + j].m_texC.y = i * dv;
		}
	}

	//
	// Create the indices.
	//

	meshData.m_indices32.resize(faceCount * 3); // 3 indices per face

	// Iterate over each quad and compute indices.
	uint32 k = 0;
	for (uint32 i = 0; i < m - 1; ++i)
	{
		for (uint32 j = 0; j < n - 1; ++j)
		{
			meshData.m_indices32[k] = i * n + j;
			meshData.m_indices32[k + 1] = i * n + j + 1;
			meshData.m_indices32[k + 2] = (i + 1) * n + j;

			meshData.m_indices32[k + 3] = (i + 1) * n + j;
			meshData.m_indices32[k + 4] = i * n + j + 1;
			meshData.m_indices32[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float width, float height, float depth, uint32 numSubdivisions)
{
	MeshData meshData;

	//
	// Create the vertices.
	//

	Vertex v[24];

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face vertex data.
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the back face vertex data.
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the top face vertex data.
	v[8] = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face vertex data.
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face vertex data.
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the right face vertex data.
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.m_vertices.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint32 i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.m_indices32.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float radius, uint32 sliceCount, uint32 stackCount)
{
	MeshData meshData;

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.m_vertices.push_back(topVertex);

	float phiStep = DirectX::XM_PI / stackCount;
	float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (uint32 i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			Vertex v;

			// spherical to cartesian
			v.m_position.x = radius * sinf(phi) * cosf(theta);
			v.m_position.y = radius * cosf(phi);
			v.m_position.z = radius * sinf(phi) * sinf(theta);

			// Partial derivative of P with respect to theta
			v.m_tangentU.x = -radius * sinf(phi) * sinf(theta);
			v.m_tangentU.y = 0.0f;
			v.m_tangentU.z = +radius * sinf(phi) * cosf(theta);

			DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&v.m_tangentU);
			DirectX::XMStoreFloat3(&v.m_tangentU, DirectX::XMVector3Normalize(T));

			DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&v.m_position);
			DirectX::XMStoreFloat3(&v.m_normal, DirectX::XMVector3Normalize(p));

			v.m_texC.x = theta / DirectX::XM_2PI;
			v.m_texC.y = phi / DirectX::XM_PI;

			meshData.m_vertices.push_back(v);
		}
	}

	meshData.m_vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (uint32 i = 1; i <= sliceCount; ++i)
	{
		meshData.m_indices32.push_back(0);
		meshData.m_indices32.push_back(i + 1);
		meshData.m_indices32.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//
	uint32 baseIndex = 1;
	uint32 ringVertexCount = sliceCount + 1;
	for (uint32 i = 0; i < stackCount - 2; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.m_indices32.push_back(baseIndex + i * ringVertexCount + j);
			meshData.m_indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.m_indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			meshData.m_indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			meshData.m_indices32.push_back(baseIndex + i * ringVertexCount + j + 1);
			meshData.m_indices32.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	uint32 southPoleIndex = (uint32)meshData.m_vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.m_indices32.push_back(southPoleIndex);
		meshData.m_indices32.push_back(baseIndex + i);
		meshData.m_indices32.push_back(baseIndex + i + 1);
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float radius, uint32 numSubdivisions)
{
	MeshData meshData;

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint32>(numSubdivisions, 6u);

	// Approximate a sphere by tessellating an
	//icosahedron.
	const float X = 0.525731f;
	const float Z = 0.850651f;

	DirectX::XMFLOAT3 pos[12] =
	{
	DirectX::XMFLOAT3(-X, 0.0f, Z), DirectX::XMFLOAT3(X, 0.0f, Z),
	DirectX::XMFLOAT3(-X, 0.0f, -Z), DirectX::XMFLOAT3(X, 0.0f, -Z),
	DirectX::XMFLOAT3(0.0f, Z, X), DirectX::XMFLOAT3(0.0f, Z, -X),
	DirectX::XMFLOAT3(0.0f, -Z, X), DirectX::XMFLOAT3(0.0f, -Z, -X),
	DirectX::XMFLOAT3(Z, X, 0.0f), DirectX::XMFLOAT3(-Z, X, 0.0f),
	DirectX::XMFLOAT3(Z, -X, 0.0f), DirectX::XMFLOAT3(-Z, -X, 0.0f)
	};

	uint32 k[60] =
	{
	1,4,0, 4,9,0, 4,5,9, 8,5,4, 1,8,4,
	1,10,8, 10,3,8, 8,3,5, 3,2,5, 3,7,2,
	3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
	10,1,6, 11,0,9, 2,11,9, 5,2,9, 11,2,7
	};

	meshData.m_vertices.resize(12);

	meshData.m_indices32.assign(&k[0], &k[60]);

	for (uint32 i = 0; i < 12; ++i)
		meshData.m_vertices[i].m_position = pos[i];

	for (uint32 i = 0; i < numSubdivisions; ++i)
		Subdivide(meshData);

	// Project vertices onto sphere and scale.
	for (uint32 i = 0; i < meshData.m_vertices.size(); ++i)
	{
		// Project onto unit sphere.

		// Project onto sphere.
		DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&meshData.m_vertices[i].m_position));
		DirectX::XMVECTOR p = DirectX::XMVectorScale(n, radius);

		DirectX::XMStoreFloat3(&meshData.m_vertices[i].m_position, p);
		DirectX::XMStoreFloat3(&meshData.m_vertices[i].m_normal, n);

		// Derive texture coordinates from spherical
		//coordinates.
		float theta =
			atan2f(meshData.m_vertices[i].m_position.z,
				meshData.m_vertices[i].m_position.x);

		// Put in [0, 2pi].
		if (theta < 0.0f)
			theta += DirectX::XM_2PI;

		float phi = acosf(meshData.m_vertices[i].m_position.y
			/ radius);

		meshData.m_vertices[i].m_texC.x = theta / DirectX::XM_2PI;
		meshData.m_vertices[i].m_texC.y = phi / DirectX::XM_PI;

		// Partial derivative of P with respect to theta
		meshData.m_vertices[i].m_tangentU.x = -
			radius * sinf(phi) * sinf(theta);

		meshData.m_vertices[i].m_tangentU.y = 0.0f;

		meshData.m_vertices[i].m_tangentU.z =
			+radius * sinf(phi) * cosf(theta);

		DirectX::XMVECTOR T = XMLoadFloat3(&meshData.m_vertices[i].m_tangentU);
		DirectX::XMStoreFloat3(&meshData.m_vertices[i].m_tangentU,
			DirectX::XMVector3Normalize(T));
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float bottomRadius, float topRadius, float height, uint32 sliceCount, uint32 stackCount)
{
	MeshData meshData;

	//
	// Build Stacks.
	//

	float stackHeight = height / stackCount;

	// Amount to increment radius as we move up each stack level from bottom to top.
	float radiusStep = (topRadius - bottomRadius) / stackCount;

	uint32 ringCount = stackCount + 1;

	// Compute vertices for each stack ring starting at the bottom and moving up.
	for (uint32 i = 0; i < ringCount; ++i)
	{
		float y = -0.5f * height + i * stackHeight;
		float r = bottomRadius + i * radiusStep;

		// vertices of ring
		float dTheta = 2.0f * DirectX::XM_PI / sliceCount;
		for (uint32 j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;

			float c = cosf(j * dTheta);
			float s = sinf(j * dTheta);

			vertex.m_position = DirectX::XMFLOAT3(r * c, y, r * s);

			vertex.m_texC.x = (float)j / sliceCount;
			vertex.m_texC.y = 1.0f - (float)i / stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			/*
				y(v) = h - hv for v in[0, 1].
				r(v) = r1 + (r0-r1)v

				x(t, v) = r(v)*cos(t)
				y(t, v) = h - hv
				z(t, v) = r(v)*sin(t)

				dx/dt = -r(v)*sin(t)
				dy/dt = 0
				dz/dt = +r(v)*cos(t)

				dx/dv = (r0-r1)*cos(t)
				dy/dv = -h
				dz/dv = (r0-r1)*sin(t)
			*/

			// This is unit length.
			vertex.m_tangentU = DirectX::XMFLOAT3(-s, 0.0f, c);

			float dr = bottomRadius - topRadius;
			DirectX::XMFLOAT3 bitangent(dr * c, -height, dr * s);

			DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&vertex.m_tangentU);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&bitangent);
			DirectX::XMVECTOR N = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(T, B));
			DirectX::XMStoreFloat3(&vertex.m_normal, N);

			meshData.m_vertices.push_back(vertex);
		}
	}

	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	uint32 ringVertexCount = sliceCount + 1;

	// Compute indices for each stack.
	for (uint32 i = 0; i < stackCount; ++i)
	{
		for (uint32 j = 0; j < sliceCount; ++j)
		{
			meshData.m_indices32.push_back(i * ringVertexCount + j);
			meshData.m_indices32.push_back((i + 1) * ringVertexCount + j);
			meshData.m_indices32.push_back((i + 1) * ringVertexCount + j + 1);

			meshData.m_indices32.push_back(i * ringVertexCount + j);
			meshData.m_indices32.push_back((i + 1) * ringVertexCount + j + 1);
			meshData.m_indices32.push_back(i * ringVertexCount + j + 1);
		}
	}

	BuildCylinderTopCap(topRadius, height, sliceCount, meshData);
	BuildCylinderBottomCap(bottomRadius, height, sliceCount, meshData);

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float x, float y, float w, float h, float depth)
{
	MeshData meshData;

	meshData.m_vertices.resize(4);
	meshData.m_indices32.resize(6);

	// Position coordinates specified in NDC space.
	meshData.m_vertices[0] = Vertex(x, y - h, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	meshData.m_vertices[1] = Vertex(x, y, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);

	meshData.m_vertices[2] = Vertex(x + w, y, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	meshData.m_vertices[3] = Vertex(x + w, y - h, depth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	meshData.m_indices32[0] = 0;
	meshData.m_indices32[1] = 1;
	meshData.m_indices32[2] = 2;

	meshData.m_indices32[3] = 0;
	meshData.m_indices32[4] = 2;
	meshData.m_indices32[5] = 3;

	return meshData;
}

void GeometryGenerator::Subdivide(MeshData& meshData)
{
	// Save a copy of the input geometry.
	MeshData inputCopy = meshData;

	meshData.m_vertices.resize(0);
	meshData.m_indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	uint32 numTris = (uint32)inputCopy.m_indices32.size() / 3;
	for (uint32 i = 0; i < numTris; ++i)
	{
		Vertex v0 = inputCopy.m_vertices[inputCopy.m_indices32[i * 3 + 0]];
		Vertex v1 = inputCopy.m_vertices[inputCopy.m_indices32[i * 3 + 1]];
		Vertex v2 = inputCopy.m_vertices[inputCopy.m_indices32[i * 3 + 2]];

		//
		// Generate the midpoints.
		//

		Vertex m0 = MidPoint(v0, v1);
		Vertex m1 = MidPoint(v1, v2);
		Vertex m2 = MidPoint(v0, v2);

		meshData.m_vertices.push_back(v0); // 0
		meshData.m_vertices.push_back(v1); // 1
		meshData.m_vertices.push_back(v2); // 2
		meshData.m_vertices.push_back(m0); // 3
		meshData.m_vertices.push_back(m1); // 4
		meshData.m_vertices.push_back(m2); // 5

		meshData.m_indices32.push_back(i * 6 + 0);
		meshData.m_indices32.push_back(i * 6 + 3);
		meshData.m_indices32.push_back(i * 6 + 5);

		meshData.m_indices32.push_back(i * 6 + 3);
		meshData.m_indices32.push_back(i * 6 + 4);
		meshData.m_indices32.push_back(i * 6 + 5);

		meshData.m_indices32.push_back(i * 6 + 5);
		meshData.m_indices32.push_back(i * 6 + 4);
		meshData.m_indices32.push_back(i * 6 + 2);

		meshData.m_indices32.push_back(i * 6 + 3);
		meshData.m_indices32.push_back(i * 6 + 1);
		meshData.m_indices32.push_back(i * 6 + 4);
	}
}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& v0, const Vertex& v1)
{
	DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.m_position);
	DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.m_position);

	DirectX::XMVECTOR n0 = DirectX::XMLoadFloat3(&v0.m_normal);
	DirectX::XMVECTOR n1 = DirectX::XMLoadFloat3(&v1.m_normal);

	DirectX::XMVECTOR tan0 = DirectX::XMLoadFloat3(&v0.m_tangentU);
	DirectX::XMVECTOR tan1 = DirectX::XMLoadFloat3(&v1.m_tangentU);

	DirectX::XMVECTOR tex0 = DirectX::XMLoadFloat2(&v0.m_texC);
	DirectX::XMVECTOR tex1 = DirectX::XMLoadFloat2(&v1.m_texC);

	// Compute the midpoints of all the attributes.  Vectors need to be normalized
	// since linear interpolating can make them not unit length.

	DirectX::XMVECTOR pos = DirectX::XMVectorScale(DirectX::XMVectorAdd(p0, p1), 0.5);
	DirectX::XMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMVectorScale(DirectX::XMVectorAdd(n0, n1), 0.5));
	DirectX::XMVECTOR tangent = DirectX::XMVector3Normalize(DirectX::XMVectorScale(DirectX::XMVectorAdd(tan0, tan1), 0.5));
	DirectX::XMVECTOR tex = DirectX::XMVectorScale(DirectX::XMVectorAdd(tex0, tex1), 0.5);

	Vertex v;
	DirectX::XMStoreFloat3(&v.m_position, pos);
	DirectX::XMStoreFloat3(&v.m_normal, normal);
	DirectX::XMStoreFloat3(&v.m_tangentU, tangent);
	DirectX::XMStoreFloat2(&v.m_texC, tex);

	return v;
}

void GeometryGenerator::BuildCylinderTopCap(float topRadius, float height, uint32 sliceCount, MeshData& meshData)
{
	uint32 baseIndex = (uint32)meshData.m_vertices.size();

	float y = 0.5f * height;
	float dTheta = 2.0f * DirectX::XM_PI / sliceCount;

	// Duplicate cap ring vertices because the texture coordinates and normals differ.
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = topRadius * cosf(i * dTheta);
		float z = topRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;
		meshData.m_vertices.push_back(Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex.
	meshData.m_vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Index of center vertex.
	uint32 centerIndex = (uint32)meshData.m_vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.m_indices32.push_back(centerIndex);
		meshData.m_indices32.push_back(baseIndex + i + 1);
		meshData.m_indices32.push_back(baseIndex + i);
	}
}
void GeometryGenerator::BuildCylinderBottomCap(float bottomRadius, float height, uint32 sliceCount, MeshData& meshData)
{
	uint32 baseIndex = (uint32)meshData.m_vertices.size();
	float y = -0.5f * height;

	// vertices of ring
	float dTheta = 2.0f * DirectX::XM_PI / sliceCount;
	for (uint32 i = 0; i <= sliceCount; ++i)
	{
		float x = bottomRadius * cosf(i * dTheta);
		float z = bottomRadius * sinf(i * dTheta);

		// Scale down by the height to try and make top cap texture coord area proportional to base.
		float u = x / height + 0.5f;
		float v = z / height + 0.5f;

		meshData.m_vertices.push_back(Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v));
	}

	// Cap center vertex.
	meshData.m_vertices.push_back(Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f));

	// Cache the index of center vertex.
	uint32 centerIndex = (uint32)meshData.m_vertices.size() - 1;

	for (uint32 i = 0; i < sliceCount; ++i)
	{
		meshData.m_indices32.push_back(centerIndex);
		meshData.m_indices32.push_back(baseIndex + i);
		meshData.m_indices32.push_back(baseIndex + i + 1);
	}
}
