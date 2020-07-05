#pragma once

// Cpp
#include <cassert>
#include <atomic>

namespace DE
{

namespace details
{
	static uint32_t id = 0;
}

template <class T, uint32_t N>
class Pool
{
public:
	static uint32_t ObjectId()
	{
		return m_iID;
	}

	static T& Create()
	{
		uint32_t i = m_iCount.fetch_add(1);
		m_Objects[i].m_iIndex = i;
		return Get(i);
	}

	static void Release()
	{
		for (const auto& obj : m_Objects)
		{
			obj.~T();
		}
	}

	static T& Get(uint32_t i)
	{
		assert(i < m_iCount);
		return m_Objects[i];
	}

	static uint32_t Size()
	{
		return N;
	}

	uint32_t Index() const
	{
		return m_iIndex;
	}

private:
	static T m_Objects[N];
	static std::atomic_uint32_t m_iCount;
	static uint32_t m_iID;

	uint32_t m_iIndex;
};

template <class T, uint32_t N> T Pool<T, N>::m_Objects[N];
template <class T, uint32_t N> std::atomic_uint32_t Pool<T, N>::m_iCount = 0;
template <class T, uint32_t N> uint32_t Pool<T, N>::m_iID = details::id++;

}
