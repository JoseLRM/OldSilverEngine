#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	struct Sprite {
		TextureAsset	texture;
		v4_f32			texCoord;
	};

	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texcoord;
		GPUImage*	image;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color)
			: tm(m), texcoord(texcoord), image(image), color(color) {}
	};

	struct EmissiveSpriteInstance {
		XMMATRIX	tm;
		v4_f32		texcoord;
		GPUImage*	diffuse_image;
		GPUImage*	emission_image;
		Color		diffuse_color;
		Color		emission_color;

		EmissiveSpriteInstance() = default;
		EmissiveSpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* diffuse_image, Color diffuse_color, GPUImage* emission_image, Color emission_color)
			: tm(m), texcoord(texcoord), diffuse_image(diffuse_image), diffuse_color(diffuse_color), emission_image(emission_image), emission_color(emission_color) {}
	};

	void draw_sprites(const SpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, GPUImage* offscreen, CommandList cmd);
	void draw_sprites_emissive(const EmissiveSpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, GPUImage* offscreen, GPUImage* emissive, CommandList cmd);

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