#pragma once

// Cpp
#include <stdint.h>

inline size_t Align(size_t size, size_t align)
{
	return size + align - 1 & ~(align - 1);
}