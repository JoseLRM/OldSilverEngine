#include "core.h"

#include "renderer_internal.h"
#include "scene/scene_internal.h"
#include "graphics/graphics_internal.h"
#include "components.h"

namespace sv {

	static uvec2						g_Resolution;
	static bool							g_BackBuffer;
	static Offscreen					g_Offscreen;
	static PostProcessing_Default		g_PP_OffscreenToBackBuffer;

	static DrawData						g_DrawData;

	// MAIN FUNCTIONS

	bool renderer_initialize(const InitializationRendererDesc& desc)
	{
		// Initial Resolution
		g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);

		// BackBuffer
		//Image& backBuffer = graphics_swapchain_get_image();

		// Create Offscreen
		svCheck(renderer_offscreen_create(g_Resolution.x, g_Resolution.y, g_Offscreen));

		//TODO: swapchain format
		//svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), GPUImageLayout_UNDEFINED, GPUImageLayout_RENDER_TARGET));

		svCheck(postprocessing_initialize());
		svCheck(renderer2D_initialize());
		svCheck(mesh_renderer_initialize());

		svCheck(postprocessing_default_create(Format_B8G8R8A8_SRGB, GPUImageLayout_Undefined, GPUImageLayout_RenderTarget, g_PP_OffscreenToBackBuffer));

		return true;
	}

	bool renderer_close()
	{
		svCheck(postprocessing_default_destroy(g_PP_OffscreenToBackBuffer));

		svCheck(mesh_renderer_close());
		svCheck(postprocessing_close());
		svCheck(renderer2D_close());

		svCheck(renderer_offscreen_destroy(g_Offscreen));

		return true;
	}

	void renderer_frame_begin()
	{
		graphics_begin();
		g_BackBuffer = false;
	}
	void renderer_frame_end()
	{
		CommandList cmd = graphics_commandlist_last();

		// PostProcess to BackBuffer
		GPUImage& backBuffer = graphics_swapchain_acquire_image();

		if (g_BackBuffer) {
			postprocessing_default_render(g_PP_OffscreenToBackBuffer, g_Offscreen.renderTarget, backBuffer, cmd);
		}

		// End
		graphics_commandlist_submit();
		graphics_present();
	}

	DrawData& renderer_drawData_get()
	{
		return g_DrawData;
	}

	bool renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen)
	{
		// Create Render Target
		GPUImageDesc imageDesc;
		imageDesc.format = SV_REND_OFFSCREEN_FORMAT;
		imageDesc.layout = GPUImageLayout_ShaderResource;
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

		return true;
	}

	bool renderer_offscreen_destroy(Offscreen& offscreen)
	{
		svCheck(graphics_destroy(offscreen.renderTarget));
		svCheck(graphics_destroy(offscreen.depthStencil));
		return true;
	}

	Offscreen& renderer_offscreen_get()
	{
		return g_Offscreen;
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

	// Render layers

	void renderLayer_count_set(ui32 count)
	{
		scene_renderWorld_get().renderLayers.resize(count);
	}

	ui32 renderLayer_count_get()
	{
		return ui32(scene_renderWorld_get().renderLayers.size());
	}

	void renderLayer_sortMode_set(ui32 layer, RenderLayerSortMode sortMode)
	{
		RenderLayer& rl = scene_renderWorld_get().renderLayers[layer];
		rl.sortMode = sortMode;
	}

	RenderLayerSortMode	renderLayer_sortMode_get(ui32 layer)
	{
		return scene_renderWorld_get().renderLayers[layer].sortMode;
	}

	///////////////////////////////// DRAW FUNCTIONS ////////////////////////////////////////////////

	void renderer_scene_render(bool backBuffer)
	{
		EntityView<CameraComponent> cameras;
		for (CameraComponent& camera : cameras) {

			bool draw = false;

			// Fill desc struct
			RendererDesc desc{};

			if (camera.GetOffscreen() != nullptr) {
				desc.camera.pOffscreen = camera.GetOffscreen();
				desc.rendererTarget |= RendererTarget_CameraOffscreen;
				draw = true;
			}

			if (camera.entity == scene_camera_get()) {
				desc.rendererTarget |= RendererTarget_Offscreen;
				if (backBuffer) desc.rendererTarget |= RendererTarget_BackBuffer;
				draw = true;
			}

			if (!camera.settings.active) draw = false;
			if (!draw) continue;

			Transform trans = scene_ecs_entity_get_transform(camera.entity);

			desc.camera.projection = camera.projection;
			desc.camera.settings = camera.settings;
			desc.camera.viewMatrix = XMMatrixInverse(nullptr, trans.GetWorldMatrix());

			// Draw
			renderer_scene_begin(&desc);
			renderer_scene_draw_scene();
			renderer_scene_end();

		}
	}

	void renderer_scene_begin(const RendererDesc* desc)
	{
		SV_ASSERT(desc != nullptr);
		g_DrawData.projection = desc->camera.projection;
		g_DrawData.settings = desc->camera.settings;
		g_DrawData.viewMatrix = desc->camera.viewMatrix;
		g_DrawData.projectionMatrix = renderer_projection_matrix(g_DrawData.projection);
		g_DrawData.viewProjectionMatrix = g_DrawData.viewMatrix * g_DrawData.projectionMatrix;

		if (desc->rendererTarget == RendererTarget_CameraOffscreen) {
			SV_ASSERT(desc->camera.pOffscreen != nullptr);
			g_DrawData.pOffscreen = desc->camera.pOffscreen;
		}
		else if (desc->rendererTarget & RendererTarget_Offscreen) {
			g_DrawData.pOffscreen = &g_Offscreen;
			if (desc->rendererTarget & RendererTarget_BackBuffer) g_BackBuffer = true;
		}

	}

	void renderer_scene_end()
	{
		g_DrawData = {};
	}

	//////////////////////////////// RENDER FUNCTIONS ////////////////////////////////////////////////

	void renderer_scene_draw_scene()
	{
		// Render
		Offscreen& offscreen = *g_DrawData.pOffscreen;

		// Begin command list
		CommandList cmd = graphics_commandlist_begin();

		Viewport viewport = offscreen.GetViewport();
		graphics_set_viewports(&viewport, 1u, cmd);

		Scissor scissor = offscreen.GetScissor();
		graphics_set_scissors(&scissor, 1u, cmd);

		// Skybox
		//TODO: clear offscreen here :)

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(offscreen.renderTarget, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(offscreen.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);		

		// Sprite rendering
		renderer2D_scene_draw_sprites(cmd);
		// Mesh rendering
		mesh_renderer_scene_draw_meshes(cmd);

		// Offscreen layout: from render target to shader resource
		GPUBarrier barrier = GPUBarrier::Image(offscreen.renderTarget, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
		graphics_barrier(&barrier, 1u, cmd);

	}

}