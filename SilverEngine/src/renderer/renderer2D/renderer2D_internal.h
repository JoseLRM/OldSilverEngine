#pragma once

#include "renderer2D.h"

namespace sv {

	struct SpriteAABB {
		vec3 position;
		vec2 size;
	};

	struct SpriteInstance {
		XMMATRIX tm;
		Sprite sprite;
		Color color;
		ui32 layerID;
		SpriteAABB boundingBox;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, Sprite sprite, sv::Color color, ui32 layerID, vec3 pos, vec2 size) : tm(m), sprite(sprite), color(color), layerID(layerID), boundingBox({ pos, size }) {}
	};

	bool renderer2D_initialize();
	bool renderer2D_close();
	void renderer2D_sprite_render(SpriteInstance* buffer, ui32 count, sv::CommandList cmd);

	struct RenderLayer {
		ui32					count;
		RenderLayerSortMode		sortMode;
	};

	void renderer2D_begin();
	void renderer2D_prepare_scene();
	void renderer2D_end();

	struct TextureAtlas_internal {
		GPUImage			image;
		Sampler				sampler;
		std::vector<vec4>	sprites;
	};

}