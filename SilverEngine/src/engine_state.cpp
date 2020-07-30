#include "core.h"

#include "engine_state.h"
#include "engine.h"

using namespace sv;

namespace _sv {

	static State*			g_CurrentState;

	static State*			g_OldState;
	static LoadingState*	g_LoadingState;
	static State*			g_NewState;

	static ThreadContext	g_UnloadContext;
	static ThreadContext	g_LoadContext;

	void engine_state_update()
	{
		if (g_CurrentState) return;

		if (!engine_state_loading()) {

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
	void engine_state_close()
	{
		if (engine_state_loading()) {
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

namespace sv {

	using namespace _sv;
	
	void engine_state_load(State* state, LoadingState* loadingState)
	{
		SV_ASSERT(state && !engine_state_loading());

		g_NewState = state;
		g_OldState = g_CurrentState;
		g_CurrentState = nullptr;

		if (!loadingState) loadingState = new LoadingState();
		g_LoadingState = loadingState;
		g_LoadingState->Initialize();

		g_UnloadContext.Reset();
		g_LoadContext.Reset();

		if (g_OldState) {
			task_execute([]() {

				g_OldState->Unload();

			}, &g_UnloadContext, true);
		}

		task_execute([]() {

			g_NewState->Load();

		}, &g_LoadContext, true);

	}

	bool engine_state_loading()
	{
		return task_running(g_UnloadContext) || task_running(g_LoadContext);
	}

	State* engine_state_get_state() noexcept { return g_CurrentState; }
	LoadingState* engine_state_get_loadingstate() noexcept { return g_LoadingState; }
	
}