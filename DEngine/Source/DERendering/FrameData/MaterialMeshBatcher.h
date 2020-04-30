#pragma once

#include <DERendering/DERendering.h>
#include <DECore/Container/Vector.h>
#include <DERendering/DataType/GraphicsDataType.h>

namespace DE
{

class MaterialMeshBatcher
{

public:
	enum class Flag : uint8_t
	{
		None = 0,
		AlbedoOnly,
		Textured,
		Count,
	};

	MaterialMeshBatcher() = default;

	void Add(Flag flag, Mesh mesh)
	{
		m_meshes[(uint32_t)flag].push_back(mesh.Index());
	}

	void Reset()
	{
		for (uint32_t i = 0; i < (uint32_t)Flag::Count; i++)
		{
			m_meshes[i].clear();
		}
	}

	const Vector<uint32_t>& Get(Flag flag) const
	{
		return m_meshes[(uint32_t)flag];
	}

private:
	Vector<uint32_t> m_meshes[(uint32_t)Flag::Count];
};

} // namespace DE