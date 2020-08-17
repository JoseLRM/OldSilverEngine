#include "core_editor.h"

#include "simulation.h"
#include "components.h"
#include "viewport_manager.h"
#include "loader.h"

namespace sve {

	static bool g_Running = false;
	static bool g_Paused = false;

	static sv::Scene g_Scene = 0;

	bool simulation_initialize()
	{
		simulation_scene_load();
		return true;
	}

	bool simulation_close()
	{
		sv::scene_destroy(g_Scene);
		return true;
	}

	void simulation_update(float dt)
	{
		auto props = viewport_manager_properties_get("Game");

		// Adjust camera
		{
			sv::Entity cameraEntity = sv::scene_camera_get();
			sv::CameraComponent& camera = *sv::scene_ecs_component_get<sv::CameraComponent>(cameraEntity);

			camera.Adjust(props.width, props.height);
		}

	}

	void simulation_render()
	{
		sv::renderer_scene_render(false);
	}

	void simulation_run()
	{
	}

	void simulation_continue()
	{
	}

	void simulation_pause()
	{
	}

	void simulation_stop()
	{
	}

	bool simulation_running()
	{
		return g_Running;
	}

	bool simulation_paused()
	{
		return g_Paused;
	}

	// TEMP
	static sv::Model g_Model;

	sv::Scene simulation_scene_create_default()
	{
		// Create scene
		sv::Scene scene = sv::scene_create();
		sv::scene_bind(scene);

		// Create main camera
		{
			sv::Entity mainCamera = sv::scene_ecs_entity_create();
			sv::scene_ecs_component_add<sv::CameraComponent>(mainCamera);
			sv::scene_camera_set(mainCamera);

			sv::CameraComponent& camera = *sv::scene_ecs_component_get<sv::CameraComponent>(mainCamera);
			camera.projection.cameraType = sv::CameraType_Orthographic;
			camera.projection.width = 10.f;
			camera.projection.height = 10.f;
		}

		// Temporal
		sv::loader_model_import("assets/dragon.obj", g_Model);


		sv::Entity meshEntity = sv::scene_ecs_entity_create();
		sv::scene_ecs_component_add<sv::MeshComponent>(meshEntity);
		sv::scene_ecs_component_get<sv::MeshComponent>(meshEntity)->mesh = &g_Model.nodes[0].mesh;
		sv::scene_ecs_component_get<sv::MeshComponent>(meshEntity)->material = &g_Model.materials[0];

		return scene;
	}

	void simulation_scene_load()
	{
		// TODO: Filepath in parameters and deserialize the scene
		sv::scene_destroy(g_Scene);
		g_Scene = simulation_scene_create_default();
		sv::scene_bind(g_Scene);
	}

	sv::Scene simulation_scene_get()
	{
		return g_Scene;
	}

}