#pragma once

#include "graphics.h"

namespace sv {

	// Render Layers

	enum RenderLayerSortMode : ui32 {
		RenderLayerSortMode_none,
		RenderLayerSortMode_coordX,
		RenderLayerSortMode_coordY,
		RenderLayerSortMode_coordZ,
	};

	void				renderLayer_count_set(ui32 count);
	ui32				renderLayer_count_get();
	void				renderLayer_sortMode_set(ui32 layer, RenderLayerSortMode sortMode);
	RenderLayerSortMode	renderLayer_sortMode_get(ui32 layer);

	// Texture Atlas

	typedef void* TextureAtlas;

	struct Sprite {
		TextureAtlas	textureAtlas;
		ui32			index;
	};

	bool			textureAtlas_create_from_file(const char* filePath, bool linearFilter, SamplerAddressMode addressMode, TextureAtlas* pTextureAtlas);
	bool			textureAtlas_destroy(TextureAtlas textureAtlas);
	Sprite			textureAtlas_sprite_add(TextureAtlas textureAtlas, float x, float y, float w, float h);
	ui32			textureAtlas_sprite_count(TextureAtlas textureAtlas);
	vec4			textureAtlas_sprite_get(TextureAtlas textureAtlas, ui32 index);

}