#include "core.h"

#include "engine.h"

#include "renderer/renderer_internal.h"
#include "graphics/graphics_internal.h"
#include "physics/physics_internal.h"
#include "input/input_internal.h"
#include "platform/window_internal.h"
#include "task_system/task_system_internal.h"
#include "utils/utils_internal.h"

using namespace sv;

namespace sv {

	static Version		g_Version = { 0u, 0u, 0u };
	static std::string	g_Name;

	static float		g_DeltaTime = 0.f;
	static float		g_TimeStep = 1.f;

	static ui64			g_FrameCount = 0u;

	static ApplicationCallbacks g_App;

	bool engine_initialize(const InitializationDesc* d)
	{
		SV_ASSERT(d != nullptr);

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_App = desc.callbacks;

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
		svCheck(physics_initialize(desc.physicsDesc));
		svCheck(window_initialize(desc.windowDesc));
		svCheck(graphics_initialize(desc.graphicsDesc));
		svCheck(renderer_initialize(desc.rendererDesc));

		// APPLICATION
		svCheck(g_App.initialize());

		return true;
	}

	bool engine_loop()
	{
		static Time		lastTime		= 0.f;
		static float	showFPSTime		= 0.f;
		static ui32		FPS				= 0u;
		bool			running			= true;

		g_FrameCount++;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			g_DeltaTime = actualTime - lastTime;
			lastTime = actualTime;

			if (g_DeltaTime > 0.3f) g_DeltaTime = 0.3f;

			// Update Input
			if (input_update()) return false;
			window_update();

			// Update User
			g_App.update(g_DeltaTime);

			// Update Physics
			physics_update(g_DeltaTime);

			// Begin Rendering
			renderer_frame_begin();

			// User Rendering
			g_App.render();

			// End Rendering
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
		svCheck(g_App.close());

		svCheck(renderer_close());
		svCheck(window_close());
		svCheck(graphics_close());
		svCheck(physics_close());
		svCheck(task_close());

		return true;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime;	}

	void engine_timestep_set(float timestep) noexcept
	{
		g_TimeStep = timestep;
	}
	float engine_timestep_get() noexcept
	{
		return g_TimeStep;
	}

	ui64 engine_stats_get_frame_count() noexcept { return g_FrameCount; }
	
}