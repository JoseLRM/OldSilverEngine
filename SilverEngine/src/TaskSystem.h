#pragma once

#include "core.h"

namespace SV {

	namespace Task {

		using Task = std::function<void()>;

		class ThreadPool;

		class ThreadContext {
			std::atomic<ui32> executedTasks = 0u;
			ui32 taskCount = 0u;
		public:
			inline void Reset() noexcept
			{
				executedTasks = 0u;
				taskCount = 0u;
			}
			friend ThreadPool;
		};

		namespace _internal {
			void Initialize(ui32 minThreadCount);
			void Close();
		}
		void Execute(const Task& task, ThreadContext* context = nullptr, bool blockingTask = false);
		void Execute(Task* task, ui32 count, ThreadContext* context, bool blockingTask = false);
		void Wait(const ThreadContext& context);
		bool Running(const ThreadContext& context);

	}

}