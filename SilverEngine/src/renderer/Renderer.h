#pragma once

#include "core.h"
#include "Camera.h"
#include "renderer_desc.h"
#include "graphics/graphics.h"

struct SV_RENDERER_INITIALIZATION_DESC {
	ui32 resolutionWidth;
	ui32 resolutionHeight;
};

namespace sv {
	class Scene;
}

// Main Functions (Internal)
namespace _sv {

	struct DrawData;

	bool renderer_initialize(const SV_RENDERER_INITIALIZATION_DESC& desc);
	bool renderer_close();
	void renderer_frame_begin();
	void renderer_frame_end();

	sv::Image& renderer_offscreen_get();
	DrawData& renderer_drawdata_get();

}

// Main Functions
namespace sv {

	void renderer_resolution_set(ui32 width, ui32 height);
	uvec2 renderer_resolution_get() noexcept;
	ui32 renderer_resolution_get_width() noexcept;
	ui32 renderer_resolution_get_height() noexcept;
	float renderer_resolution_get_aspect() noexcept;

	void renderer_scene_begin();
	void renderer_scene_end();
	void renderer_draw_scene(Scene& scene);
	void renderer_present(Camera& camera);

}

namespace _sv {

	// PostProcessing

	bool renderer_postprocessing_initialize();
	bool renderer_postprocessing_close();

	struct PostProcessing_Default {
		sv::RenderPass renderPass;
	};

	bool renderer_postprocessing_default_create(SV_GFX_FORMAT dstFormat, SV_GFX_IMAGE_LAYOUT initialLayout, SV_GFX_IMAGE_LAYOUT finalLayout, PostProcessing_Default& pp);
	bool renderer_postprocessing_default_destroy(PostProcessing_Default& pp);
	void renderer_postprocessing_default_render(PostProcessing_Default& pp, sv::Image& src, sv::Image& dst, sv::CommandList cmd);

	// Render Layer (2D)

	bool renderer_layer_initialize();
	bool renderer_layer_close();
	void renderer_layer_begin();
	void renderer_layer_end();
	void renderer_layer_prepare_scene(sv::Scene& scene);
	void renderer_layer_render(sv::CommandList cmd);

}

namespace sv {

	RenderLayerID renderer_layer_create(i16 sortValue, SV_REND_SORT_MODE sortMode);
	i16 renderer_layer_get_sort_value(RenderLayerID renderLayer);
	SV_REND_SORT_MODE renderer_layer_get_sort_mode(RenderLayerID renderLayer);
	void renderer_layer_set_sort_value(i16 value, RenderLayerID renderLayer);
	void renderer_layer_set_sort_mode(SV_REND_SORT_MODE mode, RenderLayerID renderLayer);
	void renderer_layer_destroy(RenderLayerID renderLayer);

}
	
