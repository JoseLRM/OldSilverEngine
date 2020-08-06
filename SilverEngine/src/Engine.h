#pragma once

#include "core.h"
#include "utils/Version.h"
#include "Application.h"
#include "engine_input.h"
#include "engine_state.h"
#include "renderer/Renderer.h"
#include "platform/Window.h"
#include "graphics/Graphics.h"
#include "physics/physics.h"

///////////////////Initialization Parameters////////////////////////////////

struct SV_ENGINE_INITIALIZATION_DESC {
	SV_WINDOW_INITIALIZATION_DESC windowDesc;
	SV_RENDERER_INITIALIZATION_DESC rendererDesc;
	SV_GRAPHICS_INITIALIZATION_DESC graphicsDesc;
	SV_PHYSICS_INITIALIZATION_DESC physicsDesc;
	bool showConsole;
	ui32 minThreadsCount;
};

////////////////////////////////////////////////////////////////////////////////
namespace sv {

	bool engine_initialize(Application* app, const SV_ENGINE_INITIALIZATION_DESC* desc);
	bool engine_loop();
	bool engine_close();

	Version	engine_version_get() noexcept;
	float engine_deltatime_get() noexcept;
	ui64 engine_stats_get_frame_count() noexcept;

	Application& application_get()	noexcept;

}