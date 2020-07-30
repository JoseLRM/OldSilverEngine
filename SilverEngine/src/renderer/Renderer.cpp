#include "core.h"
#include "Engine.h"

#include "renderer_components.h"
#include "graphics/graphics_properties.h"
#include "DrawData.h"

using namespace sv;

namespace _sv {

	static uvec2 g_Resolution;
	
	static Image					g_Offscreen;
	static PostProcessing_Default	g_PP_OffscreenToBackBuffer;

	static DrawData	g_DrawData;

	bool renderer_initialize(const SV_RENDERER_INITIALIZATION_DESC& desc)
	{
		// Initial Resolution
		g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);

		// BackBuffer
		//Image& backBuffer = graphics_swapchain_get_image();

		// Create Offscreen
		{
			SV_GFX_IMAGE_DESC imageDesc;
			imageDesc.format = SV_GFX_FORMAT_R8G8B8A8_UNORM;
			imageDesc.layout = SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE;
			imageDesc.dimension = 2u;
			imageDesc.width = g_Resolution.x;
			imageDesc.height = g_Resolution.y;
			imageDesc.depth = 1u;
			imageDesc.layers = 1u;
			imageDesc.CPUAccess = 0u;
			imageDesc.usage = SV_GFX_USAGE_STATIC;
			imageDesc.pData = nullptr;
			imageDesc.type = SV_GFX_IMAGE_TYPE_RENDER_TARGET | SV_GFX_IMAGE_TYPE_SHADER_RESOURCE;

			svCheck(graphics_image_create(&imageDesc, g_Offscreen));
		}
		//TODO: swapchain format
		//svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET));
		svCheck(renderer_postprocessing_default_create(SV_GFX_FORMAT_B8G8R8A8_SRGB, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, g_PP_OffscreenToBackBuffer));

		svCheck(renderer_postprocessing_initialize());
		svCheck(renderer_layer_initialize());

		return true;
	}

	bool renderer_close()
	{
		svCheck(renderer_postprocessing_default_destroy(g_PP_OffscreenToBackBuffer));

		svCheck(renderer_postprocessing_close());
		svCheck(renderer_layer_close());

		return true;
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

	sv::Image& renderer_offscreen_get()
	{
		return g_Offscreen;
	}

	DrawData& renderer_drawdata_get()
	{
		return g_DrawData;
	}

}

namespace sv {

	using namespace _sv;

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

	///////////////////////////////// DRAW FUNCTIONS ////////////////////////////////////////////////

	void renderer_scene_begin()
	{
		renderer_layer_begin();

		g_DrawData.pCamera = nullptr;
		g_DrawData.viewMatrix = XMMatrixIdentity();
		g_DrawData.projectionMatrix = XMMatrixIdentity();
		g_DrawData.viewProjectionMatrix = XMMatrixIdentity();

		g_DrawData.pOutput = nullptr;

		g_DrawData.renderLayers.clear();
	}

	void renderer_scene_end()
	{
		renderer_layer_end();
	}

	void renderer_draw_scene(Scene& scene)
	{
		renderer_layer_prepare_scene(scene);

		// Get RenderLayers

	}

	//////////////////////////////// RENDER FUNCTIONS ////////////////////////////////////////////////

	void renderer_present(Camera& camera)
	{
		// Set Camera Values
		g_DrawData.pCamera = &camera;
		g_DrawData.viewMatrix = camera.GetViewMatrix();
		g_DrawData.projectionMatrix = camera.GetProjectionMatrix();
		g_DrawData.viewProjectionMatrix = g_DrawData.viewMatrix * g_DrawData.projectionMatrix;

		// Set Output Ptr
		// TODO: Optional offscreen in Camera class
		g_DrawData.pOutput = &g_Offscreen;

		// Render
		Image& offscreen = *g_DrawData.pOutput;
		CommandList cmd = graphics_commandlist_begin();

		GPUBarrier offscreenBarrier = GPUBarrier::Image(offscreen, SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET);
		graphics_barrier(&offscreenBarrier, 1u, cmd);

		graphics_set_pipeline_mode(SV_GFX_PIPELINE_MODE_GRAPHICS, cmd);

		Viewport viewport = offscreen->GetViewport();
		graphics_set_viewports(&viewport, 1u, cmd);

		Scissor scissor = offscreen->GetScissor();
		graphics_set_scissors(&scissor, 1u, cmd);

		renderer_layer_render(cmd);

		// PostProcess to BackBuffer

		GPUBarrier barrier = GPUBarrier::Image(g_Offscreen, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE);
		graphics_barrier(&barrier, 1u, cmd);

		Image& backBuffer = graphics_swapchain_acquire_image();

		renderer_postprocessing_default_render(g_PP_OffscreenToBackBuffer, offscreen, backBuffer, cmd);
	}

}