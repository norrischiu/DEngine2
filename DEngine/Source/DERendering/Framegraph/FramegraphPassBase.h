#pragma once

namespace DE
{

class CommandList;
class FramegraphBuilder;
class FramegraphResource;

class FramegraphPassBase
{
public:
	FramegraphPassBase() = default;
	virtual ~FramegraphPassBase() = default;

	virtual void Setup(FramegraphBuilder& builder) = 0;
	virtual void Execute() = 0;

protected:	
	friend FramegraphBuilder;

	Vector<FramegraphResource*> m_readResources;
	Vector<FramegraphResource*> m_writeResources;
	Vector<FramegraphResource*> m_drawResources;

};

}