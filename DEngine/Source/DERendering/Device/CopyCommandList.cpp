#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsNativeType.h>
#include <DERendering/DataType/GraphicsResourceType.h>
#include <DERendering/Device/RenderDevice.h>
#include "CopyCommandList.h"

namespace DE
{
CopyCommandList::CopyCommandList(RenderDevice * pRenderDevice)
	: m_pRenderDevice(pRenderDevice)
{
	m_CommandList.Init(m_pRenderDevice->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT); // TODO: use copy
}

void CopyCommandList::Start()
{
	m_CommandList.Start();
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

	const uint32_t alignedSize = Align(desc.rowPitch * desc.height, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
	Buffer uploadBuffer = m_pRenderDevice->SuballocateUploadBuffer(alignedSize);

	uint8_t* mappedPtr = static_cast<uint8_t*>(uploadBuffer.mappedPtr);
	const uint8_t* ptr = source;
	const size_t offset = uploadBuffer.offset;
	size_t size = 0;
	for (uint32_t y = 0; y < desc.height; y++)
	{
		uint8_t* pScan = reinterpret_cast<uint8_t*>(mappedPtr + offset + size);
		memcpy(pScan, ptr, footprint.RowPitch);
		ptr += desc.width * numComponent * desc.pitch;
		size += footprint.RowPitch;
	}

	D3D12_TEXTURE_COPY_LOCATION dst;
	dst.pResource = destination.ptr.Get();
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = desc.subResourceIndex;
	
	D3D12_TEXTURE_COPY_LOCATION src;
	src.pResource = uploadBuffer.ptr.Get();
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