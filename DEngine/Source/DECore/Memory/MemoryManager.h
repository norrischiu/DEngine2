// MemoryManager.h: singleton class of memory manager
#ifndef MEMORYMANAGER_H_
#define MEMORYMANAGER_H_

// Cpp
#include <iostream>
#include <mutex>
// Engine
#include "MemoryPool.h"
#include "Handle.h"

namespace DE
{

const uint32_t MEMORY_POOL_NUM = 23;		// memory pool configuration: number of pool
const uint32_t MEMORY_POOL_CONFIG[][2] =	// memory pool configuration: block size and nubmer of block pair
{
	// already 16-byte aligned
	// block size, number of block
	{ 16,       65536 },
	{ 32,       4096 },
	{ 48,       4096 },
	{ 64,       4096 },
	{ 80,       2048 },
	{ 96,       2048 },
	{ 112,      2048 },
	{ 128,      2048 },
	{ 256,      2048 },
	{ 512,      2048 },
	{ 1024,     2048 },
	{ 2048,     2048 },
	{ 4096,     512 },
	{ 8192,     512 },
	{ 16384,    512 },
	{ 32768,    512 },
	{ 65536,    256 },
	{ 131072,   32 },
	{ 262144,   32 },  // low res texture (256 * 256 * 4)
	{ 524288,   32 }, 
	{ 1048576,  16 }, // mid res texture (512 * 512 * 4)
	{ 4194304,  64 }, // high res texture (1024 * 1024 * 4)
	{ 8388608,	4 },
	{ 16777216, 4 }  // ultra high res texture (2048 * 2048 * 4)
};

class MemoryManager
{

public:

	/********************************************************************************
	*	--- Function:
	*	ConstructDefaultPool()
	*	This function will allocated memory from the system to memory pool according
	*	to the defined configuration as in MEMORY_POOL_CONFIG
	*
	*	--- Parameters:
	*	@ void
	*
	*	--- Return:
	*	@ void
	********************************************************************************/
	void ConstructDefaultPool();

	/********************************************************************************
	*	--- Function:
	*	Destruct()
	*	This function will free all the heap memory allocated for the pools
	*
	*	--- Parameters:
	*	@ void
	*
	*	--- Return:
	*	@ void
	********************************************************************************/
	void Destruct();

	/********************************************************************************
	*	--- Function:
	*	Allocate(size_t)
	*	This function will return a handle with appropriate pool and block index,
	*	and update the book keeping record in memory pool
	*
	*	--- Parameters:
	*	@ size: size of the memory requested, typically pass by sizeof(class)
	*
	*	--- Return:
	*	@ void
	********************************************************************************/
	Handle Allocate(size_t size);

	/********************************************************************************
	*	--- Function:
	*	Free(Handle)
	*	This function will free the memory block referred by the give handle back 
	*	to the pool
	*
	*	--- Parameters:
	*	@ hle: a handle
	*
	*	--- Return:
	*	@ void
	********************************************************************************/
	void Free(Handle hle);

	/********************************************************************************
	*	--- Function:
	*	GetMemoryAddressFromHandle(Handle)
	*	This function will return the raw address stored with reference to handle
	*
	*	--- Parameters:
	*	@ hle: a handle
	*
	*	--- Return:
	*	@ void*: raw address referred by given handle
	********************************************************************************/
	// Get the raw address stored with reference to handle
	void* GetMemoryAddressFromHandle(Handle hle) const;

	/********************************************************************************
	*	--- Static Function:
	*	GetInstance()
	*	This function will return the singleton instance of MemoryManager
	*
	*	--- Parameters:
	*	@ void
	*
	*	--- Return:
	*	@ MemoryManager*: the singleton instance
	********************************************************************************/
	static DllExport MemoryManager* GetInstance()
	{
		if (!m_pInstance)
		{
			m_pInstance = new MemoryManager();
		}
		return m_pInstance;
	};

	static DllExport void* Start()
	{
		return m_pInstance->m_pRawHeapStart;
	}

private:

	/********************************************************************************
	*	--- Default Constructor
	********************************************************************************/
	MemoryManager() = default;

	/********************************************************************************
	*	--- Function:
	*	alignedAddress(void*);
	*	This function will return an aligned address of the given address according
	*	to the predefined memory alignement requirement
	*
	*	--- Parameters:
	*	@ ptr: memory address
	*
	*	--- Return:
	*	@ void*: aligned address
	********************************************************************************/
	void* alignedAddress(void* ptr);
	
	static MemoryManager*					m_pInstance;	// singleton instance
	void*									m_pRawHeapStart;	// Raw heap start address
	MemoryPool*								m_pPool[MEMORY_POOL_NUM];	// All memory blocks' pools

	std::mutex								m_mutex;
};

};
#endif

