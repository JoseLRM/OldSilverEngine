#include "core.h"

#include "scene_internal.h"

namespace sv {

	Result scene_create(const SceneDesc* desc, Scene& scene)
	{
		scene.timeStep = 1.f;

		// Initialize Entity Component System
		scene.ecs = ecs_create();

		// Create main camera
		scene.mainCamera = ecs_entity_create(SV_ENTITY_NULL, scene.ecs);
		ecs_component_add<CameraComponent>(scene.mainCamera, scene.ecs);

#if SV_SCENE_NAME_COMPONENT
		ecs_component_add<NameComponent>(scene.mainCamera, scene.ecs, "Main Camera");
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