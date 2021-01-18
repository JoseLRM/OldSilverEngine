#include "SilverEngine/core.h"

#include "SilverEngine/engine.h"
#include "core_internal.h"

#include "task_system/task_system_internal.h"
#include "asset_system/asset_system_internal.h"
#include "event_system/event_system_internal.h"
#include "window/window_internal.h"
#include "input/input_internal.h"
#include "graphics/graphics_internal.h"
#include "render_utils/render_utils_internal.h"


namespace sv {

	GlobalEngineData g_Engine;

#define CATCH catch (std::exception e) {\
					SV_LOG_ERROR("STD Exception: %s", e.what()); \
					exception_catched = true;\
					sv::show_message(L"STD Exception", sv::parse_wstring(e.what()).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (int i) {\
					SV_LOG_ERROR("Unknown Exception %i", i); \
					exception_catched = true;\
					sv::show_message(L"Unknown Exception", std::to_wstring(i).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (...) {\
					SV_LOG_ERROR("Unknown Exception"); \
					exception_catched = true;\
					sv::show_message(L"Unknown Exception", L"", sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}

	// Initialization

#define INIT_SYSTEM(name, fn) res = fn; if (result_okay(res)) SV_LOG_INFO(name ": %s", result_str(res)); else { SV_LOG_ERROR(name ": %s", result_str(res)); return res; }


	Result engine_initialize(const InitializationDesc* d)
	{
		if (d == nullptr) return Result_InvalidUsage;

		bool exception_catched = false;

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_Engine.app_callbacks = desc.callbacks;

		Time initTimeBegin = timer_now();

		// SYSTEMS
		try {

			Result res;

			res = logging_initialize();
			if (result_fail(res)) {
				printf("Can't initialize the logging system: %s\n", result_str(res));
				return res;
			}

			SV_LOG_CLEAR();
			SV_LOG_INFO("Initializing %s", g_Engine.name.c_str());

			INIT_SYSTEM("TaskSystem", task_initialize(desc.minThreadsCount));
			INIT_SYSTEM("EventSystem", event_initialize());
			INIT_SYSTEM("AssetSystem", asset_initialize(desc.assetsFolderPath));
			INIT_SYSTEM("Window", window_initialize());
			INIT_SYSTEM("GraphicsAPI", graphics_initialize());
			INIT_SYSTEM("RenderUtils", render_utils_initialize());

			// Open Main Window
			res = window_create(&desc.windowDesc, &g_Engine.window);
			if (result_fail(res)) {
				SV_LOG_ERROR("Can't open the main window...");
				return res;
			}

			// APPLICATION
			INIT_SYSTEM("Application", g_Engine.app_callbacks.initialize());
		}
		CATCH;

		SV_LOG_INFO("Initialized successfuly");
		
		Time initTimeEnd = timer_now();

		SV_LOG_INFO("Time: %f", initTimeEnd - initTimeBegin);

		SV_LOG_SEPARATOR();

		g_Engine.able_to_run = true;
		
		return Result_Success;
	}

	Result engine_loop()
	{
		if (!g_Engine.able_to_run) {
			printf("The engine is not able to run\n");
			return Result_CloseRequest;
		}

		if (g_Engine.close_request)
		{
			return Result_CloseRequest;
		}

		static Time	lastTime = 0.f;
		bool exception_catched = false;

		try {

			// Calculate DeltaTime
			Time actualTime = timer_now();
			g_Engine.deltatime = f32(actualTime - lastTime);
			lastTime = actualTime;

			if (g_Engine.deltatime > 0.3f) g_Engine.deltatime = 0.3f;

			// Calculate FPS
			{
				static f32 fpsTimeCount = 0.f;
				static u32 fpsCount = 0u;

				fpsTimeCount += g_Engine.deltatime;
				++fpsCount;
				if (fpsTimeCount > 0.25f) {
					fpsTimeCount -= 0.25f;
					g_Engine.FPS = fpsCount * 4u;
					fpsCount = 0u;
				}
			}

			// Update Input
			input_update();

			// Update Window
			window_update();

			if (!window_update(g_Engine.window)) {
				g_Engine.close_request = true;
				g_Engine.window = nullptr;
				return Result_CloseRequest;
			}

			// Update assets
			asset_update();

			// Update User
			g_Engine.app_callbacks.update();

			if (window_has_valid_surface(g_Engine.window)) {

				// Begin Rendering
				graphics_begin();

				// User Rendering
				g_Engine.app_callbacks.render();

				// End frame
				graphics_end();
			}
		}
		CATCH;

		++g_Engine.frame_count;

		if (exception_catched) 
			g_Engine.close_request = true;

		return Result_Success;
	}

	Result engine_close()
	{
		if (!g_Engine.able_to_run) {
			printf("The engine is not able to close\n");
			return Result_InvalidUsage;
		}
		g_Engine.able_to_run = false;

		bool exception_catched = false;

		SV_LOG_SEPARATOR();
		SV_LOG_INFO("Closing %s", g_Engine.name.c_str());

		// APPLICATION
		try {
			svCheck(g_Engine.app_callbacks.close());

			// Close Window
			svCheck(window_destroy(g_Engine.window));

			if (result_fail(render_utils_close())) { SV_LOG_ERROR("Can't close render utils"); }
			if (result_fail(graphics_close())) { SV_LOG_ERROR("Can't close graphicsAPI"); }
			if (result_fail(window_close())) { SV_LOG_ERROR("Can't close window"); }
			if (result_fail(asset_close())) { SV_LOG_ERROR("Can't close the asset system"); }
			if (result_fail(event_close())) { SV_LOG_ERROR("Can't close the event system"); }
			if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }
			logging_close();
		}
		CATCH;

		return Result_Success;
	}

}