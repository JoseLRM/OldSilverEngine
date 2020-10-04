#pragma once

#include "scene.h"

#define svThrow(x) SV_THROW("SCENE_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Green | sv::LoggingStyle_Red, "[SCENE] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Green | sv::LoggingStyle_Red, "[SCENE_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[SCENE_ERROR] "#x, __VA_ARGS__)

#define parseScene() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_);

namespace sv {

	struct Scene_internal {

		ECS*	ecs;
		Entity	mainCamera;
		float	timeStep;
		void*	pWorld2D;

	};

	Result scene_initialize();
	Result scene_close();

	Result scene_physics_create(const SceneDesc* desc, Scene_internal& scene);
	Result scene_physics_destroy(Scene_internal& scene);

	// Serialize Components Functions

	void scene_component_serialize_NameComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_SpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_CameraComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_RigidBody2DComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_MeshComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_LightComponent(BaseComponent* comp, ArchiveO& archive);

	// Deserialize Components Functions

	void scene_component_deserialize_NameComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_SpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_CameraComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_RigidBody2DComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_MeshComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_LightComponent(BaseComponent* comp, ArchiveI& archive);

}