#pragma once

#include <DERendering/DERendering.h>
#include <DECore/Container/Vector.h>

namespace DE
{

class MaterialMeshBatcher
{

public:
	MaterialMeshBatcher() = default;

	void Add(uint32_t mesh)
	{
		m_meshes.push_back(mesh);
	}

	void Reset()
	{
		m_meshes.clear();
	}

	const Vector<uint32_t>& Get() const
	{
		return m_meshes;
	}

private:
	Vector<uint32_t> m_meshes;
};

} // namespace DE