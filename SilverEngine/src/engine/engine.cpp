#include "core.h"

#include "engine.h"

#include "renderer/renderer_internal.h"
#include "graphics/graphics_internal.h"
#include "input/input_internal.h"
#include "window/window_internal.h"
#include "task_system/task_system_internal.h"
#include "utils/utils_internal.h"
#include "scene/scene_internal.h"
#include "console/console_internal.h"
#include "asset_system/asset_system_internal.h"

#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red | sv::LoggingStyle_Green | sv::LoggingStyle_BackgroundBlue, "[ENGINE] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red | sv::LoggingStyle_Green | sv::LoggingStyle_BackgroundBlue, "[ENGINE_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[ENGINE_ERROR] "#x, __VA_ARGS__)

#define svCatch catch (Exception e) { \
					sv::console_log_error(true, e.title.c_str(), "%s\nFile: %s.\nLine %u", e.desc.c_str(), e.file.c_str(), e.line);\
					return Result_UnknownError;\
				}\
				catch (std::exception e) {\
					sv::console_log_error(true, "STD Exception", e.what());\
					return Result_UnknownError;\
				}\
				catch (int i) {\
					sv::console_log_error(true, "Unknown Exception", "%i", i);\
					return Result_UnknownError;\
				}\
				catch (...) {\
					sv::console_log_error(true, "Unknown Error", "No description...");\
					return Result_UnknownError;\
				}

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
		console_clear();

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_App = desc.callbacks;

		g_Name = "SilverEngine ";
		g_Name += g_Version.ToString();

		// SYSTEMS
		try {

			svCheck(console_initialize(desc.consoleDesc));

			svLog("Initializing %s", g_Name.c_str());

			svCheck(utils_initialize());
			svCheck(task_initialize(desc.minThreadsCount));
			svCheck(assets_initialize(desc.assetsFolderPath));
			svCheck(scene_initialize());
			svCheck(window_initialize(desc.windowDesc));
			svCheck(graphics_initialize(desc.graphicsDesc));
			svCheck(renderer_initialize(desc.rendererDesc));

			// APPLICATION
			svCheck(g_App.initialize());
		}
		svCatch;

		svLog("Initialized successfuly");
		sv::console_log_separator();

		return Result_Success;
	}

	Result engine_loop()
	{
		static Time		lastTime		= 0.f;

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

			// Update assets
			svCheck(assets_update(g_DeltaTime));

			// Update User
			g_App.update(g_DeltaTime);

			// Begin Rendering
			renderer_frame_begin();

			// User Rendering
			g_App.render();

			// End Rendering
			renderer_frame_end();
		}
		svCatch;

		return Result_Success;
	}

	Result engine_close()
	{
		console_log_separator();
		svLog("Closing %s", g_Name.c_str());

		// APPLICATION
		try {
			svCheck(g_App.close());

			svCheck(renderer_close());
			svCheck(window_close());
			svCheck(graphics_close());
			svCheck(scene_close());
			svCheck(assets_close());
			svCheck(task_close());
			svCheck(console_close());
		}
		svCatch;

		return Result_Success;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime;	}

	ui64 engine_stats_get_frame_count() noexcept { return g_FrameCount; }
	
}