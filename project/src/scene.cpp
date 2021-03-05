#include "SilverEngine/core.h"

#include "SilverEngine/scene.h"
#include "SilverEngine/renderer.h"

namespace sv {

	struct Scene_internal : public Scene {



	};

#define PARSE_SCENE() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_)

	Result initialize_scene(Scene** pscene, const char* name)
	{
		Scene_internal& scene = *new Scene_internal();

		ecs_create(&scene.ecs);
		svCheck(offscreen_create(1920u, 1080u, &scene.offscreen));
		svCheck(depthstencil_create(1920u, 1080u, &scene.depthstencil));

		scene.name = name;

		*pscene = reinterpret_cast<Scene*>(&scene);

		svCheck(engine.app_callbacks.initialize_scene(*pscene, 0));

		return Result_Success;
	}

	Result close_scene(Scene* scene_)
	{
		PARSE_SCENE();

		svCheck(engine.app_callbacks.close_scene(scene_));

		ecs_destroy(scene.ecs);
		graphics_destroy(scene.offscreen);
		graphics_destroy(scene.depthstencil);

		return Result_Success;
	}

	Result set_active_scene(const char* name)
	{
		if (engine.app_callbacks.validate_scene) {
			Result res = engine.app_callbacks.validate_scene(name);

			if (result_fail(res)) return res;
		}

		engine.next_scene_name = name;
		return Result_Success;
	}

	Result serialize_scene(Scene* scene, const char* filepath)
	{
		ArchiveO archive;
		//
		//archive << engine.version;
		//archive << name;
		//
		//// ECS
		//
		//archive << main_camera;
		//
		return archive.save_file(filepath);
	}

	void update_scene(Scene* scene)
	{
		CameraComponent* camera = get_main_camera(scene);

		// Adjust camera
		if (camera) {
			camera->adjust(f32(window_width_get(engine.window)) / f32(window_height_get(engine.window)));
		}

		engine.app_callbacks.update();
	}

	CameraComponent* get_main_camera(Scene* scene)
	{
		if (scene->main_camera == SV_ENTITY_NULL || !ecs_entity_exist(scene->ecs, scene->main_camera)) return nullptr;
		return ecs_component_get<CameraComponent>(scene->ecs, scene->main_camera);
	}

	const char* get_entity_name(ECS* ecs, Entity entity)
	{
		if (!ecs_entity_exist(ecs, entity)) return nullptr;
		
		const char* name = "Unnamed";

		NameComponent* comp = ecs_component_get<NameComponent>(ecs, entity);
		if (comp && comp->name.size()) name = comp->name.c_str();

		return name;
	}

	Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera)
	{
		// Screen to clip space
		position *= 2.f;

		XMMATRIX ivm = XMMatrixInverse(0, camera_view_matrix(camera_position, camera_rotation, *camera));
		XMMATRIX ipm = XMMatrixInverse(0, camera_projection_matrix(*camera));

		XMVECTOR mouse_world = XMVectorSet(position.x, position.y, 1.f, 1.f);
		mouse_world = XMVector4Transform(mouse_world, ipm);
		mouse_world = XMVectorSetZ(mouse_world, 1.f);
		mouse_world = XMVector4Transform(mouse_world, ivm);
		mouse_world = XMVector3Normalize(mouse_world);

		Ray ray;
		ray.origin = camera_position;
		ray.direction = v3_f32(mouse_world);
		return ray;
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
