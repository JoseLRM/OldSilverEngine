#pragma once

#include "graphics.h"

namespace sv {

	struct Offscreen {
		GPUImage renderTarget;
		GPUImage depthStencil;
		inline Viewport GetViewport() const noexcept { return graphics_image_get_viewport(renderTarget); }
		inline Scissor GetScissor() const noexcept { return graphics_image_get_scissor(renderTarget); }
		inline ui32 GetWidth() const noexcept { return graphics_image_get_width(renderTarget); }
		inline ui32 GetHeight() const noexcept { return graphics_image_get_height(renderTarget); }
	};

	Result renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen);
	Result renderer_offscreen_destroy(Offscreen& offscreen);

}