#pragma once

#include <DECore/Container/Vector.h>
#include <math.h>

namespace DE
{

struct RenderConstant
{
static constexpr float CubeVertices[] =
{
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	1.0f,  1.0f, -1.0f,
	1.0f,  1.0f,  1.0f,
	1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f, -1.0f,
	1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	1.0f, -1.0f,  1.0f
};

struct GeneratedMesh
{
	Vector<float3> vertices;
	Vector<uint3> indices;
};
static GeneratedMesh GenerateIcosphere(uint32_t subdivideLevel = 2)
{
	const static float t = (1.0 + static_cast<float>(sqrt(5.0))) / 2.0;
	const static float IcosahedronVertices[] =
	{
		-1.0f, t, 0.0f,
		1.0f, t, 0.0f,
		-1.0f, -t, 0.0f,
		1.0f, -t, 0.0f,

		0.0f, -1.0f, t,
		0.0f, 1.0f, t,
		0.0f, -1.0f, -t,
		0.0f, 1.0f, -t,

		t, 0.0f, -1.0f,
		t, 0.0f, 1.0f,
		-t, 0.0f, -1.0f,
		-t, 0.0f, 1.0f,
	};

	Vector<float3> vertices;
	vertices.reserve(12);
	for (uint32_t i = 0; i < 12; ++i)
	{
		float v0 = IcosahedronVertices[i * 3];
		float v1 = IcosahedronVertices[i * 3 + 1];
		float v2 = IcosahedronVertices[i * 3 + 2];
		float length = static_cast<float>(sqrt(v0 * v0 + v1 * v1 + v2 * v2));
		vertices.push_back(float3{ v0 / length, v1 / length , v2 / length  });
	}

	static constexpr uint32_t IcosahedronIndices[] =
	{
		0, 11, 5,
		0, 5, 1,
		0, 1, 7,
		0, 7, 10,
		0, 10, 11,

		// 5 adjacent faces
		1, 5, 9,
		5, 11, 4,
		11, 10, 2,
		10, 7, 6,
		7, 1, 8,

		// 5 faces around point 3
		3, 9, 4,
		3, 4, 2,
		3, 2, 6,
		3, 6, 8,
		3, 8, 9,

		// 5 adjacent faces
		4, 9, 5,
		2, 4, 11,
		6, 2, 10,
		8, 6, 7,
		9, 8, 1,
	};
	Vector<uint3> indices;
	indices.reserve(20);
	for (uint32_t i = 0; i < 20; i++)
	{
		indices.push_back(uint3{ IcosahedronIndices[i * 3], IcosahedronIndices[i * 3 + 1], IcosahedronIndices[i * 3 + 2] });
	}

	// refine triangles
	Vector<float3> finalVertices;
	Vector<uint3> finalIndices;
	for (uint32_t level = 0; level < subdivideLevel; ++level)
	{
		const uint32_t oldSize = static_cast<uint32_t>(vertices.size());
		for (const uint3& tri : indices)
		{
			float3 v0 = vertices[tri.x];
			float3 v1 = vertices[tri.y];
			float3 v2 = vertices[tri.z];

			uint32_t i0 = static_cast<uint32_t>(vertices.size());
			vertices.push_back(float3{ (v0.x + v1.x) / 2.0f, (v0.y + v1.y) / 2.0f, (v0.z + v1.z) / 2.0f });
			uint32_t i1 = (i0 + 1); 
			vertices.push_back(float3{ (v1.x + v2.x) / 2.0f, (v1.y + v2.y) / 2.0f, (v1.z + v2.z) / 2.0f });
			uint32_t i2 = (i1 + 1);
			vertices.push_back(float3{ (v2.x + v0.x) / 2.0f, (v2.y + v0.y) / 2.0f, (v2.z + v0.z) / 2.0f });

			finalIndices.push_back({ tri.x , i0, i2});
			finalIndices.push_back({ tri.y , i1, i0});
			finalIndices.push_back({ tri.z , i2, i1});
			finalIndices.push_back({ i0, i1, i2});
		}
		indices = std::move(finalIndices);
		finalIndices.clear();
		for (uint32_t i = oldSize; i < vertices.size(); ++i)
		{
			const auto& vertex = vertices[i];
			float length = static_cast<float>(sqrt(vertex.x * vertex.x + vertex.y * vertex.y + vertex.z * vertex.z));
			vertices[i] = float3{ vertex.x / length, vertex.y / length, vertex.z / length };
		}
	}

	return GeneratedMesh{ std::move(vertices), std::move(indices) };
};
};

}