#pragma once

#include <cstdint>
#include <vector>

#include <DERendering\DataType\GraphicsResourceType.h>
#include <DERendering\DataType\Pool.h>

namespace DE
{
struct float4 final
{
	float x, y, z, w;
};

struct float3 final
{
	float x, y, z;
};

struct float2 final
{
	float x, y;
};

struct uint3 final
{
	uint32_t x, y, z;
};

struct Material final
{
	Material() = default;
	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;
	Texture m_Textures[5];
	// 8 float
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
	float2 padding;
};

/**	@brief Contains vertex and index buffer of a mesh*/
struct Mesh final
{

public:
	VertexBuffer m_Vertices;
	VertexBuffer m_Normals;
	VertexBuffer m_Tangents;
	VertexBuffer m_TexCoords;
	IndexBuffer m_Indices;

	uint32_t m_iNumIndices;
	uint32_t m_MaterialID;
};

class Meshes : public Pool<Mesh, 512>
{
};

struct Materials : public Pool<Material, 512>
{
};

}