#pragma once

#include "scene.h"
#include "terrain.h"
#include "font.h"

namespace sv {

    bool _renderer_initialize();
    bool _renderer_close();
    void _renderer_begin();
    void _renderer_end();

    constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
    constexpr Format GBUFFER_DEPTH_FORMAT = Format_D24_UNORM_S8_UINT;
    constexpr Format GBUFFER_NORMAL_FORMAT = Format_R16G16B16A16_FLOAT;
    constexpr Format GBUFFER_EMISSION_FORMAT = Format_R16G16B16A16_FLOAT;
    constexpr Format GBUFFER_SSAO_FORMAT = Format_R32_FLOAT;
	
    // Functions

    bool load_skymap_image(const char* filepath, GPUImage** pimage);

    // TEXT RENDERING (Clip space rendering)

	struct DrawTextDesc {

		const char*   text = NULL;
		size_t        text_size = size_t_max;
		f32           max_width = f32_max;
		u32           max_lines = u32_max;
		bool          centred = true;
		XMMATRIX      transform_matrix = XMMatrixIdentity();
		TextAlignment alignment = TextAlignment_Center;
		Font*         font = NULL;
		f32           font_size = 0.1f;
		f32           aspect = f32_max;
		Color         color = Color::White();
		
	};

	SV_API void draw_text(const DrawTextDesc& desc, CommandList cmd);

	struct DrawTextAreaDesc {
		
		const char*   text = NULL;
		size_t        text_size = size_t_max;
		f32           max_width = 2.f;
		u32           max_lines = u32_max;
		XMMATRIX      transform_matrix = XMMatrixIdentity();
		f32           font_size = 0.1f;
		f32           aspect = f32_max;
		TextAlignment alignment = TextAlignment_Left;
		u32           line_offset = 0u;
		bool          bottom_top = true;
		Font*         font = NULL;
		Color         color = Color::White();
		
	};
	
    SV_API void draw_text_area(const DrawTextAreaDesc& desc, CommandList cmd);

    SV_API Font& renderer_default_font();
    SV_API GPUImage* renderer_white_image();
	SV_API GPUImage* renderer_offscreen();

    // SCENE

    void draw_scene();

    void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

    // POSTPROCESSING

	enum BlurType : u32 {
		BlurType_GaussianFloat4,
		BlurType_GaussianFloat,
	};

    SV_API void postprocess_blur(
			BlurType blur_type,
			GPUImage* src, 
			GPUImageLayout src_layout0, 
			GPUImageLayout src_layout1, 
			GPUImage* aux, 
			GPUImageLayout aux_layout0, 
			GPUImageLayout aux_layout1,
			f32 intensity,
			f32 aspect,
			CommandList cmd
		);

    SV_API void postprocess_bloom(
			GPUImage* src, 
			GPUImageLayout src_layout0, 
			GPUImageLayout src_layout1, 
			GPUImage* aux0, // Used in threshold pass
			GPUImageLayout aux0_layout0, 
			GPUImageLayout aux0_layout1,
			GPUImage* aux1, // Used to blur the image
			GPUImageLayout aux1_layout0, 
			GPUImageLayout aux1_layout1,
			GPUImage* emission,
			GPUImageLayout emission_layout0, 
			GPUImageLayout emission_layout1,
			f32 threshold,
			f32 intensity,
			f32 strength,
			u32 iterations,
			f32 aspect,
			CommandList cmd
		);

    // IMMEDIATE MODE RENDERER

    enum ImRendCamera : u32 {
		ImRendCamera_Clip, // Default
		ImRendCamera_Scene,
		ImRendCamera_Normal,

#if SV_EDITOR
		ImRendCamera_Editor,
#endif
    };

    SV_API void imrend_begin_batch(CommandList cmd);
    SV_API void imrend_flush(CommandList cmd);

    SV_API void imrend_push_matrix(const XMMATRIX& matrix, CommandList cmd);
    SV_API void imrend_pop_matrix(CommandList cmd);

    SV_API void imrend_push_scissor(f32 x, f32 y, f32 width, f32 height, bool additive, CommandList cmd);
    SV_API void imrend_pop_scissor(CommandList cmd);

    SV_API void imrend_camera(ImRendCamera camera, CommandList cmd);
	SV_API void imrend_camera(const XMMATRIX& view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

    // Draw calls

    SV_API void imrend_draw_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
    SV_API void imrend_draw_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);
    SV_API void imrend_draw_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd);
    SV_API void imrend_draw_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, GPUImageLayout layout, const v4_f32& texcoord, CommandList cmd);

    SV_API void imrend_draw_mesh_wireframe(Mesh* mesh, Color color, CommandList cmd);

    // TODO: Font, color and alignment in stack
	
    SV_API void imrend_draw_text(const char* text, f32 font_size, f32 aspect, Font* font, Color color, CommandList cmd);
	SV_API void imrend_draw_text_bounds(const char* text, f32 max_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* font, Color color, CommandList cmd);
    SV_API void imrend_draw_text_area(const char* text, size_t text_size, f32 max_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* font, Color color, CommandList cmd);

    // High level draw calls

	SV_API void imrend_draw_cube_wireframe(Color color, CommandList cmd);
	SV_API void imrend_draw_sphere_wireframe(u32 vertical_segments, u32 horizontal_segments, Color color, CommandList cmd);

    SV_API void imrend_draw_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
