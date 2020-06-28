#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include "CopyCommandList.h"

namespace DE
{

uint32_t CopyCommandList::Init(const GraphicsDevice& device)
{
	m_CommandList.Init(device, D3D12_COMMAND_LIST_TYPE_DIRECT); // TODO: use copy
	m_CommandList.Start();
	m_UploadBuffer.Init(device, 512 * 1024 * 1024, D3D12_HEAP_TYPE_UPLOAD); // 512MB

	m_UploadBuffer.ptr->Map(0, nullptr, &m_pUploadBufferPtr);

	return 0;
}

void CopyCommandList::UploadTexture(const uint8_t* source, const UploadTextureDesc& desc, DXGI_FORMAT format, Texture& destination)
{
	uint32_t numComponent = 4;

	D3D12_SUBRESOURCE_FOOTPRINT footprint = {};
	footprint.Format = format;
	footprint.Width = desc.width;
	footprint.Height = desc.height;
	footprint.Depth = desc.depth;
	footprint.RowPitch = desc.rowPitch;

	const uint8_t* ptr = source;
	const size_t offset = m_Offset;
	size_t size = 0;
	for (uint32_t y = 0; y < desc.height; y++)
	{
		uint8_t* pScan = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(m_pUploadBufferPtr) + offset + size);
		memcpy(pScan, ptr, footprint.RowPitch);
		ptr += desc.width * numComponent * desc.pitch;
		size += footprint.RowPitch;
	}
	m_Offset += Align(size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);

	D3D12_TEXTURE_COPY_LOCATION dst;
	dst.pResource = destination.ptr.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = desc.subResourceIndex;
	
	D3D12_TEXTURE_COPY_LOCATION src;
	src.pResource = m_UploadBuffer.ptr.Get();
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Offset = offset;
	src.PlacedFootprint.Footprint = footprint;

	m_CommandList.ptr->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
}

void CopyCommandList::CopyTexture(Texture source, Texture destination)
{
	for (uint32_t i = 0; i < source.m_iNumSubresources; ++i)
	{
		D3D12_TEXTURE_COPY_LOCATION src;
		src.pResource = source.ptr.Get();
		src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		src.SubresourceIndex = i;

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.pResource = destination.ptr.Get();
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = i;

		m_CommandList.ptr->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}
}

};