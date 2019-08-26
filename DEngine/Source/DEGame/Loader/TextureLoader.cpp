#include <DEGame/DEGame.h>
#include <DERendering/DERendering.h>
#include <DERendering/Device/RenderDevice.h>
#include <DERendering/Device/CopyCommandList.h>
#include <DECore/Container/Vector.h>
#include <DECore/Job/JobScheduler.h>

#include "TextureLoader.h"

#include <fstream>

namespace DE
{

void TextureLoader::Init(RenderDevice& device)
{
	m_pRenderDevice = &device;
}

struct LoadToMaterialsData
{
	char path[256];
	char materialName[64];
	Material* pMaterial;
	RenderDevice* pDevice;
	CopyCommandList* pCopyCommandList;
};

void TextureLoader::Load(Texture& texture, const char* path)
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

		DXGI_FORMAT format = {};
		switch (numComponent)
		{
		case 4:
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			break;
		default:
			assert(false);
		}
		texture.Init(m_pRenderDevice->m_Device, width, height, 1, 1, format, D3D12_HEAP_TYPE_DEFAULT);
		commandList.UploadTexture(reinterpret_cast<uint8_t*>(data), width, height, 1, format, texture);

		delete data;
	}
	fin.close();

	m_pRenderDevice->Submit(&commandList, 1);
	m_pRenderDevice->Execute();
	m_pRenderDevice->WaitForIdle();
}

}

