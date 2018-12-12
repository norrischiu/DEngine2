#pragma once

// Window
#include <d3d12.h>
// C++
#include <string>

namespace DE 
{

class Texture
{
public:
	Texture() = default;
	virtual ~Texture()
	{
		ptr->Release();
	}

	void Init(ID3D12Resource* resource)
	{
		ptr = resource;
	}

	ID3D12Resource* ptr;
	D3D12_RESOURCE_DESC m_Desc;
};

class Buffer
{
public:

	virtual ~Buffer()
	{
		ptr->Release();
	}
	ID3D12Resource* ptr;
	D3D12_RESOURCE_DESC m_Desc;
};

struct VertexBuffer final : public Buffer
{
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
};

struct IndexBuffer final : public Buffer
{
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};

struct RenderTarget final : public Texture
{
	D3D12_CPU_DESCRIPTOR_HANDLE m_pDescriptor;
};

}
