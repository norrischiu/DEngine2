#pragma once

#include <DECore\Math\simdmath.h>
#include <DERendering\DataType\Pool.h>

namespace DE
{

/**	@brief Contains point light definition*/
struct PointLight final : public Pool<PointLight, 64>
{
public:
	Vector3 position;
	Vector3 color;
	float intensity;
};

}

