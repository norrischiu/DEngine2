#pragma once

// Engine
#include <DECore/Container/Vector.h>
#include <DERendering/DataType/GraphicsResourceType.h>
// C++
#include <mutex>
#include <algorithm>

namespace DE
{
class RenderDevice;

/** @brief A self growing & shrinking pool of gpu buffers,
*	allocation and release are protected by gpu fence
*/
class GpuBufferFencedPool
{
private:
	static constexpr uint32_t DEFAULT_BUFFER_SIZE = 64 * 1024; // 64KB, avoiding allocate a too small buffer
	static constexpr uint32_t DEFAULT_RELEASE_LATENCY = 64;

public:

	/** @brief Init the pool with numBuffers of size with heapType 
	*	
	*	@param pRenderDevice
	*	@param heapType
	*	@param size size of buffers to be allocated initially
	*	@param numBuffers num of buffers to be allocated initially, with size
	*	@param releaseLatency how many idle frames to wait before release a buffer in the pool
	*/
	void Init(RenderDevice* pRenderDevice, D3D12_HEAP_TYPE heapType, uint32_t size, uint32_t numBuffers, uint32_t releaseLatency = DEFAULT_RELEASE_LATENCY);

	/** @brief Release pool memory */
	inline void Release()
	{
		m_Buffers.clear();
	}

	/** @brief Suballocate a slice of buffer from the pool
	*
	*	@param fence current fence at render device
	*	@param size
	*	@return a Buffer object
	*/
	Buffer Allocate(uint64_t fence, uint32_t size);

	/** @brief Reset the buffers offset for those used buffers 
	*
	*	@param fence current fence at render device
	*/
	void Reset(uint64_t fence);

private:
	struct FencedBuffer
	{
		Buffer buffer;
		uint32_t offset;
		uint32_t size;
		uint64_t fence;
		uint32_t idle;
	};
	
	/** @brief Find a buffer that is not in use and has enough space
	*
	*	@param fence current fence at render device
	*	@param size
	*	@return a FencedBuffer object
	*/
	FencedBuffer& FindFencedBuffer(uint64_t fence, uint32_t size);

	/** @brief Internally allocate a new buffer with size or a larger initial size at pool construction
	*
	*	@param fence current fence at render device
	*	@param size
	*	@return a FencedBuffer object
	*/
	FencedBuffer& AllocateNewFencedBuffer(uint64_t fence, uint32_t size);

	RenderDevice* m_pRenderDevice;
	Vector<FencedBuffer> m_Buffers;
	uint32_t m_iInitNumBuffers;
	uint32_t m_iInitBufferSize;
	uint32_t m_iReleaseLatency;
	D3D12_HEAP_TYPE m_HeapType;
	std::mutex m_Mutex;
};
}