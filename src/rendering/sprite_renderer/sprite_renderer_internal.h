#pragma once

#include "rendering/sprite_renderer.h"
#include "utils/allocator.h"

namespace sv {

	constexpr u32 SPRITE_BATCH_COUNT = 1000u;
	constexpr u32 SPRITE_LIGHT_COUNT = 10u;

	struct SpriteVertex {
		vec4f position;
		vec2f texCoord;
		Color color;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	struct SpriteLight {
		u32		type;
		vec3f	position;
		Color3f	color;
		float	range;
		float	intensity;
		float	smoothness;
		vec2f	padding;
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