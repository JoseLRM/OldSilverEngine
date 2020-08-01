#pragma once

#include "core.h"
#include "Camera.h"
#include "renderer_desc.h"
#include "graphics/graphics.h"
#include "renderer_postprocessing.h"
#include "renderer_layer.h"

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

	bool renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen);
	bool renderer_offscreen_destroy(Offscreen& offscreen);

	Offscreen& renderer_offscreen_get();

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
	void renderer_present(Camera& camera, XMMATRIX worldMatrix);

}

