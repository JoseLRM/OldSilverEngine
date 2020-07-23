#include "core.h"

#include "Engine.h"

#include "graphics/Graphics.h"

//////////////////////////////////////////////////////////////////////////////////////

namespace SV {
	namespace Engine {
		SV::Version		g_Version = { 0u, 0u, 0u };
		std::string		g_Name;

		float g_FixedUpdateFrameRate = 1 / 60.f;

		SV::Application*	g_Application;

		bool Engine::Initialize(Application* app, const SV_ENGINE_INITIALIZATION_DESC* d)
		{
			SV_ASSERT(app);
			g_Application = app;

			// Initialization Parameters
			const SV_ENGINE_INITIALIZATION_DESC& desc = *d;

			g_Name = "SilverEngine ";
			g_Name += g_Version.ToString();

			SV::LogSeparator();
			SV::LogI("Initializing %s", g_Name.c_str());

			// CONSOLE
			if (desc.showConsole) SV::_internal::ShowConsole();
			else SV::_internal::HideConsole();

			// TIMER
			SV::Timer::_internal::Initialize();

			// TASK SYSTEM
			SV::Task::_internal::Initialize(desc.minThreadsCount);

			// WINDOW
			if (!Window::_internal::Initialize(desc.windowDesc)) {
				SV::LogE("Can't initialize Window");
				return false;
			}

			// GRAPHICS
			if (!SV::Graphics::_internal::Initialize(desc.graphicsDesc)) {
				SV::LogE("Can't Initialize Graphics");
				return false;
			}

			// RENDERER
			if (!Renderer::_internal::Initialize(desc.rendererDesc)) {
				SV::LogE("Can't initialize Renderer");
				return false;
			}

			// APPLICATION
			g_Application->Initialize();

			return true;
		}

		bool Engine::Loop()
		{
			static Time		lastTime		= 0.f;
			static float	fixedUpdateTime = 0.f;
			static float	showFPSTime		= 0.f;
			static ui32		FPS				= 0u;
			bool			running			= true;

			try {

				// Calculate DeltaTime
				SV::Time actualTime = SV::Timer::Now();
				SV::Time deltaTime = actualTime - lastTime;
				lastTime = actualTime;

				if (deltaTime > 0.3f) deltaTime = 0.3f;

				// Update Input
				Window::_internal::Update();
				if (_internal::UpdateInput()) return false;

				// Updating
				StateManager::_internal::Update();

				// Get current state
				State* state = StateManager::GetCurrentState();
				LoadingState* loadingState = StateManager::GetLoadingState();

				// Update state/loadingState and application
				if (state) state->Update(deltaTime);
				else loadingState->Update(deltaTime);

				// Fixed update state/loadingState and application
				fixedUpdateTime += deltaTime;
				g_Application->Update(deltaTime);
				if (fixedUpdateTime >= g_FixedUpdateFrameRate) {

					if (state) state->FixedUpdate();
					else loadingState->FixedUpdate();

					g_Application->FixedUpdate();
					fixedUpdateTime -= g_FixedUpdateFrameRate;
				}

				// Rendering
				Renderer::_internal::BeginFrame();

				if (state) state->Render();
				else loadingState->Render();

				g_Application->Render();
				Renderer::_internal::Render();

				Renderer::_internal::EndFrame();

				// Calculate FPS
				FPS++;
				showFPSTime += deltaTime;
				while (showFPSTime >= 1.f) {
					showFPSTime -= 1.f;
					SV::Log("FPS: %u", FPS);
					FPS = 0u;
				}
			}
			catch (SV::_internal::Exception e) {
				SV::LogE("%s: '%s'\nFile: '%s', Line: %u", e.type.c_str(), e.desc.c_str(), e.file.c_str(), e.line);
				running = false;
			}
			catch (std::exception e) {
				SV::LogE("STD Exception: '%s'", e.what());
				running = false;
			}
			catch (int i) {
				SV::LogE("Unknown Error: %i", i);
				running = false;
			}
			catch (...) {
				SV::LogE("Unknown Error");
				running = false;
			}

			return running;
		}
		bool Close()
		{
			SV::LogSeparator();
			SV::LogI("Closing %s", g_Name.c_str());

			// APPLICATION
			g_Application->Close();
			g_Application = nullptr;

			StateManager::_internal::Close();

			// RENDERER
			if (!Renderer::_internal::Close()) {
				SV::LogE("Can't close Renderer");
			}

			// WINDOW
			if (!Window::_internal::Close()) {
				SV::LogE("Can't close Window");
			}

			// GRAPHICS
			if (!Graphics::_internal::Close()) {
				SV::LogE("Can't close Graphics");
			}

			SV::Task::_internal::Close();

			return true;
		}

		Application& GetApplication() noexcept { return *g_Application; }
		SV::Version GetVersion() noexcept { return g_Version; }
	}
}