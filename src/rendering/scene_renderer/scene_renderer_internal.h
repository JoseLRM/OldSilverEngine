#pragma once

#include "rendering/scene_renderer.h"
#include "rendering/sprite_renderer.h"
#include "rendering/mesh_renderer.h"
#include "utils/allocator.h"

namespace sv {

	void scene_component_serialize_SpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_MeshComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_LightComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_SkyComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_CameraComponent(BaseComponent* comp, ArchiveO& archive);

	void scene_component_deserialize_SpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_MeshComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_LightComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_SkyComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_CameraComponent(BaseComponent* comp, ArchiveI& archive);

	struct SceneRenderer_internal {
		
		static Result initialize();
		static Result close();

	};

	struct SceneRendererContext {

		FrameList<SpriteInstance>		spritesInstances;
		
		FrameList<MeshInstance>			meshInstances;
		FrameList<MeshInstanceGroup>	meshGroups;

	};

}