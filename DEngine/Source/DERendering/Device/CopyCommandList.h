#pragma once

#include <DERendering/DERendering.h>
namespace DE
{

class CopyCommandList
{
public:

	CopyCommandList() = default;
	~CopyCommandList() = default;

	uint32_t Init(const GraphicsDevice& device);

	void UploadTexture(uint8_t* source, uint32_t width, uint32_t height, uint32_t rowPitch, uint32_t depth, DXGI_FORMAT format, Texture& destination);
	void CopyTexture(Texture source, Texture destination);

	inline const CommandList& GetCommandList() const
	{
		return m_CommandList;
	}

private:

	std::size_t SuballocateFromBuffer(std::size_t size, std::size_t alignment);

	Buffer m_UploadBuffer;
	void* m_pUploadBufferPtr;
	std::size_t m_Offset;

	CommandList m_CommandList;
};

}