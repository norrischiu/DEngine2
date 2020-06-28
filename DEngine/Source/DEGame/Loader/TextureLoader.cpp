#include <DEGame/DEGame.h>
#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DERendering/DataType/GraphicsDataType.h>
#include <DECore/Container/Vector.h>
#include <DECore/Job/JobScheduler.h>

#include "TextureLoader.h"

#include <fstream>

namespace DE
{

namespace detail
{
// only implemented for those in use
uint32_t GetPitch(DXGI_FORMAT format)
{
	if (format >= DXGI_FORMAT_R32G32B32A32_TYPELESS && format <= DXGI_FORMAT_R32G32B32_SINT) {
		return 4;
	}
	else if (format >= DXGI_FORMAT_R16G16B16A16_TYPELESS && format<= DXGI_FORMAT_R16G16B16A16_SINT) {
		return 2;
	}
	else return 1;
}
}


TextureLoader::TextureLoader(RenderDevice* device)
{
	m_pRenderDevice = device;
}

void TextureLoader::Load(CopyCommandList & commandList, Texture & texture, const char * path, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flag)
{
	std::ifstream fin;
	fin.open(path, std::ifstream::in | std::ifstream::binary);
	assert(!fin.fail());

	std::size_t size = 0;
	uint32_t width = 0, height = 0, numComponent = 0, numMip = 0;

	fin.read(reinterpret_cast<char*>(&width), sizeof(width));
	fin.read(reinterpret_cast<char*>(&height), sizeof(height));
	fin.read(reinterpret_cast<char*>(&numComponent), sizeof(numComponent));
	fin.read(reinterpret_cast<char*>(&numMip), sizeof(numMip));
	fin.read(reinterpret_cast<char*>(&size), sizeof(size));
	char* data = new char[size];
	fin.read(data, size);

	assert(numComponent == 4);

	texture.Init(m_pRenderDevice->m_Device, width, height, 1, numMip, format, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST, flag);

	size_t offset = 0;
	uint32_t srcPitch = detail::GetPitch(format);
	for (uint32_t i = 0; i < numMip; ++i)
	{
		UploadTextureDesc desc;
		desc.width = width >> i;
		desc.height = height >> i;
		desc.pitch = srcPitch;
		uint32_t rowPitch = static_cast<uint32_t>(Align(desc.width * srcPitch * numComponent, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
		desc.rowPitch = rowPitch;
		desc.subResourceIndex = i;

		uint8_t* src = reinterpret_cast<uint8_t*>(data) + offset;
		commandList.UploadTexture(src, desc, format, texture);

		offset += desc.width * desc.height * numComponent * srcPitch;
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.ptr.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList.GetCommandList().ptr->ResourceBarrier(1, &barrier);

	delete data;
}

void TextureLoader::Load(Texture& texture, const char* path, DXGI_FORMAT format/* = DXGI_FORMAT_R8G8B8A8_UNORM*/, D3D12_RESOURCE_FLAGS flag /*= D3D12_RESOURCE_FLAG_NONE*/)
{
	CopyCommandList commandList;
	commandList.Init(m_pRenderDevice->m_Device);

	Load(commandList, texture, path, format, flag);

	m_pRenderDevice->Submit(&commandList, 1);
	m_pRenderDevice->Execute();
	m_pRenderDevice->WaitForIdle();
}

void TextureLoader::LoadDefaultTexture()
{
	CopyCommandList commandList;
	commandList.Init(m_pRenderDevice->m_Device);

	Texture& white = Texture::WHITE;
	white.Init(m_pRenderDevice->m_Device, 1, 1, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

	UploadTextureDesc desc;
	desc.width = 1;
	desc.height = 1;
	desc.pitch = 1;
	desc.rowPitch = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	desc.subResourceIndex = 0;

	uint32_t color = UINT_MAX;
	commandList.UploadTexture(reinterpret_cast<uint8_t*>(&color), desc, DXGI_FORMAT_R8G8B8A8_UNORM, white);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = white.ptr.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	commandList.GetCommandList().ptr->ResourceBarrier(1, &barrier);

	m_pRenderDevice->Submit(&commandList, 1);
	m_pRenderDevice->Execute();
	m_pRenderDevice->WaitForIdle();
}
}

