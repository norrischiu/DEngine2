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

	/** @brief	Construct an handle array with give capacity. Enough capacity should be allocated 
	*			to fit the possible size of the array in order avoid reallocation
	*
	*	@param capacity
	*/
	MyArray(size_t capacity = 0)
	{
		m_iSize = 0;
		m_iCapacity = capacity;
		if (m_iCapacity > 0)
		{
			m_hElements.Set(sizeof(T) * capacity);
			memset(m_hElements.Raw(), NULL, sizeof(T) * capacity);
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
		return *this;
	}

	/** @brief Free the memory used by this array's items */
	~MyArray()
	{
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
	void push_back(T& item)
	{
		if (m_iCapacity == 0)
		{
			reserve(1);
		}
		if (m_iSize >= m_iCapacity)
		{
			reserve(m_iCapacity * 2);
		}
		((T*)m_hElements.Raw())[m_iSize] = item;
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
		((T*)m_hElements.Raw())[m_iSize] = std::move(item);
		m_iSize++;
	}

	/** @brief Resize the array
	*
	*	@param size the new size
	*/
	void resize(size_t size)
	{
		reserve(size);
		m_iSize = size;
	}	
	
	/** @brief Reserve the array's capavity
	*
	*	@param size the new capacity
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
		}
	}

	/** @brief Clear the items in this array, leaving the capacity and allocated memory unchanged */
	void clear()
	{
		if (m_iCapacity > 0)
		{
			memset(m_hElements.Raw(), NULL, sizeof(T) * m_iSize);
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
		return ((T*)m_hElements.Raw())[index];
	}

	/** @brief Return the pointer to the beginning of this array
	*	
	*	@return the pointer to the first element
	*/
	T* data() const
	{
		return ((T*)m_hElements.Raw());
	}

	/** @brief Return the pointer to the beginning of this array
	*
	*	@return the pointer to the first element
	*/
	iterator begin()
	{
		return data();
	}		
	
	/** @brief Return the const pointer to the beginning of this array
	*
	*	@return the const pointer to the first element
	*/
	const_iterator begin() const
	{
		return data();
	}	
	
	/** @brief Return the pointer to the end of this array
	*
	*	@return the pointer to the end
	*/
	iterator end()
	{
		return data() + size();
	}	
	
	/** @brief Return the const pointer to the end of this array
	*
	*	@return the const pointer to the end
	*/
	const_iterator end() const
	{
		return data() + size();
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

	Handle					m_hElements;	// the handle array containing the exact data
	std::size_t				m_iSize;		// the current size of array
	std::size_t				m_iCapacity;	// the current capacity
};

template<class T> 
using Vector = MyArray<T>;	

} // namespace DE