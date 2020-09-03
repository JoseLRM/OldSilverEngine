#include "core_editor.h"

#include "viewports/viewport_simulation.h"
#include "simulation.h"
#include "loader.h"
#include "scene.h"

namespace sve {

	static bool g_Running = false;
	static bool g_Paused = false;

	static sv::Scene g_Scene;

	sv::Result simulation_initialize()
	{
		simulation_scene_load();
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
			sv::CameraComponent& camera = *sv::ecs_component_get<sv::CameraComponent>(g_Scene.ecs, g_Scene.mainCamera);

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
		g_Running = true;
		g_Paused = false;
		ImGui::GetStyle().Alpha = 0.2f;
	}

	void simulation_continue()
	{
		if (g_Running) {
			g_Paused = false;
			ImGui::GetStyle().Alpha = 0.2f;
		}
	}

	void simulation_pause()
	{
		if (g_Running) {
			g_Paused = true;
			ImGui::GetStyle().Alpha = 1.f;
		}
	}

	void simulation_stop()
	{
		g_Running = false;
		g_Paused = false;
		ImGui::GetStyle().Alpha = 1.f;
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
		sv::SceneDesc desc;
		desc.gravity = { 0.f, 10.f, 0.f };

		sv::Scene scene;
		SV_ASSERT(sv::scene_create(&desc, scene) == sv::Result_Success);

		// Create main camera
		{
			sv::CameraComponent& camera = *sv::ecs_component_get<sv::CameraComponent>(scene.ecs, scene.mainCamera);
			camera.settings.projection.cameraType = sv::CameraType_Orthographic;
			camera.settings.projection.width = 10.f;
			camera.settings.projection.height = 10.f;
		}

		// Temporal
		//sv::loader_model_import("assets/gobber/GoblinX.obj", g_Model);
		//
		//sv::Entity meshEntity = sv::ecs_entity_create(SV_ENTITY_NULL, scene.ecs);
		//sv::ecs_component_add<sv::MeshComponent>(meshEntity, scene.ecs);
		//sv::ecs_component_get<sv::MeshComponent>(meshEntity, scene.ecs)->mesh = &g_Model.nodes[0].mesh;
		//sv::ecs_component_get<sv::MeshComponent>(meshEntity, scene.ecs)->material = &g_Model.materials[0];

		//sv::SharedRef<sv::Texture> texture;
		//sv::scene_assets_load_texture(scene, "textures/Tileset.png", texture);
		//if (texture.Get()) {
		//
		//	sv::Entity spriteEntity = sv::ecs_entity_create(0, scene.ecs);
		//	sv::ecs_component_add<sv::SpriteComponent>(spriteEntity, scene.ecs);
		//	sv::SpriteComponent* spr = sv::ecs_component_get<sv::SpriteComponent>(spriteEntity, scene.ecs);
		//	spr->sprite.index = texture->AddSprite(0.f, 0.f, 0.1f, 0.1f);
		//	spr->sprite.texture = texture;
		//
		//}
		//
		return scene;
	}

	void simulation_scene_load()
	{
		// TODO: Filepath in parameters and deserialize the scene
		sv::scene_destroy(g_Scene);
		g_Scene = simulation_scene_create_default();
	}

	sv::Scene& simulation_scene_get()
	{
		return g_Scene;
	}

}