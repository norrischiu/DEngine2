#pragma once

// Engine
#include <DEGame/DEGame.h>
// Cpp
#include <string>


namespace DE
{

class RenderDevice;
class CopyCommandList;
class Texture;

class TextureLoader
{
public:

	TextureLoader(RenderDevice* device);
	void Load(CopyCommandList& commandList, Texture& texture, const char* path, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);
	void Load(Texture& texture, const char* path, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_NONE);

	static void LoadDefaultTexture(RenderDevice* pRenderDevice);

private:

	RenderDevice* m_pRenderDevice;
};

}