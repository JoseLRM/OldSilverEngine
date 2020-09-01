#include "core.h"

#include "engine.h"

#include "renderer/renderer_internal.h"
#include "graphics/graphics_internal.h"
#include "input/input_internal.h"
#include "platform/window_internal.h"
#include "task_system/task_system_internal.h"
#include "utils/utils_internal.h"
#include "scene/scene_internal.h"

using namespace sv;

namespace sv {

	static Version		g_Version = { 0u, 0u, 0u };
	static std::string	g_Name;

	static float		g_DeltaTime = 0.f;

	static ui64			g_FrameCount = 0u;

	static ApplicationCallbacks g_App;

	Result engine_initialize(const InitializationDesc* d)
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
		if (desc.showConsole) console_show();
		else console_hide();

		// SYSTEMS

		svCheck(utils_initialize());
		svCheck(task_initialize(desc.minThreadsCount));
		svCheck(scene_assets_initialize(desc.assetFolderPath));
		svCheck(window_initialize(desc.windowDesc));
		svCheck(graphics_initialize(desc.graphicsDesc));
		svCheck(renderer_initialize(desc.rendererDesc));

		// APPLICATION
		svCheck(g_App.initialize());

		return Result_Success;
	}

	Result engine_loop()
	{
		static Time		lastTime		= 0.f;
		static float	showFPSTime		= 0.f;
		static ui32		FPS				= 0u;
		Result			loopResult		= Result_Success;

		g_FrameCount++;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			g_DeltaTime = actualTime - lastTime;
			lastTime = actualTime;

			if (g_DeltaTime > 0.3f) g_DeltaTime = 0.3f;

			// Update Input
			if (input_update()) return Result_CloseRequest;
			window_update();

			// Update User
			g_App.update(g_DeltaTime);

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
			loopResult = Result_UnknownError;
			system("PAUSE");
		}
		catch (std::exception e) {
			sv::log_error("STD Exception: '%s'", e.what());
			loopResult = Result_UnknownError;
			system("PAUSE");
		}
		catch (int i) {
			sv::log_error("Unknown Error: %i", i);
			loopResult = Result_UnknownError;
			system("PAUSE");
		}
		catch (...) {
			sv::log_error("Unknown Error");
			loopResult = Result_UnknownError;
			system("PAUSE");
		}

		return loopResult;
	}

	Result engine_close()
	{
		sv::log_separator();
		sv::log_info("Closing %s", g_Name.c_str());

		// APPLICATION
		svCheck(g_App.close());

		svCheck(renderer_close());
		svCheck(window_close());
		svCheck(graphics_close());
		svCheck(scene_assets_close());
		svCheck(task_close());

		return Result_Success;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime;	}

	ui64 engine_stats_get_frame_count() noexcept { return g_FrameCount; }
	
}