#pragma once

#include "core.h"

#include "window.h"
#include "graphics.h"
#include "renderer.h"
#include "task_system.h"
#include "input.h"
#include "scene.h"

// Main engine

namespace sv {

	struct ApplicationCallbacks {
		Result(*initialize)();
		void(*update)(float);
		void(*render)();
		Result(*close)();
	};

	struct InitializationDesc {
		ApplicationCallbacks callbacks;
		InitializationSceneDesc sceneDesc;
		InitializationWindowDesc windowDesc;
		InitializationGraphicsDesc graphicsDesc;
		InitializationRendererDesc rendererDesc;
		InitializationConsoleDesc consoleDesc;
		ui32 minThreadsCount;
	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

	Version	engine_version_get() noexcept;
	float	engine_deltatime_get() noexcept;
	ui64	engine_stats_get_frame_count() noexcept;

}