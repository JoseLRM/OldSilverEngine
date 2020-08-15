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
		vec3 position;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, Sprite sprite, sv::Color color, ui32 layerID, vec3 position) : tm(m), sprite(sprite), color(color), layerID(layerID), position(position) {}
	};

	struct RenderLayer {
		RenderLayerSortMode		sortMode;
	};

	struct TextureAtlas_internal {
		GPUImage			image;
		Sampler				sampler;
		std::vector<vec4>	sprites;
	};

	bool renderer2D_initialize();
	bool renderer2D_close();
	void renderer2D_scene_draw_sprites(CommandList cmd);

	void renderer2D_sprite_sort(SpriteInstance* buffer, ui32 count, const RenderLayer& renderLayer);
	void renderer2D_sprite_render(SpriteInstance* buffer, ui32 count, const XMMATRIX& viewProjectionMatrix, GPUImage& renderTarget, CommandList cmd);

}