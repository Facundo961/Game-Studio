#pragma once

#include "ByteEngine/Core.h"

#include <GTSL/Array.hpp>
#include <GTSL/Atomic.hpp>
#include <GTSL/Delegate.hpp>
#include <GTSL/BlockingQueue.h>
#include <thread>
#include <GTSL/Thread.h>
#include <GTSL/Tuple.h>

//https://github.com/mvorbrodt/blog

class ThreadPool
{
public:
	explicit ThreadPool() : queues(threadCount)
	{
		//lambda
		auto workers_loop = [](ThreadPool* pool, const uint8 i)
		{
			while (true)
			{
				GTSL::Delegate<void(void*)> work;
				for (auto n = 0; n < threadCount * K; ++n)
				{
					if (pool->queues[(i + n) % threadCount].TryPop(work)) { break; }
				}

				if (!work && !pool->queues[i].Pop(work)) { break; }
				work(pool->taskDatas[i]);
			}
		};

		for (uint8 i = 0; i < threadCount; ++i)
		{
			//Constructing threads with function and I parameter
			//threads.EmplaceBack(workers_loop, i);
			threads.EmplaceBack(GTSL::Delegate<void(ThreadPool*, uint8)>::Create(workers_loop), this, i);
			threads[i].SetPriority(GTSL::Thread::Priority::HIGH);
		}
	}

	~ThreadPool()
	{
		for (auto& queue : queues) { queue.Done(); }
		for (auto& thread : threads) { thread.Join(); }
	}

	template<typename F, typename... ARGS>
	void EnqueueTask(const GTSL::Delegate<F>& delegate, GTSL::Semaphore* semaphore, ARGS&&... args)
	{
		auto task = new TaskInfo<F, ARGS...>(delegate, semaphore, GTSL::MakeForwardReference<ARGS>(args)...);
		
		auto work = [](void* voidTask)
		{
			TaskInfo<F, ARGS...>* task = static_cast<TaskInfo<F, ARGS...>*>(voidTask);
			GTSL::Thread::Call<F, ARGS...>(task->Delegate, task->Arguments);
			task->Semaphore->Post();

			delete task;
		};
		
		const auto current_index = ++index;

		for (auto n = 0; n < threadCount * K; ++n)
		{
			//Try to Push work into queues, if success return else when Done looping place into some queue.

			if (queues[(current_index + n) % threadCount].TryPush(GTSL::Delegate<void(void*)>::Create(work))) { return; }
		}

		queues[current_index % threadCount].Push(GTSL::Delegate<void(void*)>::Create(work));
	}

private:
	inline const static uint8 threadCount{ static_cast<uint8>(std::thread::hardware_concurrency() - 1) };
	GTSL::Atomic<uint32> index{ 0 };
	
	GTSL::Array<GTSL::BlockingQueue<GTSL::Delegate<void(void*)>>, 64> queues;
	GTSL::Array<GTSL::Thread, 64> threads;
	GTSL::Array<void*, 128> taskDatas;

	template<typename T, typename... ARGS>
	struct TaskInfo
	{
		TaskInfo(const GTSL::Delegate<T>& delegate, GTSL::Semaphore* semaphore, ARGS&&... args) : Delegate(delegate), Semaphore(semaphore), Arguments(GTSL::MakeForwardReference<ARGS>(args)...)
		{
		}
		
		GTSL::Delegate<T> Delegate;
		GTSL::Tuple<ARGS...> Arguments;
		GTSL::Semaphore* Semaphore = nullptr;
	};
	/**
	 * \brief Number of times to loop around the queues to find one that is free.
	 */
	inline static constexpr uint8 K{ 2 };
};