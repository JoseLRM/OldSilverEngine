#include "core.h"

#include "renderer_internal.h"
#include "graphics/graphics_internal.h"

namespace sv {

	// MAIN FUNCTIONS

	Result renderer_initialize(const InitializationRendererDesc& desc)
	{
		svCheck(renderer_postprocessing_initialize());
		svCheck(renderer_sprite_initialize());
		svCheck(renderer_mesh_initialize());
		svCheck(renderer_debug_initialize());

		return Result_Success;
	}

	Result renderer_close()
	{
		svCheck(renderer_mesh_close());
		svCheck(renderer_postprocessing_close());
		svCheck(renderer_sprite_close());
		svCheck(renderer_debug_close());

		return Result_Success;
	}

	void renderer_frame_begin()
	{
		graphics_begin();
	}
	void renderer_frame_end()
	{
		graphics_commandlist_submit();
		graphics_present();
	}

	void renderer_present(GPUImage& image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd)
	{
		GPUImage& backBuffer = graphics_swapchain_acquire_image();

		GPUImageBlit blit;
		blit.srcRegion = region;
		blit.dstRegion.size = { graphics_image_get_width(backBuffer), graphics_image_get_height(backBuffer), 1u };

		graphics_image_blit(image, backBuffer, layout, GPUImageLayout_Present, 1u, &blit, SamplerFilter_Nearest, cmd);
	}

}