#include "Handle.h"
#include "MemoryManager.h"

namespace DE
{

Handle::Handle(size_t size)
    : m_counter(1)
{
    *this = MemoryManager::GetInstance()->Allocate(size);
};

void Handle::Set(size_t size)
{
    assert(m_counter == 0);
    m_counter++;
    *this = MemoryManager::GetInstance()->Allocate(size);
}

void* Handle::Raw() const
{
    return MemoryManager::GetInstance()->GetMemoryAddressFromHandle(*this);
}

void Handle::Free()
{
    if (m_counter == 1)
    {
        MemoryManager::GetInstance()->Free(*this);
    }
    m_counter--;
}

}