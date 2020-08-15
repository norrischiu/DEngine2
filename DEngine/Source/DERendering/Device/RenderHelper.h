#pragma once

// Engine
#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>

namespace DE
{
namespace RenderHelper
{

/** @brief A iterator class to update and get constant buffer resource for binding */
template <class T>
class ConstantBufferRange
{
public:

	ConstantBufferRange(const Buffer& buffer, uint32_t stride, uint32_t num)
		: buffer(buffer)
		, stride(stride)
		, num(num)
	{}
	~ConstantBufferRange() = default;

	/** @brief Increment the index underneath */
	ConstantBufferRange& operator++()
	{
		assert(index < num);
		index++;
		return *this;
	}

	/** @brief Return class casted pointer to current index */
	T* operator->()
	{
		return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(buffer.GetMappedPtr()) + index * stride);
	}

	/** @brief Return the constant resource at current index */
	const ConstantResource GetCurrentResource()
	{
		return ConstantResource().Buffer(buffer).Offset(index * stride);
	}

	/** @brief reset the current iterator to begin */
	void Reset()
	{
		index = 0;
	}

private:
	Buffer buffer;
	uint32_t stride;
	uint32_t num;
	uint32_t index = 0;
};

/** @brief Helper function to allocate constant buffer for multiple data */
template<class T>
static ConstantBufferRange<T> AllocateConstant(RenderDevice* pDevice, uint32_t num)
{
	const uint32_t stride = static_cast<uint32_t>(Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
	return ConstantBufferRange<T>(pDevice->SuballocateUploadBuffer(stride * num), stride, num);
}

}
}

