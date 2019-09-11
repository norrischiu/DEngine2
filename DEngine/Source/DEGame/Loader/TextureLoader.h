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
	void Load(Texture& texture, const char* path);

private:

	RenderDevice* m_pRenderDevice;
};

}