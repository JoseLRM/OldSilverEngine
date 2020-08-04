#include "core.h"
#include "Engine.h"

#include "renderer_components.h"
#include "graphics/graphics_properties.h"

using namespace sv;

namespace _sv {

	static uvec2				g_Resolution;
	static SV_REND_OUTPUT_MODE	g_OutputMode;
	
	static Offscreen					g_Offscreen;
	static PostProcessing_Default		g_PP_OffscreenToBackBuffer;

	static DrawData	g_DrawData;

	bool renderer_initialize(const SV_RENDERER_INITIALIZATION_DESC& desc)
	{
		// Initial Resolution
		g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);
		g_OutputMode = desc.outputMode;

		// BackBuffer
		//Image& backBuffer = graphics_swapchain_get_image();

		// Create Offscreen
		svCheck(renderer_offscreen_create(g_Resolution.x, g_Resolution.y, g_Offscreen));

		//TODO: swapchain format
		//svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET));

		svCheck(renderer_postprocessing_initialize());
		svCheck(renderer_layer_initialize());

		svCheck(renderer_postprocessing_default_create(SV_GFX_FORMAT_B8G8R8A8_SRGB, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, g_PP_OffscreenToBackBuffer));

		return true;
	}

	bool renderer_close()
	{
		svCheck(renderer_postprocessing_default_destroy(g_PP_OffscreenToBackBuffer));

		svCheck(renderer_postprocessing_close());
		svCheck(renderer_layer_close());

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
		Image& backBuffer = graphics_swapchain_acquire_image();

		if (g_OutputMode == SV_REND_OUTPUT_MODE_BACK_BUFFER) {
			renderer_postprocessing_default_render(g_PP_OffscreenToBackBuffer, g_Offscreen.renderTarget, backBuffer, cmd);
		}

		// End
		graphics_commandlist_submit();
		graphics_present();
	}

	bool renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen)
	{
		// Create Render Target
		SV_GFX_IMAGE_DESC imageDesc;
		imageDesc.format = SV_REND_OFFSCREEN_FORMAT;
		imageDesc.layout = SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE;
		imageDesc.dimension = 2u;
		imageDesc.width = width;
		imageDesc.height = height;
		imageDesc.depth = 1u;
		imageDesc.layers = 1u;
		imageDesc.CPUAccess = 0u;
		imageDesc.usage = SV_GFX_USAGE_STATIC;
		imageDesc.pData = nullptr;
		imageDesc.type = SV_GFX_IMAGE_TYPE_RENDER_TARGET | SV_GFX_IMAGE_TYPE_SHADER_RESOURCE;

		svCheck(graphics_image_create(&imageDesc, offscreen.renderTarget));

		// Create Depth Stencil
		imageDesc.format = SV_GFX_FORMAT_D24_UNORM_S8_UINT;
		imageDesc.layout = SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL;
		imageDesc.type = SV_GFX_IMAGE_TYPE_DEPTH_STENCIL;

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

		g_DrawData.cameras.clear();
		g_DrawData.currentCamera = {};
		g_DrawData.viewMatrix = XMMatrixIdentity();
		g_DrawData.projectionMatrix = XMMatrixIdentity();
		g_DrawData.viewProjectionMatrix = XMMatrixIdentity();
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
		// Present scene cameras
		{
			auto& cameras = g_DrawData.cameras;
			for (ui32 i = 0; i < cameras.size(); ++i) {
				Camera_DrawData& c = g_DrawData.cameras[i];
				c.viewMatrix = XMMatrixInverse(nullptr, c.viewMatrix);
				renderer_present(c.projection, c.viewMatrix, c.pOffscreen);
			}
		}

		// End render layers
		renderer_layer_end();
	}

	void renderer_scene_draw_scene(Scene& scene)
	{
		// Get Cameras
		{
			auto addCameraFn = [](Scene& scene, Entity entity, BaseComponent** comp, float dt) 
			{
				CameraComponent& camComp = *reinterpret_cast<CameraComponent*>(comp[0]);
				sv::Transform trans = scene.GetTransform(entity);
				if (camComp.HasOffscreen()) {
					Offscreen* offscreen = camComp.GetOffscreen();
					g_DrawData.cameras.push_back({ camComp.projection, offscreen, trans.GetWorldMatrix() });
				}
			};

			CompID req[] = {
				CameraComponent::ID
			};

			SV_ECS_SYSTEM_DESC cameraSystem;
			cameraSystem.executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
			cameraSystem.system = addCameraFn;
			cameraSystem.pRequestedComponents = req;
			cameraSystem.requestedComponentsCount = 1u;
			cameraSystem.optionalComponentsCount = 0u;

			scene.ExecuteSystems(&cameraSystem, 1u, 0.f);
		}

		// Render layers
		renderer_layer_prepare_scene(scene);
	}

	void renderer_scene_set_camera(const CameraProjection& projection, XMMATRIX worldMatrix)
	{
		g_DrawData.cameras.push_back({ projection, nullptr, worldMatrix });
	}

	//////////////////////////////// RENDER FUNCTIONS ////////////////////////////////////////////////

	void renderer_present(CameraProjection projection, XMMATRIX viewMatrix, _sv::Offscreen* pOffscreen)
	{
		// Set Camera Values
		g_DrawData.currentCamera.projection = projection;
		g_DrawData.currentCamera.viewMatrix = viewMatrix;
		g_DrawData.viewMatrix = viewMatrix;
		g_DrawData.projectionMatrix = projection.GetProjectionMatrix();
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
		graphics_image_clear(offscreen.renderTarget, SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);
		graphics_image_clear(offscreen.depthStencil, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, { 0.f, 0.f, 0.f, 1.f }, 1.f, 0u, cmd);

		graphics_set_pipeline_mode(SV_GFX_PIPELINE_MODE_GRAPHICS, cmd);

		Viewport viewport = offscreen.GetViewport();
		graphics_set_viewports(&viewport, 1u, cmd);

		Scissor scissor = offscreen.GetScissor();
		graphics_set_scissors(&scissor, 1u, cmd);

		renderer_layer_render(cmd);
		
		// Offscreen layout: from render target to shader resource
		GPUBarrier barrier = GPUBarrier::Image(offscreen.renderTarget, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE);
		graphics_barrier(&barrier, 1u, cmd);

	}

}