#include <DECore/DECore.h>
#include <DECore/Memory/MemoryManager.h>
#include <DECore/Memory/Handle.h>

void* operator new(std::size_t size)
{
	DE::Handle handle(size); // a temp handle, because porting from old code
	return DE::MemoryManager::GetInstance()->GetMemoryAddressFromHandle(handle);
}

void operator delete(void* p)
{
	// TODO: revamp memory management system
}