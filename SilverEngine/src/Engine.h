#pragma once

#include "core.h"
#include "Input.h"
#include "renderer/Renderer.h"
#include "Application.h"
#include "StateManager.h"
#include "utils/Version.h"
#include "graphics/Window.h"

///////////////////Initialization Parameters////////////////////////////////

struct SV_ENGINE_INITIALIZATION_DESC {
	SV_WINDOW_INITIALIZATION_DESC windowDesc;
	SV_RENDERER_INITIALIZATION_DESC rendererDesc;
	SV_GRAPHICS_INITIALIZATION_DESC graphicsDesc;
	bool showConsole;
	ui32 minThreadsCount;
};

////////////////////////////////////////////////////////////////////////////////
namespace SV {

	namespace Engine {

		bool Initialize(Application* app, const SV_ENGINE_INITIALIZATION_DESC* desc = nullptr);
		bool Loop();
		bool Close();

		Application&	GetApplication()	noexcept;
		SV::Version		GetVersion()		noexcept;

	}

}