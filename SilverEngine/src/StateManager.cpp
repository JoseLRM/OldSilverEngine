#include "core.h"

#include "StateManager.h"
#include "Engine.h"

namespace SV {
	namespace StateManager {

		State* g_CurrentState;

		State* g_OldState;
		LoadingState* g_LoadingState;
		State* g_NewState;

		SV::ThreadContext g_UnloadContext;
		SV::ThreadContext g_LoadContext;

		namespace _internal {

			void Update()
			{
				if (g_CurrentState) return;

				if (!IsLoading()) {

					if (!g_NewState) return;

					if (g_OldState) {
						g_OldState->Close();
						delete g_OldState;
						g_OldState = nullptr;
					}

					g_LoadingState->Close();
					delete g_LoadingState;
					g_LoadingState = nullptr;

					g_NewState->Initialize();
					g_CurrentState = g_NewState;
					g_NewState = nullptr;
				}
			}

			void Close()
			{
				if (IsLoading()) {
					exit(1);
				}

				if (g_CurrentState) {
					g_CurrentState->Unload();
					g_CurrentState->Close();
					delete g_CurrentState;
					g_CurrentState = nullptr;
				}
			}

		}

		void LoadState(State* state, LoadingState* loadingState)
		{
			SV_ASSERT(state && !IsLoading());

			g_NewState = state;
			g_OldState = g_CurrentState;
			g_CurrentState = nullptr;

			if (!loadingState) loadingState = new LoadingState();
			g_LoadingState = loadingState;
			g_LoadingState->Initialize();

			g_UnloadContext.Reset();
			g_LoadContext.Reset();

			if (g_OldState) {
				SV::Task::Execute([]() {

					g_OldState->Unload();

				}, &g_UnloadContext, true);
			}

			SV::Task::Execute([]() {

				g_NewState->Load();

			}, &g_LoadContext, true);

		}

		bool IsLoading()
		{
			return SV::Task::Running(g_UnloadContext) || SV::Task::Running(g_LoadContext);
		}

		State* GetCurrentState() noexcept { return g_CurrentState; }
		LoadingState* GetLoadingState() noexcept { return g_LoadingState; }
	}
}