#pragma once

#include "renderer2D.h"

namespace sv {

	struct SpriteInstance {
		XMMATRIX tm;
		Sprite sprite;
		Color color;
		ui32 layerID;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, Sprite sprite, sv::Color color, ui32 layerID) : tm(m), sprite(sprite), color(color), layerID(layerID) {}
	};

	bool renderer2D_initialize();
	bool renderer2D_close();
	void renderer2D_sprite_render(SpriteInstance* buffer, ui32 count, sv::CommandList cmd);

	struct RenderLayer {
		RenderLayerSortMode		sortMode;
		bool					transparent;
		ui32					count;
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