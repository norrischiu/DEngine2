#pragma once

// Cpp
#include <cassert>
#include <atomic>

namespace DE
{

template <class T, uint64_t N>
class Pool
{

public:
	static uint32_t Create()
	{
		uint32_t index = m_iCount.fetch_add(1);
		return index;
	}

	static T& Get(uint32_t i)
	{
		assert(i < m_iCount);
		return m_Objects[i];
	}

	static uint64_t Size()
	{
		return N;
	}

private:
	static T m_Objects[N];
	static std::atomic_uint32_t m_iCount;
};

template <class T, uint64_t N> T Pool<T, N>::m_Objects[N];
template <class T, uint64_t N> std::atomic_uint32_t Pool<T, N>::m_iCount = 0;

}
