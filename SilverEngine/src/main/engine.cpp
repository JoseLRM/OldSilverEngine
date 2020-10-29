#include "core.h"

#include "main/engine.h"

#include "rendering/material_system/material_system_internal.h"
#include "rendering/debug_renderer/debug_renderer_internal.h"
#include "rendering/sprite_renderer/sprite_renderer_internal.h"
#include "platform/graphics/graphics_internal.h"
#include "platform/input/input_internal.h"
#include "platform/window/window_internal.h"
#include "task_system/task_system_internal.h"
#include "logging/logging_internal.h"
#include "simulation/asset_system/asset_system_internal.h"
#include "simulation/animator/animator_internal.h"
#include "main/event_system/event_system_internal.h"

#define svCatch catch (std::exception e) {\
					SV_LOG_ERROR("STD Exception: %s", e.what()); \
					sv::show_message(L"STD Exception", sv::parse_wstring(e.what()).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (int i) {\
					SV_LOG_ERROR("Unknown Exception %i", i); \
					sv::show_message(L"Unknown Exception", std::to_wstring(i).c_str(), sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
					return Result_UnknownError;\
				}\
				catch (...) {\
					SV_LOG_ERROR("Unknown Exception"); \
					sv::show_message(L"Unknown Exception", L"", sv::MessageStyle_IconError | sv::MessageStyle_Ok);\
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
		if (d == nullptr) return Result_InvalidUsage;

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_App = desc.callbacks;

		g_Name = "SilverEngine ";
		g_Name += g_Version.ToString();

		// SYSTEMS
		try {

			svCheck(logging_initialize());

			SV_LOG_CLEAR();
			SV_LOG_INFO("Initializing %s", g_Name.c_str());

			svCheck(task_initialize(desc.minThreadsCount));
			svCheck(event_initialize());
			svCheck(asset_initialize(desc.assetsFolderPath));
			svCheck(animator_initialize());
			svCheck(window_initialize(desc.windowStyle, desc.windowBounds, desc.windowTitle, desc.iconFilePath));
			svCheck(graphics_initialize());
			svCheck(matsys_initialize());
			svCheck(debug_renderer_initialize());
			svCheck(sprite_renderer_initialize());

			// APPLICATION
			svCheck(g_App.initialize());
		}
		svCatch;

		SV_LOG_INFO("Initialized successfuly");
		SV_LOG_SEPARATOR();

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

			// Update animations
			animator_update(g_DeltaTime);

			// Update assets
			asset_update(g_DeltaTime);

			// Update User
			g_App.update(g_DeltaTime);

			// Begin Rendering
			graphics_begin();

			// Before rendering, update materials
			matsys_update();

			// User Rendering
			g_App.render();

			// End frame
			graphics_end();
		}
		svCatch;

		return Result_Success;
	}

	Result engine_close()
	{
		SV_LOG_SEPARATOR();
		SV_LOG_INFO("Closing %s", g_Name.c_str());

		// APPLICATION
		try {
			svCheck(g_App.close());

			asset_free_unused();
			svCheck(sprite_renderer_close());
			svCheck(debug_renderer_close());
			svCheck(matsys_close());
			svCheck(graphics_close());
			svCheck(window_close());
			svCheck(animator_close());
			svCheck(asset_close());
			svCheck(event_close());
			svCheck(task_close());
			svCheck(logging_close());
		}
		svCatch;

		return Result_Success;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime;	}

	ui64 engine_stats_get_frame_count() noexcept { return g_FrameCount; }
	
}