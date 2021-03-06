#pragma once

// Engine
#include <DECore/DECore.h>
#include <DECore/Container/Vector.h>
#include <DECore/Job/JobWorker.h>

namespace DE
{

class DllExport JobScheduler
{
public:
	JobScheduler() = default;
	~JobScheduler() = default;

	void StartUp(uint8_t numThreads);
	void ShutDown();

	/** @brief Put a list of jobs onto the scheduler and run it
	*
	*	@return a job as a counter to call WaitOnMainThread() on
	*/
	Job* Run(Vector<Job::Desc>& jobDescs);

	/** @brief Get a job from the scheduler by stealing from other threads
	*
	*	@return a stolen job
	*/
	Job* Get();

	/** @brief	Wait for a job or counter to be finished by busy spinning,
	*			will work other job at the same time
	*
	*	@param a job to be waited
	*/
	void WaitOnMainThread(Job* job);

	static JobScheduler* Instance()
	{
		if (!m_pInstance)
		{
			m_pInstance = new JobScheduler();
		}
		return m_pInstance;
	}

private:
	static JobScheduler*				m_pInstance;

	uint32_t							m_iNumWorker;
	Vector<std::unique_ptr<JobWorker>>	m_Workers;
};

}


