#include "GpuBufferFencedPool.h"
#include <DERendering/Device/RenderDevice.h>

namespace DE
{

 void GpuBufferFencedPool::Init(RenderDevice * pRenderDevice, D3D12_HEAP_TYPE heapType, uint32_t size, uint32_t numBuffers, uint32_t releaseLatency /*= DEFAULT_RELEASE_LATENCY*/)
{
	m_pRenderDevice = pRenderDevice;
	m_HeapType = heapType;
	m_iInitNumBuffers = numBuffers;
	m_iInitBufferSize = size;
	m_iReleaseLatency = releaseLatency;

	m_Buffers.reserve(numBuffers);
	for (uint32_t i = 0; i < numBuffers; ++i)
	{
		AllocateNewFencedBuffer(pRenderDevice->m_FenceValue, max(DEFAULT_BUFFER_SIZE, size));
	}
}

 Buffer GpuBufferFencedPool::Allocate(uint64_t fence, uint32_t size)
 {
	 std::lock_guard<std::mutex> lock(m_Mutex);

	 // find buffer
	 FencedBuffer& buffer = FindFencedBuffer(fence, size);

	 const uint32_t offset = buffer.offset;
	 buffer.offset += size;
	 return Buffer{ buffer.buffer.ptr, buffer.buffer.mappedPtr, buffer.buffer.m_Desc, offset };
 }

 void GpuBufferFencedPool::Reset(uint64_t fence)
 {
	 for (auto& b : m_Buffers)
	 {
		 if (fence > b.fence)
		 {
			 b.offset = 0;
			 b.idle++;
		 }
	 }

	 if (m_Buffers.size() > m_iInitNumBuffers && m_Buffers.back().idle >= m_iReleaseLatency)
	 {
		 m_Buffers.pop_back();
	 }
 }

 GpuBufferFencedPool::FencedBuffer & GpuBufferFencedPool::FindFencedBuffer(uint64_t fence, uint32_t size)
 {
	 for (auto& b : m_Buffers)
	 {
		 if (fence >= b.fence)
		 {
			 if (b.offset + size > b.size)
			 {
				 continue;
			 }
			 else
			 {
				 b.fence = fence;
				 b.idle = 0;
				 return b;
			 }
		 }
	 }

	 // not enough memory in existing buffers
	 return AllocateNewFencedBuffer(fence, max(m_iInitBufferSize, size)); // use the initial size or current requested size
 }

 GpuBufferFencedPool::FencedBuffer& GpuBufferFencedPool::AllocateNewFencedBuffer(uint64_t fence, uint32_t size)
{
	uint32_t bufferSize = max(DEFAULT_BUFFER_SIZE, size);

	Buffer newBuffer;
	newBuffer.Init(m_pRenderDevice->m_Device, bufferSize, m_HeapType);
	if (m_HeapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		newBuffer.mappedPtr = newBuffer.Map();
	}

	m_Buffers.push_back({ newBuffer, 0, bufferSize, fence });
	return m_Buffers.back();
}

}
