#pragma once

#include <cstdint>
#include <vector>

#include <DERendering\DataType\GraphicsResourceType.h>
#include <DERendering\DataType\Pool.h>

namespace DE
{

struct float2 final
{
	float x, y;
}; 
	
struct float3 final
{
	float x, y, z;
};

inline float3 operator+(const float3 lhs, const float3 rhs)
{
	return float3{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

struct float4 final
{
	float x, y, z, w;

	inline float4() = default;
	inline float4(float x, float y, float z, float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}
	inline float4(const float3 xyz, const float w)
	{
		this->x = xyz.x;
		this->y = xyz.y;
		this->z = xyz.z;
		this->w = w;
	}
};

struct uint3 final
{
	uint32_t x, y, z;
};

enum ShadingType : uint8_t
{
	None,
	AlbedoOnly,
	Textured,
};

struct MaterialParameter
{
	float params[8];
};

struct Material final : public Pool<Material, 512>
{
	Material() = default;
	Material(const Material&) = delete;
	Material& operator=(const Material&) = delete;
	ShadingType shadingType;
	Texture m_Textures[5];
	// 8 float
	float3 albedo;
	float metallic;
	float roughness;
	float ao;
	float2 padding;
};

/**	@brief Contains vertex and index buffer of a mesh*/
struct Mesh final : public Pool<Mesh, 512>
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

}