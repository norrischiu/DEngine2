#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/Framegraph/FramegraphPass.h>

#include <string>

namespace DE 
{

class RenderDevice;
class CommandList;

class Framegraph
{
public:

	Framegraph(RenderDevice& RenderDevice);
	Framegraph(const Framegraph& other) = delete;
	Framegraph(Framegraph&& other) = delete;
	Framegraph& operator=(const Framegraph& other) = delete;
	Framegraph& operator=(Framegraph&& other) = delete;
	~Framegraph() = default;

	template<typename T_DATA>
	FramegraphPass<T_DATA>* AddPass(std::string name, std::function<void(FramegraphBuilder&, T_DATA&)> setup, std::function<void(T_DATA&, CommandList&)> execute)
	{
		CommandList* commandList = new CommandList();
		commandList->Init(m_pRenderer->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_CommandLists.push_back(commandList);

		using PassType = FramegraphPass<T_DATA>;
		m_vPasses.push_back(new PassType(name, setup, execute, commandList));
		auto pass = static_cast<PassType*>(m_vPasses.back());
		return pass;
	}

	void Compile();
	void Execute();

	Vector<CommandList*> m_CommandLists;

private:
	friend FramegraphBuilder;

	RenderDevice* m_pRenderer;
	Vector<FramegraphPassBase*> m_vPasses;
	Vector<FramegraphResource*> m_resources;
};

}