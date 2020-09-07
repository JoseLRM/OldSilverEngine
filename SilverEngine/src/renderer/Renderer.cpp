#include "core.h"

#include "renderer_internal.h"
#include "graphics/graphics_internal.h"

namespace sv {

	static uvec2						g_Resolution;
	static bool							g_BackBuffer;
	static Offscreen					g_Offscreen;

	// MAIN FUNCTIONS

	Result renderer_initialize(const InitializationRendererDesc& desc)
	{
		// Initial Resolution
		g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);

		// BackBuffer
		//Image& backBuffer = graphics_swapchain_get_image();

		// Create Offscreen
		svCheck(renderer_offscreen_create(g_Resolution.x, g_Resolution.y, g_Offscreen));

		//TODO: swapchain format
		//svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), GPUImageLayout_UNDEFINED, GPUImageLayout_RENDER_TARGET));

		svCheck(renderer_postprocessing_initialize(desc));
		svCheck(renderer_sprite_initialize(desc));
		svCheck(renderer_mesh_initialize(desc));

		return Result_Success;
	}

	Result renderer_close()
	{
		svCheck(renderer_mesh_close());
		svCheck(renderer_postprocessing_close());
		svCheck(renderer_sprite_close());

		svCheck(renderer_offscreen_destroy(g_Offscreen));

		return Result_Success;
	}

	void renderer_frame_begin()
	{
		graphics_begin();
	}
	void renderer_frame_end()
	{
		CommandList cmd;
		if (graphics_commandlist_count() == 0u) {
			cmd = graphics_commandlist_begin();
		}
		else {
			cmd = graphics_commandlist_last();
		}

		// PostProcess to BackBuffer
		GPUImage& backBuffer = graphics_swapchain_acquire_image();

		if (g_BackBuffer) {
			GPUImageBlit blit;
			blit.srcRegion.size = { g_Offscreen.GetWidth(), g_Offscreen.GetHeight(), 1u };
			blit.dstRegion.size = { graphics_image_get_width(backBuffer), graphics_image_get_height(backBuffer), 1u };

			graphics_image_blit(g_Offscreen.renderTarget, backBuffer, GPUImageLayout_RenderTarget, GPUImageLayout_Present, 1u, &blit, SamplerFilter_Nearest, cmd);
		}

		// End
		graphics_commandlist_submit();
		graphics_present();
	}

	Result renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen)
	{
		// Create Render Target
		GPUImageDesc imageDesc;
		imageDesc.format = SV_REND_OFFSCREEN_FORMAT;
		imageDesc.layout = GPUImageLayout_RenderTarget;
		imageDesc.dimension = 2u;
		imageDesc.width = width;
		imageDesc.height = height;
		imageDesc.depth = 1u;
		imageDesc.layers = 1u;
		imageDesc.CPUAccess = 0u;
		imageDesc.usage = ResourceUsage_Static;
		imageDesc.pData = nullptr;
		imageDesc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;

		svCheck(graphics_image_create(&imageDesc, offscreen.renderTarget));

		// Create Depth Stencil
		imageDesc.format = Format_D24_UNORM_S8_UINT;
		imageDesc.layout = GPUImageLayout_DepthStencil;
		imageDesc.type = GPUImageType_DepthStencil;

		svCheck(graphics_image_create(&imageDesc, offscreen.depthStencil));

		return Result_Success;
	}

	Result renderer_offscreen_destroy(Offscreen& offscreen)
	{
		svCheck(graphics_destroy(offscreen.renderTarget));
		svCheck(graphics_destroy(offscreen.depthStencil));
		return Result_Success;
	}

	Offscreen& renderer_offscreen_get()
	{
		return g_Offscreen;
	}

	void renderer_offscreen_set_present(bool enable)
	{
		g_BackBuffer = enable;
	}

	void renderer_resolution_set(ui32 width, ui32 height)
	{
		if (g_Resolution.x == width && g_Resolution.y == height) return;

		g_Resolution = uvec2(width, height);

		//TODO: resize buffers
	}

	uvec2 renderer_resolution_get() noexcept { return g_Resolution; }
	ui32 renderer_resolution_get_width() noexcept { return g_Resolution.x; }
	ui32 renderer_resolution_get_height() noexcept { return g_Resolution.y; }
	float renderer_resolution_get_aspect() noexcept { return float(g_Resolution.x) / float(g_Resolution.y); }

}