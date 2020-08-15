#pragma once

#include "core.h"

#include "graphics.h"
#include "renderer/RenderList.h"
#include "renderer/renderer2D/renderer2D.h"
#include "renderer/mesh_renderer/mesh_renderer.h"

namespace sv {
	
	// Enums

	enum CameraType {
		CameraType_Clip,
		CameraType_Orthographic,
		CameraType_Perspective,
	};

	// Renderer initialization

	struct InitializationRendererDesc {
		ui32				resolutionWidth;
		ui32				resolutionHeight;
	};

	// Camera Projection

	struct CameraProjection {
		CameraType cameraType = CameraType_Orthographic;
		float width = 1.f;
		float height = 1.f;
		float near = 0.0f;
		float far = 10000.f;
	};

	XMMATRIX	renderer_projection_matrix(const CameraProjection& projection);
	float		renderer_projection_aspect_get(const CameraProjection& projection);
	void		renderer_projection_aspect_set(CameraProjection& projection, float aspect);
	vec2		renderer_projection_position(const CameraProjection& projection, const vec2& point); // The point must be in range { -0.5 - 0.5 }
	float		renderer_projection_zoom_get(const CameraProjection& projection);
	void		renderer_projection_zoom_set(CameraProjection& projection, float zoom);

	// Offscreen

	struct Offscreen {
		GPUImage renderTarget;
		GPUImage depthStencil;
		inline Viewport GetViewport() const noexcept { return graphics_image_get_viewport(renderTarget); }
		inline Scissor GetScissor() const noexcept { return graphics_image_get_scissor(renderTarget); }
		inline ui32 GetWidth() const noexcept { return graphics_image_get_width(renderTarget); }
		inline ui32 GetHeight() const noexcept { return graphics_image_get_height(renderTarget); }
	};

	bool renderer_offscreen_create(ui32 width, ui32 height, Offscreen& offscreen);
	bool renderer_offscreen_destroy(Offscreen& offscreen);

	Offscreen& renderer_offscreen_get();

	// MainOffscreen resolution

	void	renderer_resolution_set(ui32 width, ui32 height);
	uvec2	renderer_resolution_get() noexcept;
	ui32	renderer_resolution_get_width() noexcept;
	ui32	renderer_resolution_get_height() noexcept;
	float	renderer_resolution_get_aspect() noexcept;

	// Camera Settings

	struct CameraSettings {
		bool					active;
		MeshRenderingTechnique	meshTechnique;
		struct
		{
			bool enabled;
			bool drawColliders;
		} debug;
	};

	// High level draw calls

	void renderer_scene_render(bool backBuffer);

	enum RendererTarget {
		RendererTarget_Offscreen		= SV_BIT(0),
		RendererTarget_CameraOffscreen	= SV_BIT(1),
		RendererTarget_BackBuffer		= SV_BIT(2),
	};
	typedef ui32 RendererTargetFlags;

	struct CameraDesc {
		CameraProjection	projection;
		CameraSettings		settings;
		Offscreen*			pOffscreen;
		XMMATRIX			viewMatrix;
	};

	struct RendererDesc {
		RendererTargetFlags		rendererTarget;
		CameraDesc				camera;
	};

	void renderer_scene_begin(const RendererDesc* desc);
	void renderer_scene_end();
	void renderer_scene_draw_scene();

}