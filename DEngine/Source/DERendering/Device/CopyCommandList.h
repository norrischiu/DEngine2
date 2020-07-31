#pragma once

// Windows
#include <d3d12.h>
// Engine
#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>

namespace DE
{
struct UploadTextureDesc final
{
	uint32_t width;
	uint32_t height;
	uint32_t depth = 1;
	uint32_t pitch = 1;
	uint32_t rowPitch;
	uint32_t subResourceIndex = 0;
};

class RenderDevice;

class CopyCommandList
{
public:

	CopyCommandList(RenderDevice* pRenderDevice);
	CopyCommandList(CopyCommandList&) = default;
	CopyCommandList& operator=(const CopyCommandList&) = default;
	CopyCommandList& operator=(CopyCommandList&&) = default;
	CopyCommandList(CopyCommandList&&) = default;
	~CopyCommandList() = default;

	void Start();

	void UploadTexture(const uint8_t* source, const UploadTextureDesc& desc, DXGI_FORMAT format, Texture& destination);
	void CopyTexture(Texture source, Texture destination);

	inline const CommandList& GetCommandList() const
	{
		return m_CommandList;
	}

private:

	CommandList m_CommandList;
	RenderDevice* m_pRenderDevice;
};

}