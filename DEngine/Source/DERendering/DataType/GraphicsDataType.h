#pragma once

namespace DE
{

/**	@brief Contains vertex and index buffer of a mesh*/
struct Mesh
{
public:
	void*						m_pVertexBuffer;
	void*						m_pIndexBuffer;
};

}