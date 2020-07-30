#pragma once

#include "core.h"

namespace _sv {

	void engine_state_update();
	void engine_state_close();

}

namespace sv {

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

	void engine_state_load(State* state, LoadingState* loadingState);
	State* engine_state_get_state() noexcept;
	LoadingState* engine_state_get_loadingstate() noexcept;
	bool engine_state_loading();

}