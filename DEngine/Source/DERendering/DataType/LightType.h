#pragma once

#include <DECore\Math\simdmath.h>
#include <DERendering\DataType\GraphicsDataType.h>
#include <DERendering\DataType\Pool.h>

namespace DE
{

/**	@brief Contains point light definition*/
struct PointLight final : public Pool<PointLight, 64>
{
public:
	bool enable;
	float3 position;
	float3 color;
	float intensity;
};

struct QuadLight final : public Pool<QuadLight, 8>
{
public:
	bool enable;
	float3 position;
	float width;
	float height;
	float3 color;
	float intensity;
	uint32_t mesh;
};

}

