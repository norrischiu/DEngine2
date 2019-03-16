#include <DECore/DECore.h>
#include <DECore/Memory/MemoryManager.h>
#include <DECore/Memory/Handle.h>

#if 0
void* operator new(std::size_t size)
{
	DE::Handle handle(size); // a temp handle, because porting from old code
	return DE::MemoryManager::GetInstance()->GetMemoryAddressFromHandle(handle);
}

void operator delete(void* p)
{
	uintptr_t target = reinterpret_cast<uintptr_t>(p);
	uintptr_t ptr = reinterpret_cast<uintptr_t>(DE::MemoryManager::Start());
	for (uint32_t i = 0; i < DE::MEMORY_POOL_NUM; ++i)
	{
		uint32_t size = DE::MEMORY_POOL_CONFIG[i][0] * DE::MEMORY_POOL_CONFIG[i][1];
		if (ptr + size > target)
		{
			uint32_t poolIndex = i;
			uint32_t blockIndex = static_cast<uint32_t>((target - ptr) / DE::MEMORY_POOL_CONFIG[i][0]);
			DE::Handle h(poolIndex, blockIndex);
			h.Free();
			return;
		}
		ptr += size;
	}
}
#endif