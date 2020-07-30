#include "core.h"

#include "engine.h"

using namespace sv;

namespace _sv {

	static Version		g_Version = { 0u, 0u, 0u };
	static std::string	g_Name;

	static float		g_FixedUpdateFrameRate = 1 / 60.f;
	static float		g_DeltaTime = 0.f;

	static Application* g_Application;

}

namespace sv {

	using namespace _sv;

		

	bool engine_initialize(Application* app, const SV_ENGINE_INITIALIZATION_DESC* d)
	{
		SV_ASSERT(app);
		g_Application = app;

		// Initialization Parameters
		const SV_ENGINE_INITIALIZATION_DESC& desc = *d;

		g_Name = "SilverEngine ";
		g_Name += g_Version.ToString();

		sv::log_separator();
		sv::log_info("Initializing %s", g_Name.c_str());

		// CONSOLE
		if (desc.showConsole) _sv::console_show();
		else _sv::console_hide();

		// SYSTEMS

		svCheck(utils_initialize());
		svCheck(task_initialize(desc.minThreadsCount));
		svCheck(window_initialize(desc.windowDesc));
		svCheck(graphics_initialize(desc.graphicsDesc));
		svCheck(renderer_initialize(desc.rendererDesc));

		// APPLICATION
		g_Application->Initialize();

		return true;
	}

	bool engine_loop()
	{
		static Time		lastTime		= 0.f;
		static float	fixedUpdateTime = 0.f;
		static float	showFPSTime		= 0.f;
		static ui32		FPS				= 0u;
		bool			running			= true;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			g_DeltaTime = actualTime - lastTime;
			lastTime = actualTime;

			if (g_DeltaTime > 0.3f) g_DeltaTime = 0.3f;

			// Update Input
			if (input_update()) return false;
			window_update();

			// Updating
			engine_state_update();

			// Get current state
			State* state = engine_state_get_state();
			LoadingState* loadingState = engine_state_get_loadingstate();

			// Update state/loadingState and application
			if (state) state->Update(g_DeltaTime);
			else loadingState->Update(g_DeltaTime);

			// Fixed update state/loadingState and application
			fixedUpdateTime += g_DeltaTime;
			g_Application->Update(g_DeltaTime);
			if (fixedUpdateTime >= g_FixedUpdateFrameRate) {

				if (state) state->FixedUpdate();
				else loadingState->FixedUpdate();

				g_Application->FixedUpdate();
				fixedUpdateTime -= g_FixedUpdateFrameRate;
			}

			// Rendering
			renderer_frame_begin();

			if (state) state->Render();
			else loadingState->Render();

			g_Application->Render();

			renderer_frame_end();

			// Calculate FPS
			FPS++;
			showFPSTime += g_DeltaTime;
			while (showFPSTime >= 1.f) {
				showFPSTime -= 1.f;
				sv::log("FPS: %u", FPS);
				FPS = 0u;
			}
		}
		catch (Exception e) {
			sv::log_error("%s: '%s'\nFile: '%s', Line: %u", e.type.c_str(), e.desc.c_str(), e.file.c_str(), e.line);
			running = false;
		}
		catch (std::exception e) {
			sv::log_error("STD Exception: '%s'", e.what());
			running = false;
		}
		catch (int i) {
			sv::log_error("Unknown Error: %i", i);
			running = false;
		}
		catch (...) {
			sv::log_error("Unknown Error");
			running = false;
		}

		return running;
	}
	bool engine_close()
	{
		sv::log_separator();
		sv::log_info("Closing %s", g_Name.c_str());

		// APPLICATION
		g_Application->Close();
		g_Application = nullptr;

		engine_state_close();
		svCheck(renderer_close());
		svCheck(window_close());
		svCheck(graphics_close());
		svCheck(task_close());

		return true;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime;	}

	Application& application_get() noexcept { return *g_Application; }
	
}