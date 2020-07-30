#include "core.h"

#include "task_system.h"

namespace _sv {

	class ThreadPool {
		std::vector<std::thread> m_Threads;

		std::condition_variable m_ConditionVar;
		std::mutex m_SleepMutex;
		std::mutex m_ExecuteMutex;

		std::queue<std::pair<sv::TaskFunction, sv::ThreadContext*>> m_Tasks;

		bool m_Running = true;

	public:
		ThreadPool() {}

		inline void Reserve(ui32 cant) noexcept
		{
			m_Threads.reserve(cant);
			ui32 lastID = ui32(m_Threads.size());
			for (ui32 i = 0; i < cant; ++i) {
				ui32 ID = i + lastID;
				m_Threads.emplace_back([this, ID]() {
					sv::TaskFunction task;
					sv::ThreadContext* pContext;

					while (m_Running) {
						// sleep - critical section
						{
							std::unique_lock<std::mutex> sleepLock(m_SleepMutex);
							m_ConditionVar.wait(sleepLock, [&]() { return !m_Running || !m_Tasks.empty(); });

							std::lock_guard<std::mutex> executeLock(m_ExecuteMutex);

							if (m_Tasks.empty()) continue;

							task = std::move(m_Tasks.front()).first;
							pContext = m_Tasks.front().second;
							m_Tasks.pop();
						}

						// execution
						task();
						if (pContext) pContext->executedTasks.fetch_add(1);
					}
				});
			}
		}

		inline void Execute(const sv::TaskFunction& task, sv::ThreadContext* context = nullptr) noexcept
		{
			std::lock_guard<std::mutex> lock(m_ExecuteMutex);

			if (context) context->taskCount.fetch_add(1);

			m_Tasks.emplace(std::make_pair(std::move(task), context));
			m_ConditionVar.notify_one();
		}

		inline void Execute(sv::TaskFunction* tasks, ui32 count, sv::ThreadContext* context = nullptr) noexcept
		{
			std::lock_guard<std::mutex> lock(m_ExecuteMutex);

			if (context) context->taskCount.fetch_add(count);

			for (ui32 i = 0; i < count; ++i) {
				m_Tasks.emplace(std::make_pair(std::move(tasks[i]), context));
			}

			m_ConditionVar.notify_all();
		}

		inline bool Running(const sv::ThreadContext& pContext) const noexcept
		{
			return pContext.executedTasks.load() != pContext.taskCount.load();
		}

		inline void Wait(const sv::ThreadContext& pContext) noexcept
		{
			while (Running(pContext)) {
				std::this_thread::yield();
				m_ConditionVar.notify_all();
			}
		}

		inline void Stop() noexcept
		{
			m_Running = false;
			m_ConditionVar.notify_all();

			for (ui32 i = 0; i < m_Threads.size(); ++i) m_Threads[i].join();
		}

		inline ui32 GetThreadCount() const noexcept
		{
			return ui32(m_Threads.size());
		}

	};

	static ThreadPool	g_ThreadPool;
	static ui32			g_MinThreads = 0;
	static std::mutex	g_BlockingTaskMutex;

	bool task_initialize(ui32 minThreads)
	{
		g_MinThreads = minThreads;
		ui32 numThreads = std::thread::hardware_concurrency();
		if (numThreads < g_MinThreads) numThreads = g_MinThreads;

		sv::log_info("Reserving %u threads", numThreads);

		g_ThreadPool.Reserve(numThreads);
		return true;
	}
	bool task_close()
	{
		g_ThreadPool.Stop();
		return true;
	}

}

namespace sv {

	using namespace _sv;

	void task_execute(const TaskFunction& task, ThreadContext* context, bool blockingTask)
	{
		g_ThreadPool.Execute(task, context);
		
		if (blockingTask)
		{
			std::lock_guard<std::mutex> lock(g_BlockingTaskMutex);
			g_ThreadPool.Reserve(1);
		}
	}
	void task_execute(TaskFunction* task, ui32 count, ThreadContext* context, bool blockingTask)
	{
		g_ThreadPool.Execute(task, count, context);
		if (blockingTask)
		{
			std::lock_guard<std::mutex> lock(g_BlockingTaskMutex);
			g_ThreadPool.Reserve(1);
		}
	}
	void task_wait(const ThreadContext& context)
	{
		g_ThreadPool.Wait(context);
	}
	bool task_running(const ThreadContext& context)
	{
		return g_ThreadPool.Running(context);
	}

	ui32 task_thread_count() noexcept
	{
		return std::thread::hardware_concurrency();
	}

}