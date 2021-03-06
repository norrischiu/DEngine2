#pragma once

// Cpp
#include <atomic>
#include <memory>
// Engine
#include <DECore/Job/Job.h>
#include <DECore/Job/JobScheduler.h>

namespace DE
{
template <class T>
class JobFuture
{
public:
	JobFuture(Job* job, std::unique_ptr<T> output)
		: m_job(job)
		, m_pOutput(std::move(output))
		, m_iCount(0)
	{
	}
	JobFuture(const JobFuture&) = delete;
	JobFuture(JobFuture&&) = default;
	JobFuture& operator=(const JobFuture&) = delete;
	JobFuture& operator=(JobFuture&&) = default;
	~JobFuture() = default;

	T WaitGet()
	{
		assert(m_iCount == 0); // future can only be WaitGet once

		m_iCount++;
		JobScheduler::Instance()->WaitOnMainThread(m_job);
		return std::move(*m_pOutput);
	}

private:
	Job* m_job;
	std::unique_ptr<T> m_pOutput;
	std::atomic_uint32_t m_iCount;
};

}