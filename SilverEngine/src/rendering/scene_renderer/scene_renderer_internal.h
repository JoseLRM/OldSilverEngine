#pragma once

#include "rendering/scene_renderer.h"
#include "rendering/sprite_renderer.h"
#include "utils/allocator.h"

namespace sv {

	struct SpriteIntermediate {

		SpriteInstance	instance;
		Material* material;

		SpriteIntermediate() = default;
		SpriteIntermediate(const XMMATRIX& m, const vec4f& texCoord, GPUImage* pTex, Color color, Material* material)
			: instance(m, texCoord, pTex, color), material(material) {}

	};

	struct SceneRenderer_internal {

		CameraBuffer cameraBuffer;

		// Allocators used to draw the scene without allocate memory every frame
		FrameList<SpriteIntermediate>	spritesIntermediates;
		FrameList<SpriteInstance>		spritesInstances;

	};

}