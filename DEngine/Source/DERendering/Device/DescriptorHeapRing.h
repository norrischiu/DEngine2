#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>

namespace DE
{
class DescriptorHeapRing
{
public:
	DescriptorHeapRing() = default;

	void Init(const GraphicsDevice& device, D3D12_DESCRIPTOR_HEAP_TYPE type)
	{
		m_Heap.Init(device, 65536, type, true);
	}

	DescriptorHeap Alloc(uint32_t num)
	{
		DescriptorHeap subHeap;
		subHeap.ptr = m_Heap.ptr;
		subHeap.current = m_Heap.current;
		subHeap.gpuCurrent = m_Heap.gpuCurrent;
		subHeap.stride = m_Heap.stride;
		
		m_Heap.Increment(num);

		return subHeap;
	}

	void Reset()
	{
		m_Heap.Reset();
	}

	ID3D12DescriptorHeap* Raw()
	{
		return m_Heap.ptr.Get();
	}

private:
	DescriptorHeap m_Heap;
};
}