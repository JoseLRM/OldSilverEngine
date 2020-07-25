#pragma once

#include "core.h"

namespace SV {

	class LoadingState {

	public:
		virtual void Initialize() {}

		virtual void Update(float dt) {}
		virtual void FixedUpdate() {}
		virtual void Render() {}

		virtual void Close() {}

	};

	class State {
	public:
		virtual void Load() {}
		virtual void Initialize() {}

		virtual void Update(float dt) {}
		virtual void FixedUpdate() {}
		virtual void Render() {}

		virtual void Unload() {}
		virtual void Close() {}

	};

	namespace StateManager {

		namespace _internal {
			void Update();
			void Close();
		}

		void LoadState(State* state, LoadingState* loadingState = nullptr);
		State* GetCurrentState() noexcept;
		LoadingState* GetLoadingState() noexcept;

		bool IsLoading();

	}
}