#pragma once

#include "Window.h"
#include "Version.h"

///////////////////Initialization Parameters////////////////////////////////

struct SV_ENGINE_INITIALIZATION_DESC {
	bool executeInThisThread;
	SV_WINDOW_INITIALIZATION_DESC windowDesc;

	void SetDefault();
};

////////////////////////////////////////////////////////////////

namespace SV {

	class Engine {

		enum {
			SV_ENGINE_STATE_NONE,
			SV_ENGINE_STATE_INITIALIZING,
			SV_ENGINE_STATE_RUNNING,
			SV_ENGINE_STATE_CLOSING
		} m_EngineState;

		SV::Version m_Version;

		SV::Window m_Window;

	public:
		Engine();
		~Engine();

		Engine& operator=(const Engine& other) = delete;
		Engine& operator=(Engine&& other) = delete;

		void Run(const SV_ENGINE_INITIALIZATION_DESC* desc = nullptr);

	};

}