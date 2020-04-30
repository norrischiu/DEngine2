#pragma once

// Windows
#include <d3d12.h>
// Engine
#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{

class CopyCommandList
{
public:

	CopyCommandList() = default;
	~CopyCommandList() = default;

	uint32_t Init(const GraphicsDevice& device);

	void UploadTexture(const uint8_t* source, uint32_t width, uint32_t height, uint32_t rowPitch, uint32_t depth, DXGI_FORMAT format, Texture& destination);
	void CopyTexture(Texture source, Texture destination);

	inline const CommandList& GetCommandList() const
	{
		return m_CommandList;
	}

private:

	size_t SuballocateFromBuffer(size_t size, size_t alignment);

	Buffer m_UploadBuffer;
	void* m_pUploadBufferPtr;
	size_t m_Offset;

	CommandList m_CommandList;
};

}