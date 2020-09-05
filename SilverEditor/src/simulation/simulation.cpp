#include "core_editor.h"

#include "viewports/viewport_simulation.h"
#include "simulation.h"
#include "loader.h"
#include "scene.h"

namespace sve {

	static bool g_Running = false;
	static bool g_Paused = false;

	static sv::Scene* g_Scene = nullptr;
	static std::string g_ScenePath;

	sv::Result simulation_initialize(const char* sceneFilePath)
	{
		sv::SceneDesc desc;
		desc.gravity = { 0.f, 20.f, 0.f };

		svCheck(sv::scene_create(&desc, &g_Scene));

		svCheck(sv::scene_deserialize(g_Scene, sceneFilePath));
		g_ScenePath = sceneFilePath;

		return sv::Result_Success;
	}

	sv::Result simulation_close()
	{
		sv::scene_destroy(g_Scene);
		return sv::Result_Success;
	}

	void simulation_update(float dt)
	{
		// Adjust camera
		{
			sv::ECS* ecs = sv::scene_ecs_get(g_Scene);
			sv::CameraComponent& camera = *sv::ecs_component_get<sv::CameraComponent>(ecs, sv::scene_camera_get(g_Scene));

			sv::uvec2 size = viewport_simulation_size();
			camera.Adjust(size.x, size.y);
		}

		sv::scene_assets_update(g_Scene, dt);

		if (g_Running && !g_Paused) {

			sv::scene_physics_simulate(g_Scene, dt);

		}
	}

	void simulation_render()
	{
		if (!simulation_running() && !viewport_simulation_visible()) {
			return;
		}

		sv::scene_renderer_draw(g_Scene);
	}

	void simulation_run()
	{
		if (g_Running) return;

		g_Running = true;
		g_Paused = false;
		ImGui::GetStyle().Alpha = 0.2f;

		sv::scene_serialize(g_Scene, g_ScenePath.c_str());
	}

	void simulation_continue()
	{
		if (!g_Running || !g_Paused) return;
	
		g_Paused = false;
		ImGui::GetStyle().Alpha = 0.2f;
	}

	void simulation_pause()
	{
		if (!g_Running || g_Paused) return;
		
		g_Paused = true;
		ImGui::GetStyle().Alpha = 1.f;
	}

	void simulation_stop()
	{
		if (!g_Running) return;

		g_Running = false;
		g_Paused = false;
		ImGui::GetStyle().Alpha = 1.f;

		sv::scene_deserialize(g_Scene, g_ScenePath.c_str());
	}

	bool simulation_running()
	{
		return g_Running;
	}

	bool simulation_paused()
	{
		return g_Paused;
	}

	sv::Scene* simulation_scene_get()
	{
		return g_Scene;
	}

}