#pragma once

#include "core.h"

#include "window.h"
#include "graphics.h"
#include "renderer.h"
#include "physics.h"
#include "task_system.h"
#include "input.h"

// Main engine

namespace sv {

	struct ApplicationCallbacks {
		bool(*initialize)();
		void(*update)(float);
		void(*render)();
		bool(*close)();
	};

	struct InitializationDesc {
		ApplicationCallbacks callbacks;
		InitializationWindowDesc windowDesc;
		InitializationGraphicsDesc graphicsDesc;
		InitializationRendererDesc rendererDesc;
		InitializationPhysicsDesc physicsDesc;
		bool showConsole;
		ui32 minThreadsCount;
	};

	bool engine_initialize(const InitializationDesc* desc);
	bool engine_loop();
	bool engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	void	engine_timestep_set(float timestep) noexcept;
	float	engine_timestep_get() noexcept;
	ui64	engine_stats_get_frame_count() noexcept;

}