#pragma once

#include <DERendering/DERendering.h>
namespace DE
{

class DrawCommandList
{
public:

	DrawCommandList() = default;
	~DrawCommandList() = default;

	uint32_t Init(const GraphicsDevice& device);

	void Draw();
	void Dispatch();

private:
	CommandList m_CommandList;
};

}