#include <DERendering/DERendering.h>
#include "DrawCommandList.h"

namespace DE
{

uint32_t DrawCommandList::Init(const GraphicsDevice& device)
{
	m_CommandList.Init(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	return 0;
}

};