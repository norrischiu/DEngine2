#include <DECore/DECore.h>
#include "JobScheduler.h"
#include "JobWorker.h"
#include "Job.h"

#include <assert.h>
#include <thread>
#include <random>

namespace DE
{

JobScheduler* JobScheduler::m_pInstance = nullptr;

void EmptyJob(void*) {}

void JobScheduler::StartUp(uint8_t numThreads)
{
	m_iNumWorker = numThreads;
	m_Workers.resize(m_iNumWorker);
	for (uint8_t cnt = 0; cnt < m_iNumWorker; ++cnt)
	{		
		m_Workers[cnt] = new JobWorker(this);
	}

	for (uint8_t cnt = 1; cnt < m_iNumWorker; ++cnt) // index 0 is main thread
	{
		m_Workers[cnt]->Start();
	}
}

void JobScheduler::ShutDown()
{
	uint8_t numWorkers = static_cast<uint8_t>(m_Workers.size());
	for (uint8_t cnt = 1; cnt < numWorkers; ++cnt)
	{
		m_Workers[cnt]->End();
	}
}

Job* JobScheduler::Run(Vector<Job::Desc>& jobDescs)
{
	Job::Desc parent(&EmptyJob, nullptr, nullptr);
	parent.m_iUnfinished = static_cast<uint32_t>(jobDescs.size()) + 1;
	Job* counter = m_Workers[0]->Push(parent);

	for (auto& desc : jobDescs)
	{
		desc.m_iUnfinished++;
		desc.m_pParent = counter;
		m_Workers[0]->Push(desc); // push to main thread only for now
	}

	return counter;
}

Job* JobScheduler::Get()
{
	uint8_t index = rand() % (m_iNumWorker - 1);
	return m_Workers[index]->Steal();
}

void JobScheduler::WaitOnMainThread(Job* job)
{
	while (job->m_iUnfinished > 0)
	{
		Job* job = Get();
		if (job != nullptr)
		{
			job->m_pFunction(job->m_pData);
			m_Workers[0]->FinishJob(job);
		}
	}
}

}
