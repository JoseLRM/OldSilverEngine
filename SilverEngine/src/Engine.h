#pragma once

#include "core.h"
#include "Window.h"
#include "Input.h"
#include "Version.h"

///////////////////Initialization Parameters////////////////////////////////

struct SV_ENGINE_INITIALIZATION_DESC {
	bool executeInThisThread;
	SV_WINDOW_INITIALIZATION_DESC windowDesc;

	void SetDefault();
};

////////////////////////////////////////////////////////////////

namespace SV {

	bool Initialize();
	bool Close();

	class Engine {

		ui32 m_InstanceID = 0u;
		enum {
			SV_ENGINE_STATE_NONE,
			SV_ENGINE_STATE_INITIALIZING,
			SV_ENGINE_STATE_RUNNING,
			SV_ENGINE_STATE_CLOSING
		} m_EngineState;

		SV::Version m_Version;

		SV::Window	m_Window;
		SV::Input	m_Input;

	public:
		Engine();
		~Engine();

		Engine& operator=(const Engine& other) = delete;
		Engine& operator=(Engine&& other) = delete;

		void Run(const SV_ENGINE_INITIALIZATION_DESC* desc = nullptr);

		// Devices Getters
		inline Window& GetWindow() noexcept { return m_Window; }
		inline Input& GetInput() noexcept { return m_Input; }

		// State Getters
		inline bool IsRunning() const noexcept { return m_EngineState != SV_ENGINE_STATE_NONE; }

	private:
		void _Run(SV_ENGINE_INITIALIZATION_DESC& desc);

		bool Initialize(SV_ENGINE_INITIALIZATION_DESC& desc);
		void Loop();
		bool Close();

	};

}