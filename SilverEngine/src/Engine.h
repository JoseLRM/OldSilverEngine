#pragma once

#include "core.h"

#include "main/Application.h"

#include "window.h"
#include "graphics.h"
#include "renderer.h"
#include "physics.h"
#include "task_system.h"
#include "input.h"

// Main engine

namespace sv {

	struct InitializationDesc {
		InitializationWindowDesc windowDesc;
		InitializationGraphicsDesc graphicsDesc;
		InitializationRendererDesc rendererDesc;
		InitializationPhysicsDesc physicsDesc;
		bool showConsole;
		ui32 minThreadsCount;
	};

	bool engine_initialize(Application* app, const InitializationDesc* desc);
	bool engine_loop();
	bool engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	ui64	engine_stats_get_frame_count() noexcept;

	Application& application_get()	noexcept;

}

// State management

namespace sv {

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

	void	engine_state_load(State* state, State* loadingState = nullptr);
	State*	engine_state_get_state() noexcept;
	bool	engine_state_loading();

}