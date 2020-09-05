#pragma once

#include "scene.h"

#define parseScene() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_);

namespace sv {

	struct Scene_internal {

		ECS*	ecs;
		Entity	mainCamera;
		float	timeStep;
		void*	pWorld2D;

		struct {
			ui32 refreshCount;
			std::vector<std::pair<std::string, SharedRef<Texture>>> textures;
		} assets;

	};

	Result scene_initialize(const InitializationSceneDesc& desc);
	Result scene_close();

	Result scene_assets_initialize(const char* assetsFolderPath);
	Result scene_assets_close();

	Result scene_assets_create(const SceneDesc* desc, Scene_internal& scene);
	Result scene_assets_destroy(Scene_internal& scene);

	Result scene_physics_create(const SceneDesc* desc, Scene_internal& scene);
	Result scene_physics_destroy(Scene_internal& scene);

	// Serialize Components Functions

	void scene_component_serialize_NameComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_SpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_CameraComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_RigidBody2DComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_QuadComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_MeshComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_LightComponent(BaseComponent* comp, ArchiveO& archive);

	// Deserialize Components Functions

	void scene_component_deserialize_NameComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_SpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_CameraComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_RigidBody2DComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_QuadComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_MeshComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_LightComponent(BaseComponent* comp, ArchiveI& archive);

}