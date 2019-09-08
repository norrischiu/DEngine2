#pragma once

#include <DEGame/DEGame.h>
#include <DECore/Container/Vector.h>
#include <functional>

namespace DE 
{

class Scene
{
public:
	Scene() = default;
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	void Set(Vector<uint32_t>& meshes)
	{
		m_meshes = std::move(meshes);
	}

	void ForEachMesh(std::function<void(uint32_t)> func)
	{
		for (auto& mesh : m_meshes)
		{
			func(mesh);
		}
	}

private:
	Vector<uint32_t> m_meshes;
};

}