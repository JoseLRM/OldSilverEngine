#pragma once

#include "core/rendering/render_utils.h"

namespace sv {

	struct Sprite {
		
		TextureAsset	texture;
		v4_f32			texCoord;
		
	};

	// RENDERING

	// TODO: Emission texture
	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texCoord;
		GPUImage*	image;
		Color		color;
		Color		emissionColor;

		SpriteInstance() = default;
		SpriteInstance(const SpriteInstance& other)
			: tm(other.tm), texCoord(other.texCoord), image(other.image), color(other.color), emissionColor(other.emissionColor) {}
		SpriteInstance(const XMMATRIX& m, const v4_f32& texCoord, GPUImage* image, Color color)
			: tm(m), texCoord(texCoord), image(image), color(color), emissionColor(Color::Black()) {}
		SpriteInstance(const XMMATRIX& m, const v4_f32& texCoord, GPUImage* image, Color color, Color emissionColor)
			: tm(m), texCoord(texCoord), image(image), color(color), emissionColor(emissionColor) {}
	};

	void draw_sprites(const SpriteInstance* pSprites, u32 count, const XMMATRIX& viewProjectionMatrix, GPUImage* offscreen, GPUImage* emissive, CommandList cmd);

	// TEXTURE ATLAS HELPERS

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

	// SPRITE ANIMATION

	//struct AnimatedSprite {
	//
	//
	//
	//};

}