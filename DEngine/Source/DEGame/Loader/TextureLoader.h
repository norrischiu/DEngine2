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

	void Init(RenderDevice* device);
	void Load(Texture& texture, const char* path, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

private:

	RenderDevice* m_pRenderDevice;
};

}