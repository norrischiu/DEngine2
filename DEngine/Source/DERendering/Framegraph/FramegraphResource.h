#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/DataType/GraphicsViewType.h>

namespace DE
{

class FramegraphResource
{
public:
	enum State
	{
		READ,
		WRITE,
		DRAW,
	};

	FramegraphResource(State state, FramegraphResource* parent = nullptr)
		: m_state(state)
		, m_pParent(parent)
	{};
	~FramegraphResource() = default;

private:
	FramegraphResource* m_pParent;
	State m_state;

	union
	{
		RenderTargetView rtv;
		DepthStencilView dsv;
	};
	ShaderResourceView srv;
	UnorderedAccessView uav;
};
}