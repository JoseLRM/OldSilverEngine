#pragma once

#include "core.h"
#include "EngineDevice.h"
#include "Scene.h"

namespace SV {

	class Engine;

	class LoadingState {

	public:
		virtual void Initialize() {}

		virtual void Update(float dt) {}
		virtual void FixedUpdate() {}
		virtual void Render() {}

		virtual void Close() {}

	};

	class State : public SV::EngineDevice {

	public:
		State(SV::Engine& engine);

		virtual void Load() {}
		virtual void Initialize() {}

		virtual void Update(float dt) {}
		virtual void FixedUpdate() {}
		virtual void Render() {}

		virtual void Unload() {}
		virtual void Close() {}

	};

	class StateManager {
		State* m_CurrentState;

		State* m_OldState;
		LoadingState* m_LoadingState;
		State* m_NewState;

		SV::ThreadContext m_UnloadContext;
		SV::ThreadContext m_LoadContext;

	public:
		StateManager();

		void LoadState(State* state, LoadingState* loadingState = nullptr);
		State* GetCurrentState() const noexcept { return m_CurrentState; }
		LoadingState* GetLoadingState() const noexcept { return m_LoadingState; }

		void Update();
		void Close();

		bool IsLoading() const;

	};

}