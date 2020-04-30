#pragma once

#include <DEGame/DEGame.h>
#include <DECore/Container/Vector.h>
#include <DERendering/DataType/LightType.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <functional>

namespace DE 
{

class Scene
{
public:
	Scene()
	{
		m_objects.resize(64);
	}
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	template <typename T>
	void Add(const T& obj)
	{
		m_objects[T::ObjectId()].push_back(obj.Index());
	}
	template <typename T>
	void ForEach(std::function<void(T&)> func) 
	{
		for (auto& obj : m_objects[T::ObjectId()])
		{
			func(T::Get(obj));
		}
	}

private:
	Vector<Vector<uint32_t>> m_objects;
};

}