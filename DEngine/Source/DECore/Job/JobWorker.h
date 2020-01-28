#pragma once

// Cpp
#include <stdint.h>
#include <thread>
#include <array>
#include <atomic>
// Engine
#include <DECore/Job/Job.h>

namespace DE
{ 

class JobScheduler;

constexpr uint32_t DEFAULT_JOB_QUEUE_SIZE = 4096;
constexpr uint32_t DEFAULT_JOB_QUEUE_SIZE_MASK = DEFAULT_JOB_QUEUE_SIZE - 1;

class DllExport JobWorker
{

public:
	JobWorker(JobScheduler* pScheduler);
	~JobWorker();

	/** @brief Kick off the underlying thread
	*/
	void Start();

	/** @brief Wait for the underlying thread completes processing
	*/
	void End();

	/** @brief Create a job according to the job desc and put it to the queue
	*
	*	@param desc the job description
	*	@return pointer to the created job
	*/
	Job* Push(Job::Desc& desc);

	/** @brief Pop a job from the queue
	*
	*	@return pointer to a job
	*/
	Job* Pop();

	/** @brief Steal a job from the queue
	*
	*	@return pointer to a job
	*/
	Job* Steal();

	/** @brief Finish processing a job and set relavant state
	*
	*	@param pointer to a job
	*/
	void FinishJob(Job* pJob);

private:
	enum class State : uint8_t
	{
		RUNNING, END
	};

	/** @brief	the main thread loop, will keep on grabbing job from own
	*			queue, and steal from other queue if own is empty
	*/
	void RunLoop();

	JobScheduler*								m_pScheduler;
	State										m_State;
	std::thread									m_Thread;
	std::array<Job, DEFAULT_JOB_QUEUE_SIZE>		m_JobQueue;
	std::atomic_int32_t							m_iTop;
	std::atomic_int32_t							m_iBottom;

};

}
