#pragma once

#include "sprite/sprite.h"
#include "core/utils/allocator.h"

namespace sv {

	constexpr u32 SPRITE_BATCH_COUNT = 1000u;

	struct SpriteVertex {
		v4_f32 position;
		v2_f32 texCoord;
		Color color;
		Color emissionColor;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	struct SpriteRendererContext {

		SpriteData* spriteData;

	};

	Result sprite_module_initialize();
	Result sprite_module_close();

}