#pragma once

#include "sprite/sprite_renderer.h"
#include "core/utils/allocator.h"

namespace sv {

	constexpr u32 SPRITE_BATCH_COUNT = 1000u;
	constexpr u32 SPRITE_LIGHT_COUNT = 10u;

	struct SpriteVertex {
		v4_f32 position;
		v2_f32 texCoord;
		Color color;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	struct SpriteLight {
		u32		type;
		v3_f32	position;
		Color3f	color;
		float	range;
		float	intensity;
		float	smoothness;
		v2_f32	padding;
	};

	struct SpriteLightData {
		Color3f ambient;
		u32 lightCount;
		SpriteLight lights[SPRITE_LIGHT_COUNT];
	};

	struct SpriteRendererContext {

		SpriteData* spriteData;
		SpriteLightData* lightData;

	};

	struct SpriteRenderer_internal {

		static Result initialize();
		static Result close();

	};

}