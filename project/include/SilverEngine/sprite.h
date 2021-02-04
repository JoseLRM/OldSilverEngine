#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

    struct Sprite {
	TextureAsset texture;
	v4_f32       texcoord;
    };
    
    struct SpriteAnimationInfo {
	
	std::vector<Sprite> sprites;
	GPUImage*           image       = nullptr;
	f32                 sprite_time = 0.1f;
	
    };
    
    struct SpriteAnimation {

	u32 index = 0u;
	f32 current_time = 0.f;
	
    };

    SV_INLINE void restet_sprite_animation(SpriteAnimation& anim)
    {
	anim.index = 0u;
	anim.current_time = 0.f;
    }

    SV_INLINE void update_sprite_animation(SpriteAnimation& anim, const SpriteAnimationInfo& info, f32 dt)
    {
	anim.current_time += dt;

	while (anim.current_time >= info.sprite_time) {

	    anim.current_time -= info.sprite_time;
	    anim.index = (anim.index + 1u) % u32(info.sprites.size());
	}
    }

    // TEXTURE ATLAS UTILS

    SV_INLINE v4_f32 texcoord_from_atlas(u32 num_columns, u32 num_rows, u32 index)
    {
	const f32 xOffset = 1.f / f32(num_columns);
	const f32 yOffset = 1.f / f32(num_rows);

	const f32 xIndex = f32(index % num_columns);
	const f32 yIndex = f32(index / num_columns);

	f32 u0 = xOffset * xIndex;
	f32 v0 = yOffset * yIndex;
	f32 u1 = u0 + xOffset;
	f32 v1 = v0 + yOffset;

	return { u0, v0, u1, v1 };
    }

}
