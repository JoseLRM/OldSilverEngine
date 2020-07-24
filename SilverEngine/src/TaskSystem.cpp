#include "core.h"

#include "TaskSystem.h"

namespace SV {
	namespace Task {
		class ThreadPool {
			std::vector<std::thread> m_Threads;

			std::condition_variable m_ConditionVar;
			std::mutex m_SleepMutex;
			std::mutex m_ExecuteMutex;

			std::queue<std::pair<TaskFunction, ThreadContext*>> m_Tasks;

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
						TaskFunction task;
						ThreadContext* pContext;

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

			inline void Execute(const TaskFunction& task, ThreadContext* context = nullptr) noexcept
			{
				std::lock_guard<std::mutex> lock(m_ExecuteMutex);

				if (context) context->taskCount.fetch_add(1);

				m_Tasks.emplace(std::make_pair(std::move(task), context));
				m_ConditionVar.notify_one();
			}

			inline void Execute(TaskFunction* tasks, ui32 count, ThreadContext* context = nullptr) noexcept
			{
				std::lock_guard<std::mutex> lock(m_ExecuteMutex);

				if (context) context->taskCount.fetch_add(count);

				for (ui32 i = 0; i < count; ++i) {
					m_Tasks.emplace(std::make_pair(std::move(tasks[i]), context));
				}

				m_ConditionVar.notify_all();
			}

			inline bool Running(const ThreadContext& pContext) const noexcept
			{
				return pContext.executedTasks.load() != pContext.taskCount.load();
			}

			inline void Wait(const ThreadContext& pContext) noexcept
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

		ThreadPool g_ThreadPool;
		ui32 g_MinThreads = 0;

		std::mutex g_BlockingTaskMutex;

		namespace _internal {
			void Initialize(ui32 minThreads)
			{
				g_MinThreads = minThreads;
				ui32 numThreads = std::thread::hardware_concurrency();
				if (numThreads < g_MinThreads) numThreads = g_MinThreads;

				SV::LogI("Reserving %u threads", numThreads);

				g_ThreadPool.Reserve(numThreads);
			}
			void Close()
			{
				g_ThreadPool.Stop();
			}
		}

		void Execute(const TaskFunction& task, ThreadContext* context, bool blockingTask)
		{
			g_ThreadPool.Execute(task, context);
			
			if (blockingTask)
			{
				std::lock_guard<std::mutex> lock(g_BlockingTaskMutex);
				g_ThreadPool.Reserve(1);
			}
		}
		void Execute(TaskFunction* task, ui32 count, ThreadContext* context, bool blockingTask)
		{
			g_ThreadPool.Execute(task, count, context);
			if (blockingTask)
			{
				std::lock_guard<std::mutex> lock(g_BlockingTaskMutex);
				g_ThreadPool.Reserve(1);
			}
		}
		void Wait(const ThreadContext& context)
		{
			g_ThreadPool.Wait(context);
		}
		bool Running(const ThreadContext& context)
		{
			return g_ThreadPool.Running(context);
		}

		ui32 GetThreadCount() noexcept
		{
			return std::thread::hardware_concurrency();
		}

	}
}