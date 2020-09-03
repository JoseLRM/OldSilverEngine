#include "core.h"

#include "scene_internal.h"

namespace sv {

	Result scene_initialize(const InitializationSceneDesc& desc)
	{
		ecs_register<NameComponent>("NameComponent");
		ecs_register<SpriteComponent>("SpriteComponent");
		ecs_register<CameraComponent>("CameraComponent");
		ecs_register<RigidBody2DComponent>("RigidBody2DComponent");
		ecs_register<QuadComponent>("QuadComponent");
		ecs_register<MeshComponent>("MeshComponent");
		ecs_register<LightComponent>("LightComponent");

		svCheck(scene_assets_initialize(desc.assetsFolderPath));

		return Result_Success;
	}

	Result scene_close()
	{
		svCheck(scene_assets_close());

		return Result_Success;
	}

	Result scene_create(const SceneDesc* desc, Scene& scene)
	{
		scene.timeStep = 1.f;

		// Initialize Entity Component System
		scene.ecs = ecs_create();

		// Create main camera
		scene.mainCamera = ecs_entity_create(scene.ecs);
		ecs_component_add<CameraComponent>(scene.ecs, scene.mainCamera);

#if SV_SCENE_NAME_COMPONENT
		ecs_component_add<NameComponent>(scene.ecs, scene.mainCamera, "Camera");
#endif

		// Assets
		svCheck(scene_assets_create(desc, scene));

		// Physics
		svCheck(scene_physics_create(desc, scene));

		return Result_Success;
	}

	Result scene_destroy(Scene& scene)
	{
		ecs_destroy(scene.ecs);
		scene.mainCamera = SV_ENTITY_NULL;
		svCheck(scene_physics_destroy(scene));
		svCheck(scene_assets_destroy(scene));
		return Result_Success;
	}

	void scene_clear(Scene& scene)
	{
		ecs_clear(scene.ecs);
		scene.mainCamera = SV_ENTITY_NULL;
	}

}