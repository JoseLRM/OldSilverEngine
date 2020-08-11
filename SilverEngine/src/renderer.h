#pragma once

#include "core.h"

#include "graphics.h"
#include "renderer/RenderList.h"
#include "renderer/renderer2D/renderer2D.h"

namespace sv {
	
	// Enums

	enum RendererOutputMode : ui8 {
		RendererOutputMode_backBuffer,
		RendererOutputMode_offscreen
	};

	enum CameraType {
		CameraType_Clip,
		CameraType_Orthographic,
		CameraType_Perspective,
	};

	// Renderer initialization

	struct InitializationRendererDesc {
		ui32				resolutionWidth;
		ui32				resolutionHeight;
		RendererOutputMode	outputMode;
	};

	// Camera Projection

	struct CameraProjection {
		CameraType cameraType;
		union {
			struct {
				float width;
				float height;
			} orthographic;
			struct {

			} perspective;
		};

		CameraProjection()
		{
			cameraType = CameraType_Orthographic;
			orthographic = { 1.f, 1.f };
		}
	};

	XMMATRIX	renderer_compute_projection_matrix(const CameraProjection& projection);
	float		renderer_compute_projection_aspect_get(const CameraProjection& projection);
	void		renderer_compute_projection_aspect_set(CameraProjection& projection, float aspect);
	vec2		renderer_compute_orthographic_position(const CameraProjection& projection, const vec2& point); // The point must be in range { -0.5 - 0.5 }
	float		renderer_compute_orthographic_zoom_get(const CameraProjection& projection);
	void		renderer_compute_orthographic_zoom_set(CameraProjection& projection, float zoom);

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

	// High level draw calls

	void renderer_scene_begin();
	void renderer_scene_end();
	void renderer_scene_draw_scene();

	void renderer_present(CameraProjection projection, XMMATRIX viewMatrix, Offscreen* pOffscreen);

}