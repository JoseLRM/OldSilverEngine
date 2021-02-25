#include "SilverEngine/core.h"

#include "SilverEngine/scene.h"
#include "SilverEngine/renderer.h"

namespace sv {

	struct Scene_internal : public Scene {

	};

#define PARSE_SCENE() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_)

	Result create_scene(Scene** pscene)
	{
		Scene_internal& scene = *new Scene_internal();

		ecs_create(&scene.ecs);
		scene.gui = gui_create();
		svCheck(offscreen_create(1920u, 1080u, &scene.offscreen));
		svCheck(depthstencil_create(1920u, 1080u, &scene.depthstencil));

		*pscene = reinterpret_cast<Scene*>(&scene);

		return Result_Success;
	}

	Result destroy_scene(Scene* scene_)
	{
		PARSE_SCENE();

		ecs_destroy(scene.ecs);
		gui_destroy(scene.gui);
		graphics_destroy(scene.offscreen);
		graphics_destroy(scene.depthstencil);

		return Result_Success;
	}

	void update_scene(Scene* scene)
	{
		CameraComponent* camera = get_main_camera(scene);

		// Adjust camera
		if (camera) {
			camera->adjust(f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));
		}

		gui_update(scene->gui, window_width_get(engine.window), window_height_get(engine.window));
	}

	CameraComponent* get_main_camera(Scene* scene)
	{
		if (scene->main_camera == SV_ENTITY_NULL || !ecs_entity_exist(scene->ecs, scene->main_camera)) return nullptr;
		return ecs_component_get<CameraComponent>(scene->ecs, scene->main_camera);
	}

	void SpriteComponent::serialize(ArchiveO& archive)
	{
		save_asset(archive, texture);
		archive << texcoord << color << layer;
	}

	void SpriteComponent::deserialize(ArchiveI& archive)
	{
		load_asset(archive, texture);
		archive >> texcoord >> color >> layer;
	}

	void NameComponent::serialize(ArchiveO& archive)
	{
		archive << name;
	}

	void NameComponent::deserialize(ArchiveI& archive)
	{
		archive >> name;
	}

	void CameraComponent::serialize(ArchiveO& archive)
	{
		archive << projection_type << near << far << width << height;
	}

	void CameraComponent::deserialize(ArchiveI& archive)
	{
		archive >> projection_type >> near >> far >> width >> height;
	}

}