#include "core.h"

#include "renderer_internal.h"
#include "scene.h"
#include "graphics/graphics_internal.h"
#include "components.h"

namespace sv {

	static uvec2				g_Resolution;
	static RendererOutputMode	g_OutputMode;
	
	static Offscreen					g_Offscreen;
	static PostProcessing_Default		g_PP_OffscreenToBackBuffer;

	static DrawData	g_DrawData;

	// MAIN FUNCTIONS

	bool renderer_initialize(const InitializationRendererDesc& desc)
	{
		// Initial Resolution
		g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);
		g_OutputMode = desc.outputMode;

		// BackBuffer
		//Image& backBuffer = graphics_swapchain_get_image();

		// Create Offscreen
		svCheck(renderer_offscreen_create(g_Resolution.x, g_Resolution.y, g_Offscreen));

		//TODO: swapchain format
		//svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), GPUImageLayout_UNDEFINED, GPUImageLayout_RENDER_TARGET));

		svCheck(postprocessing_initialize());
		svCheck(renderer2D_initialize());

		svCheck(postprocessing_default_create(Format_B8G8R8A8_SRGB, GPUImageLayout_Undefined, GPUImageLayout_RenderTarget, g_PP_OffscreenToBackBuffer));

		return true;
	}

	bool renderer_close()
	{
		svCheck(postprocessing_default_destroy(g_PP_OffscreenToBackBuffer));

		svCheck(postprocessing_close());
		svCheck(renderer2D_close());

		svCheck(renderer_offscreen_destroy(g_Offscreen));

		return true;
	}

	void renderer_frame_begin()
	{
		graphics_begin();
	}
	void renderer_frame_end()
	{
		CommandList cmd = graphics_commandlist_last();

		// PostProcess to BackBuffer
		GPUImage& backBuffer = graphics_swapchain_acquire_image();

		if (g_OutputMode == RendererOutputMode_backBuffer) {
			postprocessing_default_render(g_PP_OffscreenToBackBuffer, g_Offscreen.renderTarget, backBuffer, cmd);
		}

		// End
		graphics_commandlist_submit();
		graphics_present();
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

	DrawData& renderer_drawData_get()
	{
		return g_DrawData;
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

	///////////////////////////////// DRAW FUNCTIONS ////////////////////////////////////////////////

	void renderer_scene_begin()
	{

		// Clear last draw data
		g_DrawData.cameras.clear();
		g_DrawData.currentCamera = {};
		g_DrawData.viewMatrix = XMMatrixIdentity();
		g_DrawData.projectionMatrix = XMMatrixIdentity();
		g_DrawData.viewProjectionMatrix = XMMatrixIdentity();
		g_DrawData.sprites.Reset();
		//g_DrawData.meshes.Reset();

		renderer2D_begin();
	}

	void renderer_scene_end()
	{
#ifdef SV_DEBUG
		{
			ui32 mainCameraCount = 0u;
			auto& cameras = g_DrawData.cameras;
			for (ui32 i = 0; i < cameras.size(); ++i) {
				if (cameras[i].pOffscreen == nullptr) mainCameraCount++;
			}

			SV_ASSERT(mainCameraCount == 1);
		}
#endif

		renderer2D_end();

		// Present scene cameras
		{
			auto& cameras = g_DrawData.cameras;
			for (ui32 i = 0; i < cameras.size(); ++i) {
				Camera_DrawData& c = g_DrawData.cameras[i];
				c.viewMatrix = XMMatrixInverse(nullptr, c.viewMatrix);
				renderer_present(c.projection, c.viewMatrix, c.pOffscreen);
			}
		}
	}

	void renderer_scene_draw_scene()
	{
		// Get Cameras
		{
			EntityView<CameraComponent> cameras;

			for (auto& camera : cameras) {
				sv::Transform trans = scene_ecs_entity_get_transform(camera.entity);

				if (sv::scene_camera_get() == camera.entity) {
					g_DrawData.cameras.push_back({ camera.projection, nullptr, trans.GetWorldMatrix() });
				}
				else if (camera.HasOffscreen()) {
					Offscreen* offscreen = camera.GetOffscreen();
					g_DrawData.cameras.push_back({ camera.projection, offscreen, trans.GetWorldMatrix() });
				}
			}
		}

		// Render Layers
		renderer2D_prepare_scene();
	}

	//////////////////////////////// RENDER FUNCTIONS ////////////////////////////////////////////////

	void renderer_present(CameraProjection projection, XMMATRIX viewMatrix, Offscreen* pOffscreen)
	{
		// Set Camera Values
		g_DrawData.currentCamera.projection = projection;
		g_DrawData.currentCamera.viewMatrix = viewMatrix;
		g_DrawData.viewMatrix = viewMatrix;
		g_DrawData.projectionMatrix = renderer_compute_projection_matrix(projection);
		g_DrawData.viewProjectionMatrix = g_DrawData.viewMatrix * g_DrawData.projectionMatrix;

		// Set Output Ptr
		if (pOffscreen) {
			g_DrawData.currentCamera.pOffscreen = pOffscreen;
		}
		else {
			g_DrawData.currentCamera.pOffscreen = &g_Offscreen;
		}

		// Render
		Offscreen& offscreen = *g_DrawData.currentCamera.pOffscreen;
		CommandList cmd = graphics_commandlist_begin();

		// Skybox
		//TODO: clear offscreen here :)

		// this is not good for performance - remember to do image barrier here
		graphics_image_clear(offscreen.renderTarget, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(offscreen.depthStencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		graphics_set_pipeline_mode(GraphicsPipelineMode_Graphics, cmd);

		Viewport viewport = offscreen.GetViewport();
		graphics_set_viewports(&viewport, 1u, cmd);

		Scissor scissor = offscreen.GetScissor();
		graphics_set_scissors(&scissor, 1u, cmd);

		// Sprite rendering
		renderer2D_sprite_render(g_DrawData.sprites.data(), g_DrawData.sprites.Size(), cmd);

		// Offscreen layout: from render target to shader resource
		GPUBarrier barrier = GPUBarrier::Image(offscreen.renderTarget, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
		graphics_barrier(&barrier, 1u, cmd);

	}

}