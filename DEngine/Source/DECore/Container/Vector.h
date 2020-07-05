#pragma once

// Engine
#include <DECore/Memory/Handle.h>

namespace DE
{

/** @brief T is the class of the item to be stored
*		This is the default contiguous array to be used in DEngine, it behaves
*		in similar way as std::vector
*/
template <class T>
class MyArray
{
	using iterator = T*;
	using reference = T&;
	using const_iterator = const T*;
	using const_reference = const T&;

public:

	/** @brief Default constructor **/
	MyArray() = default;

	/** @brief	Construct an handle array with given size and construct elements
	*
	*	@param size
	*/
	MyArray(size_t size)
	{
		m_iSize = size;
		m_iCapacity = size;
		if (size > 0)
		{
			m_hElements.Set(sizeof(T) * size);
			m_pBegin = reinterpret_cast<T*>(m_hElements.Raw());
			for (uint32_t i = 0; i < size; ++i)
			{
				new (&m_pBegin[i]) T();
			}
		}
	}

	/** @brief	Move the other handle from another array, and invalidate it
	*
	*	@param other the other MyArray object
	*/
	MyArray(MyArray&& other)
	{
		m_hElements = other.m_hElements;
		other.m_hElements.m_counter = -1;
		m_hElements.m_counter++;
		m_iSize = other.m_iSize;
		m_iCapacity = other.m_iCapacity;
		m_pBegin = reinterpret_cast<T*>(m_hElements.Raw());
	}

	/** @brief	Move the other handle from another array, and invalidate it
	*
	*	@param other the other MyArray object
	*/
	const MyArray& operator=(MyArray&& other)
	{
		m_hElements = other.m_hElements;
		other.m_hElements.m_counter = -1;
		m_hElements.m_counter++;
		m_iSize = other.m_iSize;
		m_iCapacity = other.m_iCapacity;
		m_pBegin = reinterpret_cast<T*>(m_hElements.Raw());
		return *this;
	}

	/** @brief Free the memory used by this array's items */
	~MyArray()
	{
		for (uint32_t i = 0; i < m_iSize; ++i)
		{
			m_pBegin[i].~T();
		}
		m_hElements.Free();
	}

	/** @brief Return the current size
	*
	*	@return the current size (number of items) of this array
	*/
	inline size_t size() const
	{
		return m_iSize;
	}

	/** @brief Add an element at the end of this array
	*
	*	@param item the item to be added
	*/
	void push_back(const T& item)
	{
		if (m_iCapacity == 0)
		{
			reserve(1);
		}
		if (m_iSize >= m_iCapacity)
		{
			reserve(m_iCapacity * 2);
		}
		m_pBegin[m_iSize] = item;
		m_iSize++;
	}

	/** @brief Add an movable element at the end of this array
	*
	*	@param item the item to be added
	*/
	void push_back(T&& item)
	{
		if (m_iCapacity == 0)
		{
			reserve(1);
		}
		if (m_iSize >= m_iCapacity)
		{
			reserve(m_iCapacity * 2);
		}
		m_pBegin[m_iSize] = std::move(item);
		m_iSize++;
	}

	/** @brief Construct an element in place at the end of this array
	*
	*	@param args the parameter forward to T's constructor
	*/
	template<class... Args>
	T& emplace_back(Args&&... args)
	{
		if (m_iCapacity == 0)
		{
			reserve(1);
		}
		if (m_iSize >= m_iCapacity)
		{
			reserve(m_iCapacity * 2);
		}
		new (&m_pBegin[m_iSize]) T(std::forward<Args>(args)...);
		m_iSize++;

		return m_pBegin[m_iSize];
	}

	/** @brief Resize the array and construct all elements
	*
	*	@param size the new size
	*/
	void resize(size_t size)
	{
		reserve(size);
		for (uint32_t i = m_iSize; i < size; ++i)
		{
			push_back(T());
		}
		m_iSize = size;
	}	
	
	/** @brief Reserve the array's capavity
	*
	*	@param capacity the new capacity
	*/
	void reserve(std::size_t capacity)
	{
		std::size_t oldCapacity = m_iCapacity;
		if (capacity > oldCapacity)
		{
			m_iCapacity = capacity;
			Handle hNewElements(sizeof(T) * capacity);
			if (oldCapacity > 0)
			{
				memcpy(hNewElements.Raw(), m_hElements.Raw(), sizeof(T) * m_iSize);
				m_hElements.Free();
			}
			m_hElements = hNewElements;
			m_pBegin = reinterpret_cast<T*>(m_hElements.Raw());
		}
	}

	/** @brief Clear the items in this array, leaving the capacity and allocated memory unchanged */
	void clear()
	{
		if (m_iCapacity > 0)
		{
			memset(m_pBegin, NULL, sizeof(T) * m_iSize);
		}
		m_iSize = 0;
	}

	/** @brief Return the element located the index-th element
	*
	*	@param index the element index
	*	@return the item located at the index
	*/
	T& operator[](const int index) const
	{
		return m_pBegin[index];
	}

	/** @brief Return the pointer to the beginning of this array
	*	
	*	@return the pointer to the first element
	*/
	T* data() const
	{
		return m_pBegin;
	}

	/** @brief Return the pointer to the beginning of this array
	*
	*	@return the pointer to the first element
	*/
	iterator begin()
	{
		return m_pBegin;
	}		
	
	/** @brief Return the const pointer to the beginning of this array
	*
	*	@return the const pointer to the first element
	*/
	const_iterator begin() const
	{
		return m_pBegin;
	}	
	
	/** @brief Return the pointer to the end of this array
	*
	*	@return the pointer to the end
	*/
	iterator end()
	{
		return m_pBegin + m_iSize;
	}	
	
	/** @brief Return the const pointer to the end of this array
	*
	*	@return the const pointer to the end
	*/
	const_iterator end() const
	{
		return m_pBegin + m_iSize;
	}

	/** @brief Return reference to the last element
	*
	*	@return the reference to last element
	*/
	reference back()
	{
		return end()[-1];
	}	
	
	/** @brief Return const reference to the last element
	*
	*	@return the const reference to last element
	*/
	const_reference back() const
	{
		return end()[-1];
	}

private:

	Handle					m_hElements;		// the handle array containing the exact data
	T*						m_pBegin;			// the cached pointer to the first element
	std::size_t				m_iSize = 0;		// the current size of array
	std::size_t				m_iCapacity = 0;	// the current capacity
};

template<class T> 
using Vector = MyArray<T>;	

} // namespace DE