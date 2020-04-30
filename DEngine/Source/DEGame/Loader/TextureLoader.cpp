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
uint32_t GetStride(DXGI_FORMAT format)
{
	if (format >= DXGI_FORMAT_R16G16B16A16_TYPELESS && format<= DXGI_FORMAT_R16G16B16A16_SINT) {
		return 2;
	}
	else return 1;
}
}


void TextureLoader::Init(RenderDevice* device)
{
	m_pRenderDevice = device;
}

struct LoadToMaterialsData
{
	char path[256];
	char materialName[64];
	Material* pMaterial;
	RenderDevice* pDevice;
	CopyCommandList* pCopyCommandList;
};

void TextureLoader::Load(Texture& texture, const char* path, DXGI_FORMAT format/* = DXGI_FORMAT_R8G8B8A8_UNORM*/)
{
	CopyCommandList commandList;
	commandList.Init(m_pRenderDevice->m_Device);

	std::ifstream fin;
	fin.open(path, std::ifstream::in | std::ifstream::binary);
	assert(!fin.fail());

	// load the file content and upload to ID3D12Resource
	{
		std::size_t size = 0;
		uint32_t width = 0, height = 0, numComponent = 0;

		fin.read(reinterpret_cast<char*>(&width), sizeof(width));
		fin.read(reinterpret_cast<char*>(&height), sizeof(height));
		fin.read(reinterpret_cast<char*>(&numComponent), sizeof(numComponent));
		fin.read(reinterpret_cast<char*>(&size), sizeof(size));
		char* data = new char[size];
		fin.read(data, size);

		assert(numComponent == 4);

		texture.Init(m_pRenderDevice->m_Device, width, height, 1, 1, format, D3D12_HEAP_TYPE_DEFAULT);
		uint32_t rowPitch = static_cast<uint32_t>(Align(width * detail::GetStride(format) * numComponent, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
		commandList.UploadTexture(reinterpret_cast<uint8_t*>(data), width, height, rowPitch, 1, format, texture);

		delete data;
	}
	fin.close();

	m_pRenderDevice->Submit(&commandList, 1);
	m_pRenderDevice->Execute();
	m_pRenderDevice->WaitForIdle();
}
}

