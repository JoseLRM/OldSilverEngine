#pragma once

#include "core.h"

namespace _sv {

	class ThreadPool;

	bool task_initialize(ui32 minThreadCount);
	bool task_close();

}

namespace sv {

	class ThreadContext {
		std::atomic<ui32> executedTasks = 0u;
		std::atomic<ui32> taskCount = 0u;
	public:

		inline void Reset() noexcept
		{
			executedTasks = 0u;
			taskCount = 0u;
		}
		friend _sv::ThreadPool;
	};

	using TaskFunction = std::function<void()>;
	
	void task_execute(const TaskFunction& task, ThreadContext* context = nullptr, bool blockingTask = false);
	void task_execute(TaskFunction* task, ui32 count, ThreadContext* context, bool blockingTask = false);
	void task_wait(const ThreadContext& context);
	bool task_running(const ThreadContext& context);
	ui32 task_thread_count() noexcept;

}