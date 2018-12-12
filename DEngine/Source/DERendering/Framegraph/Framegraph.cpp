#include <DERendering/DERendering.h>
#include <DERendering/Framegraph/Framegraph.h>
#include <DERendering/Framegraph/FramegraphPassBase.h>
#include <DERendering/D3D12/D3D12Renderer.h>

#include <DECore/Container/Vector.h>

namespace DE
{

Framegraph::Framegraph(D3D12Renderer& renderer)
	: m_pRenderer(&renderer)
{
}

void Framegraph::Compile()
{
	for (auto& pass : m_vPasses)
	{
		pass->Setup();
	}
}

void ExecutePass(void* data)
{
	reinterpret_cast<FramegraphPassBase*>(data)->Execute();
}

void Framegraph::Execute()
{
	Vector<Job::Desc> jobDescs(m_vPasses.size());
	for (auto& pass : m_vPasses)
	{
		Job::Desc desc(&ExecutePass, pass.get(), nullptr);
		jobDescs.push_back(std::move(desc));
	}
	auto counter = JobScheduler::Instance()->Run(jobDescs);
	JobScheduler::Instance()->WaitOnMainThread(counter);
}

}

