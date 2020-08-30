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
		Wireframe,
		NoNormalMap,
		Unlit,
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
		for (auto& meshes : m_meshes)
		{
			meshes.clear();
		}
	}

	const Vector<uint32_t>& Get(Flag flag) const
	{
		return m_meshes[(uint32_t)flag];
	}

	uint32_t GetTotalNum() const
	{
		uint32_t total = 0;
		for (const auto& meshes : m_meshes)
		{
			total += static_cast<uint32_t>(meshes.size());
		}
		return total;
	}

private:
	Vector<uint32_t> m_meshes[(uint32_t)Flag::Count];
};

} // namespace DE