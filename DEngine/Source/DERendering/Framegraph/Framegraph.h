#pragma once

#include <DERendering/DERendering.h>
#include <DERendering/Framegraph/FramegraphPass.h>

#include <string>

namespace DE 
{

class D3D12Renderer;
class CommandList;

class Framegraph
{
public:

	Framegraph(D3D12Renderer& D3D12Renderer);
	Framegraph(const Framegraph& other) = delete;
	Framegraph(Framegraph&& other) = delete;
	Framegraph& operator=(const Framegraph& other) = delete;
	Framegraph& operator=(Framegraph&& other) = delete;
	~Framegraph() = default;

	template<typename T_DATA>
	FramegraphPass<T_DATA>* AddPass(std::string name, std::function<void(T_DATA&)> setup, std::function<void(T_DATA&, CommandList&)> execute)
	{
		CommandList* commandList = new CommandList();
		commandList->Init(m_pRenderer->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		m_CommandLists.push_back(commandList);

		using PassType = FramegraphPass<T_DATA>;
		m_vPasses.push_back(std::move(std::make_unique<PassType>(name, setup, execute, commandList)));
		auto pass = static_cast<PassType*>(m_vPasses.back().get());
		return pass;
	}

	void Compile();
	void Execute();

	Vector<CommandList*> m_CommandLists;

private:
	D3D12Renderer* m_pRenderer;
	Vector<std::unique_ptr<FramegraphPassBase>> m_vPasses;
};

}