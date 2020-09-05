#include "core.h"

#include "scene_internal.h"
#include "engine.h"

namespace sv {

	Result scene_initialize(const InitializationSceneDesc& desc)
	{
		ecs_register<NameComponent>("NameComponent"					, scene_component_serialize_NameComponent			, scene_component_deserialize_NameComponent);
		ecs_register<SpriteComponent>("SpriteComponent"				, scene_component_serialize_SpriteComponent			, scene_component_deserialize_SpriteComponent);
		ecs_register<CameraComponent>("CameraComponent"				, scene_component_serialize_CameraComponent			, scene_component_deserialize_CameraComponent);
		ecs_register<RigidBody2DComponent>("RigidBody2DComponent"	, scene_component_serialize_RigidBody2DComponent	, scene_component_deserialize_RigidBody2DComponent);
		ecs_register<QuadComponent>("QuadComponent"					, scene_component_serialize_QuadComponent			, scene_component_deserialize_QuadComponent);
		ecs_register<MeshComponent>("MeshComponent"					, scene_component_serialize_MeshComponent			, scene_component_deserialize_MeshComponent);
		ecs_register<LightComponent>("LightComponent"				, scene_component_serialize_LightComponent			, scene_component_deserialize_LightComponent);

		svCheck(scene_assets_initialize(desc.assetsFolderPath));

		return Result_Success;
	}

	Result scene_close()
	{
		svCheck(scene_assets_close());

		return Result_Success;
	}

	Result scene_create(const SceneDesc* desc, Scene** scene_)
	{
		Scene_internal& scene = *new Scene_internal();

		scene.timeStep = 1.f;

		// Initialize Entity Component System
		ecs_create(&scene.ecs);

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

		*scene_ = &scene;
		return Result_Success;
	}

	Result scene_destroy(Scene* scene_)
	{
		if (scene_ == nullptr) return Result_Success;
		parseScene();
		ecs_destroy(scene.ecs);
		scene.mainCamera = SV_ENTITY_NULL;
		svCheck(scene_physics_destroy(scene));
		svCheck(scene_assets_destroy(scene));
		delete &scene;
		return Result_Success;
	}

	void scene_clear(Scene* scene_)
	{
		parseScene();
		ecs_clear(scene.ecs);
		scene.mainCamera = SV_ENTITY_NULL;
	}

	Result scene_serialize(Scene* scene_, const char* filePath)
	{
		parseScene();
		ArchiveO archive;
		
		archive << engine_version_get();

		svCheck(ecs_serialize(scene.ecs, archive));

		// Save
#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif
		archive.save_file(filePath, false);

		return Result_Success;
	}

	Result scene_deserialize(Scene* scene_, const char* filePath)
	{
		ArchiveI archive;

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		svCheck(archive.open_file(filePath));

		parseScene();

		// Version
		{
			Version version;
			archive >> version;

			if (version < SCENE_MINIMUM_SUPPORTED_VERSION) return Result_UnsupportedVersion;
		}

		svCheck(ecs_deserialize(scene.ecs, archive));

		return Result_Success;
	}

	ECS* scene_ecs_get(Scene* scene_)
	{
		parseScene();
		return scene.ecs;
	}

	void scene_camera_set(Scene* scene_, Entity camera)
	{
		parseScene();
		scene.mainCamera = camera;
	}

	Entity scene_camera_get(Scene* scene_)
	{
		parseScene();
		return scene.mainCamera;
	}

}