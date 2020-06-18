#include "core.h"

#include "StateManager.h"
#include "Engine.h"

namespace SV {

	State::State(SV::Engine& engine)
	{
		SetEngine(&engine);
	}

	StateManager::StateManager()
		: m_CurrentState(nullptr), m_OldState(nullptr), m_LoadingState(new LoadingState()), m_NewState(nullptr)
	{}

	void StateManager::LoadState(State* state, LoadingState* loadingState)
	{
		SV_ASSERT(state && !IsLoading());

		m_NewState = state;
		m_OldState = m_CurrentState;
		m_CurrentState = nullptr;

		if (!loadingState) loadingState = new LoadingState();
		m_LoadingState = loadingState;
		m_LoadingState->Initialize();
		
		m_UnloadContext.Reset();
		m_LoadContext.Reset();

		if (m_OldState) {
			SV::Task::Execute([this]() {

				m_OldState->Unload();

			}, &m_UnloadContext, true);
		}

		SV::Task::Execute([this]() {

			m_NewState->Load();

		}, &m_LoadContext, true);

	}

	void StateManager::Update()
	{
		if (m_CurrentState) return;

		if (!IsLoading()) {

			if (!m_NewState) return;

			if (m_OldState) {
				m_OldState->Close();
				delete m_OldState;
				m_OldState = nullptr;
			}

			m_LoadingState->Close();
			delete m_LoadingState;
			m_LoadingState = nullptr;

			m_NewState->Initialize();
			m_CurrentState = m_NewState;
			m_NewState = nullptr;
		}
	}

	void StateManager::Close()
	{
		if (IsLoading()) {
			exit(1);
		}

		m_CurrentState->Unload();
		m_CurrentState->Close();
		delete m_CurrentState;
		m_CurrentState = nullptr;
	}

	bool StateManager::IsLoading() const
	{
		return SV::Task::Running(m_UnloadContext) || SV::Task::Running(m_LoadContext);
	}

}