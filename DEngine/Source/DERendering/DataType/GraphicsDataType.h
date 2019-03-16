#pragma once

#include <cstdint>
#include <vector>

#include <DERendering\DataType\GraphicsResourceType.h>

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
};

/**	@brief Contains vertex and index buffer of a mesh*/
struct Mesh final
{

public:
	Vector<float3> m_Vertices;
	Vector<float3> m_Normals;
	Vector<float3> m_Tangents;
	Vector<uint3> m_Indices;
	Vector<float2> m_TexCoords;
	uint32_t m_MaterialID;
};

struct Meshes final
{
	static Mesh m_Meshes[512];
	//static uint32_t m_iNum;
};

struct Materials final
{
	static Material m_Materials[512];
	//static uint32_t m_iNum;
};

}