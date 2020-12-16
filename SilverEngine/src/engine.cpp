#include "core.h"

#include "engine.h"
#include "core_internal.h"
#include "rendering/material_system/material_system_internal.h"
#include "rendering/debug_renderer/debug_renderer_internal.h"
#include "rendering/sprite_renderer/sprite_renderer_internal.h"
#include "rendering/mesh_renderer/mesh_renderer_internal.h"
#include "rendering/scene_renderer/scene_renderer_internal.h"
#include "rendering/postprocessing/postprocessing_internal.h"
#include "rendering/render_utils/render_utils_internal.h"
#include "platform/graphics/graphics_internal.h"
#include "platform/input/input_internal.h"
#include "platform/window/window_internal.h"
#include "simulation/task_system/task_system_internal.h"
#include "simulation/asset_system/asset_system_internal.h"
#include "simulation/event_system/event_system_internal.h"
#include "simulation/sprite_animator/sprite_animator_internal.h"
#include "simulation/scene/scene_internal.h"
#include "simulation/model/model_internal.h"

namespace sv {

#define SV_CATCH catch (std::exception e) {\
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

	static ApplicationCallbacks		g_App;
	static constexpr Version		g_Version = { 0u, 0u, 0u };
	static std::string				g_Name;

	static float		g_DeltaTime = 0.f;
	static u64			g_FrameCount = 0u;

	static bool g_EnableAnimations = true;

	// Initialization


	Result engine_initialize(const InitializationDesc* d)
	{
		if (d == nullptr) return Result_InvalidUsage;

		// Initialization Parameters
		const InitializationDesc& desc = *d;
		g_App = desc.callbacks;

		g_Name = "SilverEngine ";
		g_Name += g_Version.toString();

		// SYSTEMS
		try {

			svCheck(logging_initialize());

			SV_LOG_CLEAR();
			SV_LOG_INFO("Initializing %s", g_Name.c_str());

			svCheck(task_initialize(desc.minThreadsCount));
			svCheck(event_initialize());
			svCheck(asset_initialize(desc.assetsFolderPath));
			svCheck(window_initialize(desc.windowStyle, desc.windowBounds, desc.windowTitle, desc.iconFilePath));
			svCheck(graphics_initialize());
			svCheck(sprite_animator_initialize());
			svCheck(matsys_initialize());
			svCheck(model_initialize());
			svCheck(render_utils_initialize());

			// Renderers
			svCheck(DebugRenderer_internal::initialize());
			svCheck(PostProcessing_internal::initialize());
			svCheck(SpriteRenderer_internal::initialize());
			svCheck(MeshRenderer_internal::initialize());
			svCheck(SceneRenderer_internal::initialize());

			svCheck(scene_initialize());

			// APPLICATION
			svCheck(g_App.initialize());
		}
		SV_CATCH;

		SV_LOG_INFO("Initialized successfuly");
		SV_LOG_SEPARATOR();

		return Result_Success;
	}

	Result engine_loop()
	{
		static Time		lastTime = 0.f;

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
			if (g_EnableAnimations) {
				sprite_animator_update(g_DeltaTime);
			}

			// Update assets
			asset_update(g_DeltaTime);

			// Update User
			g_App.update(g_DeltaTime);

			// Update render utils
			render_utils_update(g_DeltaTime);

			// Begin Rendering
			graphics_begin();

			// Before rendering, update materials
			matsys_update();

			// User Rendering
			g_App.render();

			// End frame
			graphics_end();
		}
		SV_CATCH;

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

			if (result_fail(scene_close())) { SV_LOG_ERROR("Can't close the scene system"); }

			// Renderers
			if (result_fail(SceneRenderer_internal::close())) { SV_LOG_ERROR("Can't close Scene Renderer"); }
			if (result_fail(MeshRenderer_internal::close())) { SV_LOG_ERROR("Can't close the mesh renderer"); }
			if (result_fail(SpriteRenderer_internal::close())) { SV_LOG_ERROR("Can't close the sprite renderer"); }
			if (result_fail(PostProcessing_internal::close())) { SV_LOG_ERROR("Can't close the postprocessing"); }
			if (result_fail(DebugRenderer_internal::close())) { SV_LOG_ERROR("Can't close the debug renderer"); }

			if (result_fail(render_utils_close())) { SV_LOG_ERROR("Can't close the render utils"); }
			if (result_fail(matsys_close())) { SV_LOG_ERROR("Can't close the material system"); }
			if (result_fail(model_close())) { SV_LOG_ERROR("Can't close the model system"); }
			if (result_fail(sprite_animator_close())) { SV_LOG_ERROR("Can't close the sprite animator"); }
			if (result_fail(graphics_close())) { SV_LOG_ERROR("Can't close graphics"); }
			if (result_fail(window_close())) { SV_LOG_ERROR("Can't close the window"); }
			if (result_fail(asset_close())) { SV_LOG_ERROR("Can't close the asset system"); }
			if (result_fail(event_close())) { SV_LOG_ERROR("Can't close the event system"); }
			if (result_fail(task_close())) { SV_LOG_ERROR("Can't close the task system"); }
			logging_close();
		}
		SV_CATCH;

		return Result_Success;
	}

	Version engine_version_get() noexcept { return g_Version; }
	float engine_deltatime_get() noexcept { return g_DeltaTime; }

	u64 engine_frame_count() noexcept { return g_FrameCount; }

	void engine_animations_enable()
	{
		g_EnableAnimations = true;
	}

	void engine_animations_disable()
	{
		g_EnableAnimations = false;
	}

	bool engine_animations_is_enabled()
	{
		return g_EnableAnimations;
	}

}