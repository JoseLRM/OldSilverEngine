#pragma once

#include "rendering/sprite_renderer.h"

namespace sv {

	constexpr ui32 SPRITE_BATCH_COUNT = 1000u;

	struct SpriteVertex {
		vec4f position;
		vec2f texCoord;
		Color color;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	struct SpriteRendererContext {

		CameraBuffer* pCameraBuffer;
		GPUImage* renderTarget;
		GPUImage* depthStencil;
		SpriteData* spriteData;

	};

	struct SpriteRenderer_internal {

		static Result initialize();
		static Result close();

	};

}