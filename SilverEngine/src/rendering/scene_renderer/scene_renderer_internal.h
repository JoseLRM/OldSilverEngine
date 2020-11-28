#pragma once

#include "rendering/scene_renderer.h"
#include "rendering/sprite_renderer.h"
#include "utils/allocator.h"

namespace sv {

	void scene_component_serialize_SpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveO& archive);
	void scene_component_serialize_CameraComponent(BaseComponent* comp, ArchiveO& archive);

	void scene_component_deserialize_SpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_AnimatedSpriteComponent(BaseComponent* comp, ArchiveI& archive);
	void scene_component_deserialize_CameraComponent(BaseComponent* comp, ArchiveI& archive);

	struct SceneRenderer_internal {
		
		static Result initialize();
		static Result close();
		
	};

	struct SpriteIntermediate {

		SpriteInstance instance;
		Material* material;
		float depth;

		SpriteIntermediate() = default;
		SpriteIntermediate(const XMMATRIX& m, const vec4f& texCoord, GPUImage* pTex, Color color, Material* material, float depth)
			: instance(m, texCoord, pTex, color), material(material), depth(depth) {}

	};

	// Temporal data created while rendering
	struct SceneRendererTemp {

		CameraBuffer cameraBuffer;

		// Allocators used to draw the scene without allocate memory every frame
		FrameList<SpriteIntermediate>	spritesIntermediates[SceneRenderer::RENDER_LAYER_COUNT];
		FrameList<SpriteInstance>		spritesInstances;

	};

}