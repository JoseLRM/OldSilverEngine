#pragma once

#include "core.h"

namespace sv {

	class ThreadPool;

	class ThreadContext {
		std::atomic<u32> executedTasks = 0u;
		std::atomic<u32> taskCount = 0u;
	public:

		inline void Reset() noexcept
		{
			executedTasks = 0u;
			taskCount = 0u;
		}
		friend ThreadPool;
	};

	using TaskFunction = std::function<void()>;
	
	void task_execute(const TaskFunction& task, ThreadContext* context = nullptr, bool blockingTask = false);
	void task_execute(TaskFunction* task, u32 count, ThreadContext* context, bool blockingTask = false);
	void task_wait(const ThreadContext& context);
	bool task_running(const ThreadContext& context);
	u32 task_thread_count() noexcept;

}