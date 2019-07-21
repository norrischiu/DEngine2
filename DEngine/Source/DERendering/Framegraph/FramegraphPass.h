#pragma once

#include <DERendering/Framegraph/FramegraphPassBase.h>

#include <functional>
#include <string>

namespace DE
{

class CommandList;
class FramegraphResource;
class FramegraphBuilder;

template <typename T_DATA>
class FramegraphPass : public FramegraphPassBase
{
public:

	FramegraphPass(std::string name, std::function<void(T_DATA&)> setup, std::function<void(T_DATA&, CommandList&)> execute, CommandList* pCommandList)
		: m_sName(std::move(name))
		, m_Data()
		, m_Setup(setup)
		, m_Execute(execute)
		, m_pCommandList(pCommandList)
	{};
	FramegraphPass(const FramegraphPass& other) = delete;
	FramegraphPass(FramegraphPass&& other) = default;
	FramegraphPass& operator=(const FramegraphPass& other) = delete;
	FramegraphPass& operator=(FramegraphPass&& other) = default;
	~FramegraphPass() = default;

	void Setup(FramegraphBuilder& builder) override
	{
		m_Setup(builder, m_Data);
	}

	void Execute() override
	{
		m_pCommandList->Start();
		m_Execute(m_Data, *m_pCommandList);
		m_pCommandList->End();
	}

protected:
	std::string m_sName;
	CommandList* m_pCommandList;
	T_DATA m_Data;
	std::function<void(T_DATA&)> m_Setup;
	std::function<void(T_DATA&, CommandList&)> m_Execute;

	//std::vector<D3D12_RESOURCE_BARRIER> m_resourceBarriers;
};

}