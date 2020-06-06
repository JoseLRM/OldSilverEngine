#pragma once

#include "core.h"
#include "EngineDevice.h"
#include "Window.h"
#include "Input.h"
#include "Graphics.h"
#include "Renderer.h"
#include "Application.h"
#include "Version.h"

///////////////////Initialization Parameters////////////////////////////////

struct SV_ENGINE_INITIALIZATION_DESC {
	bool executeInThisThread;
	SV_WINDOW_INITIALIZATION_DESC	windowDesc;
	SV_GRAPHICS_INITIALIZATION_DESC graphicsDesc;
	SV_RENDERER_INITIALIZATION_DESC rendererDesc;

	void SetDefault();
};

/////////////////////Static Initialization///////////////////////////////////////////

struct SV_ENGINE_STATIC_INITIALIZATION_DESC {
	bool showConsole;
	ui32 minThreadsCount;

	void SetDefault();
};

namespace SV {
	bool Initialize(const SV_ENGINE_STATIC_INITIALIZATION_DESC* desc = nullptr);
	bool Close();
}

////////////////////////////////////////////////////////////////////////////////
namespace SV {

	class Engine {

		enum {
			SV_ENGINE_STATE_NONE,
			SV_ENGINE_STATE_INITIALIZING,
			SV_ENGINE_STATE_RUNNING,
			SV_ENGINE_STATE_CLOSING
		} m_EngineState;

		ui32			m_InstanceID = 0u;
		SV::Version		m_Version = { 0u, 0u, 0u };
		std::string		m_Name;

		// Devices
		SV::Window			m_Window;
		SV::Graphics		m_Graphics;
		SV::Renderer		m_Renderer;
		SV::Input			m_Input;
		SV::Application*	m_Application;

	public:
		Engine();
		~Engine();

		Engine& operator=(const Engine& other) = delete;
		Engine& operator=(Engine&& other) = delete;

		void Run(Application* app, const SV_ENGINE_INITIALIZATION_DESC* desc = nullptr);
		void Exit(bool endFrame = true);

		// Devices Getters
		inline Window&		GetWindow() noexcept { return m_Window; }
		inline Graphics&	GetGraphics() noexcept { return m_Graphics; }
		inline Renderer&	GetRenderer() noexcept { return m_Renderer; }
		inline Input&		GetInput() noexcept { return m_Input; }
		inline Application& GetApplication() noexcept { return *m_Application; }

		// State Getters
		inline bool IsInitializing() const noexcept { return m_EngineState == SV_ENGINE_STATE_INITIALIZING; }
		inline bool IsRunning() const noexcept { return m_EngineState != SV_ENGINE_STATE_NONE; }

	private:
		void _Run(SV_ENGINE_INITIALIZATION_DESC& desc);

		bool Initialize(SV_ENGINE_INITIALIZATION_DESC& desc);
		void Loop();
		bool Close();

	};

}