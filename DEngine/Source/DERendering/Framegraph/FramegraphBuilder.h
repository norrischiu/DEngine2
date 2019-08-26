#pragma once

#include <DERendering/Framegraph/FramegraphResource.h>

#include <cstdint>
#include <dxgi.h>

namespace DE
{

class Texture;
class Framegraph;
class FramegraphPassBase;


struct FramegraphTextureDesc
{
	uint32_t width;
	uint32_t height;
	DXGI_FORMAT format;
};

struct FramegraphBufferDesc
{
	size_t size;
};

class FramegraphBuilder
{
public:

	FramegraphBuilder(Framegraph* pFramegraph, FramegraphPassBase* pass)
		: m_pFramegraph(pFramegraph)
		, m_pPass(pass)
	{};

	// use created back buffer
	FramegraphResource CreateTexture(Texture& texture)
	{
	}

	FramegraphResource CreateTexture(const FramegraphTextureDesc& desc)
	{
	}

	FramegraphResource CreateBuffer(const FramegraphBufferDesc& desc)
	{
	}

	FramegraphResource* Read(FramegraphResource* prev);
	FramegraphResource* Write(FramegraphResource* prev);
	FramegraphResource* Draw(FramegraphResource* prev);

private:
	Framegraph* m_pFramegraph;
	FramegraphPassBase* m_pPass;
};

} // namespace DE