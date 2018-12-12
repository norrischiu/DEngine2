#pragma once

#include "Math/simdmath.h"
#include "Math/Plane.h"

#include <cstdint>

namespace DE
{

template<class T1, class T2>
int32_t Intersects(T1 /*primitiveA*/, T2 /*primitiveB*/)
{
	return 0;
};

template<>
int32_t Intersects(Plane primitiveA, Vector4 primitiveB)
{
	float dist = primitiveA.m_vNormal.Dot(primitiveB);
	return dist > primitiveA.m_vNormal.GetW();
};

}
