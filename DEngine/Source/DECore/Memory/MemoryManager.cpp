// MemoryManager.cpp

#include <DECore/DECore.h>
#include "MemoryManager.h"

namespace DE
{

MemoryManager* MemoryManager::m_pInstance;

void MemoryManager::ConstructDefaultPool()
{
	uint32_t heapSize = 0;

	for (uint32_t i = 0; i < MEMORY_POOL_NUM; ++i)
	{
		heapSize += MEMORY_POOL_CONFIG[i][0] * MEMORY_POOL_CONFIG[i][1] + sizeof(uint32_t) * 4 + sizeof(uint32_t) * MEMORY_POOL_CONFIG[i][1];
	}
	m_pRawHeapStart = std::malloc(heapSize);
	void* heapStart = alignedAddress(m_pRawHeapStart); // align the start of memory pool

	for (uint32_t i = 0; i < MEMORY_POOL_NUM; ++i)
	{
		m_pPool[i] = MemoryPool::Construct(MEMORY_POOL_CONFIG[i][0], MEMORY_POOL_CONFIG[i][1], heapStart);
	}
}

void MemoryManager::Destruct()
{
	std::free(m_pRawHeapStart);
	*m_pPool = nullptr;
	delete this;
}

Handle MemoryManager::Allocate(size_t size)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	for (uint32_t i = 0; i < MEMORY_POOL_NUM; ++i)
	{
		if (size <= MEMORY_POOL_CONFIG[i][0])
		{
			if (m_pPool[i]->m_iFreeBlockNum == 0)
			{
				assert(false && "no block left");
			}
			int index = m_pPool[i]->m_pFreeList[m_pPool[i]->m_iFreeBlockIndex];
			m_pPool[i]->m_iFreeBlockNum--;
			m_pPool[i]->m_iFreeBlockIndex++;
			return Handle(i, index);
		}
	}

	assert(false && "no block fits");
	return NULL;
}

void MemoryManager::Free(Handle hle)
{
	memset(hle.Raw(), 0, m_pPool[hle.m_poolIndex]->m_iBlockSize);
	int index = m_pPool[hle.m_poolIndex]->m_iFreeBlockIndex - 1;
	m_pPool[hle.m_poolIndex]->m_iFreeBlockIndex--;
	if (index == -1)
	{
		m_pPool[hle.m_poolIndex]->m_iFreeBlockIndex = MEMORY_POOL_CONFIG[hle.m_poolIndex][1] - 1;
		index = MEMORY_POOL_CONFIG[hle.m_poolIndex][1] - 1;
	}
	m_pPool[hle.m_poolIndex]->m_pFreeList[index] = hle.m_blockIndex;
	m_pPool[hle.m_poolIndex]->m_iFreeBlockNum++;
}

void * MemoryManager::GetMemoryAddressFromHandle(Handle hle) const
{
	if (hle.m_counter == 0)
	{
		return nullptr;
	}
	assert((((uint64_t)m_pPool[hle.m_poolIndex] + sizeof(uint32_t) * 4 + sizeof(uint32_t) * MEMORY_POOL_CONFIG[hle.m_poolIndex][1] + m_pPool[hle.m_poolIndex]->m_iBlockSize * hle.m_blockIndex)) % 16 == 0); // temp check
	return (void*)((uint64_t)m_pPool[hle.m_poolIndex] + sizeof(uint32_t) * 4 + sizeof(uint32_t) * MEMORY_POOL_CONFIG[hle.m_poolIndex][1] + m_pPool[hle.m_poolIndex]->m_iBlockSize * hle.m_blockIndex);
}

// Return a aligned address according to the alignment
void* MemoryManager::alignedAddress(void* ptr)
{
	uintptr_t ptr_ = reinterpret_cast<uintptr_t>(ptr);
	uint32_t adjustment = static_cast<uint32_t>(ptr_ % MEMORY_ALIGNMENT);
	if (adjustment)
	{
		return reinterpret_cast<void*>(ptr_ + MEMORY_ALIGNMENT - adjustment);
	}
	else
	{
		return ptr;
	}
}

};