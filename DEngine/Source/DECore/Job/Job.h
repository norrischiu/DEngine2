#pragma once

#include <DECore/DECore.h>
#include <atomic>
#include <new>

namespace DE
{

/** @brief Describes a task, will be accessed through different threads */
struct DllExport alignas(std::hardware_destructive_interference_size) Job
{
	/** @brief Description to create a actual job */
	struct Desc
	{
		Desc(void(*pFunc)(void*), void* pData, Job* pParent)
			: m_pFunction(pFunc)
			, m_pData(pData)
			, m_pParent(pParent)
			, m_iUnfinished(0)
		{}

		void(*m_pFunction)(void*) = nullptr;
		void* m_pData = nullptr;
		Job* m_pParent = nullptr;
		uint32_t m_iUnfinished;
	};

	Job() = default;
	Job(const Job&) = delete;
	Job& operator=(const Job&) = delete;
	~Job() = default;

	Job(void(*pFunc)(void*), void* pData, Job* pParent)
		: m_pFunction(pFunc)
		, m_pData(pData)
		, m_pParent(pParent)
	{}

	void (*m_pFunction)(void*) = nullptr;		//< the job itself with data as parameter
	void* m_pData = nullptr;					//< pointer to custom data for the job
	Job* m_pParent = nullptr;					//< parent job to be ran first
	std::atomic_int32_t m_iUnfinished = {0};	//< atomic int on number of unfinished child jobs
};

}
