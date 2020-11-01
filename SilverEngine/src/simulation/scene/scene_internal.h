#pragma once

#include "simulation/scene.h"

#define parseScene() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_);

namespace sv {

	// Serialize Components Functions

	void scene_component_serialize_NameComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_SpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_CameraComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_RigidBody2DComponent(BaseComponent* comp, ArchiveO& archive);

	// Deserialize Components Functions

	void scene_component_deserialize_NameComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_SpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_CameraComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_RigidBody2DComponent(BaseComponent* comp, ArchiveI& archive);

}