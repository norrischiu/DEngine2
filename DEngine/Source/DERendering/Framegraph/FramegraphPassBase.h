#pragma once

namespace DE
{

class CommandList;

class FramegraphPassBase
{
public:
	FramegraphPassBase() = default;
	virtual ~FramegraphPassBase() = default;

	virtual void Setup() = 0;
	virtual void Execute() = 0;
};

}