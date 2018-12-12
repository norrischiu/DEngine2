#include <DECore/DECore.h>
#include "JobWorker.h"
#include "JobScheduler.h"
#include "Job.h"

#include <Windows.h>
#include <thread>
#include <atomic>

namespace DE
{

JobWorker::JobWorker(JobScheduler* pScheduler)
	: m_State(State::RUNNING)
	, m_Thread()
	, m_pScheduler(pScheduler)
	, m_JobQueue()
	, m_iTop(0)
	, m_iBottom(0)
{
}

JobWorker::~JobWorker()
{
}

void JobWorker::Start()
{
	m_Thread = std::thread(&JobWorker::RunLoop, this); // a class member function needs to be bound to a class object
}

void JobWorker::End()
{
	m_State = State::END;
	m_Thread.join();
}

Job* JobWorker::Push(Job::Desc& desc)
{
	int32_t b = m_iBottom.load(std::memory_order_relaxed);
	auto& job = m_JobQueue[b & DEFAULT_JOB_QUEUE_SIZE_MASK];
	job.m_pFunction = desc.m_pFunction;
	job.m_pData = desc.m_pData;
	job.m_pParent = desc.m_pParent;
	job.m_iUnfinished = desc.m_iUnfinished;
	m_iBottom.fetch_add(1, std::memory_order_release);

	return &job;
}

Job* JobWorker::Pop()
{
	int32_t b = m_iBottom.load(std::memory_order_acquire) - 1;
	m_iBottom.store(b, std::memory_order_relaxed);
	int32_t t = m_iTop.load(std::memory_order_acquire);

	if (b < t)
	{
		// empty queue
		m_iBottom.store(t, std::memory_order_relaxed);
		return nullptr;
	}

	Job* job = &m_JobQueue[b & DEFAULT_JOB_QUEUE_SIZE_MASK];
	if (b != t)
	{
		return job;
	}

	// last job
	if (!m_iTop.compare_exchange_strong(t, t + 1)) // attempt to increment t
	{
		// last job is being stolen
		job = nullptr;
	}
	m_iBottom.store(t + 1);
	return job;
}

Job* JobWorker::Steal()
{
	// empty queue
	int32_t t = m_iTop.load(std::memory_order_acquire);
	int32_t b = m_iBottom.load(std::memory_order_acquire);

	if (b <= t)
	{
		return nullptr;
	}

	Job* job = &m_JobQueue[t & DEFAULT_JOB_QUEUE_SIZE_MASK];

	if (!m_iTop.compare_exchange_strong(t, t + 1)) // attempt to increment t
	{
		// last job is being stolen or poped
		return nullptr;
	}

	return job;
}

void JobWorker::FinishJob(Job* pJob)
{
	const int32_t unfinishedJobs = --pJob->m_iUnfinished; // atomic
	if (unfinishedJobs == 0)
	{
		if (pJob->m_pParent)
		{
			FinishJob(pJob->m_pParent);
			pJob->m_iUnfinished--; // atomic
		}
	}
}

void JobWorker::RunLoop()
{
	while (m_State == State::RUNNING)
	{
		Job* job = Pop();
		if (job == nullptr)
		{
			// steal
			job = m_pScheduler->Get();
		}
		if (job == nullptr)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(0));
			continue;
		}

		job->m_pFunction(job->m_pData);
		FinishJob(job);
	}

	// scheduler being shut down
}

}


