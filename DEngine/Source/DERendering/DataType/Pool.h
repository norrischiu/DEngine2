#pragma once

namespace DE
{

template <class T, uint64_t N>
class Pool
{

public:
	static T& Get(uint32_t i)
	{
		return m_Objects[i];
	}

	static uint64_t Size()
	{
		return N;
	}

private:
	static T m_Objects[N];
};

template <class T, uint64_t N>
T Pool<T, N>::m_Objects[N];

}
