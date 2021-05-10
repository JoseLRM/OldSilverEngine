#pragma once

#include "scene.h"
#include "font.h"

namespace sv {

    bool _renderer_initialize();
    bool _renderer_close();
    void _renderer_begin();
    void _renderer_end();

    constexpr Format OFFSCREEN_FORMAT = Format_R8G8B8A8_UNORM;
    constexpr Format GBUFFER_DEPTH_FORMAT = Format_D24_UNORM_S8_UINT;
    constexpr Format GBUFFER_DIFFUSE_FORMAT = Format_R16G16B16A16_FLOAT;
    constexpr Format GBUFFER_NORMAL_FORMAT = Format_R16G16B16A16_FLOAT;
    constexpr Format GBUFFER_SSAO_FORMAT = Format_R32_FLOAT;

    // Functions

    bool load_skymap_image(const char* filepath, GPUImage** pimage);

    // TEXT RENDERING (Clip space rendering)

    // Return the number of character rendered
    u32 draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd);

    SV_INLINE u32 draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, CommandList cmd)
    {
	return draw_text(text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, pFont, Color::White(), cmd);
    }

    void draw_text_area(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd);

    SV_API Font& renderer_default_font();
    SV_API GPUImage* renderer_white_image();

    // SCENE

    void draw_scene();

    void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

    // POSTPROCESSING

    SV_API void postprocess_gaussian_blur(
	    GPUImage* src, 
	    GPUImageLayout src_layout0, 
	    GPUImageLayout src_layout1, 
	    GPUImage* aux, 
	    GPUImageLayout aux_layout0, 
	    GPUImageLayout aux_layout1,
	    f32 intensity,
	    f32 aspect,
	    CommandList cmd,
	    RenderPass* renderpass = nullptr // Is only used while the rendertarget is the src image
	);

    SV_API void postprocess_addition(
	    GPUImage* src,
	    GPUImage* dst,
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
	    GPUImage* emissive, // The layout should be ShaderResource
	    f32 threshold,
	    f32 intensity,
	    f32 aspect,
	    CommandList cmd
	);

    // IMMEDIATE MODE RENDERER

    enum ImRendCamera : u32 {
	ImRendCamera_Clip, // Default
	ImRendCamera_Scene,
	ImRendCamera_Normal,

#if SV_DEV
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

    SV_API void imrend_draw_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
    SV_API void imrend_draw_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd);
    SV_API void imrend_draw_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, GPUImageLayout layout, const v4_f32& texcoord, CommandList cmd);

    SV_API void imrend_draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd);

    // DEBUG RENDERER

    SV_API void begin_debug_batch(CommandList cmd);
    SV_API void end_debug_batch(bool transparent_blend, bool depth_test, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

    SV_API void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd);
    SV_API void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);
    SV_API void draw_debug_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd);
    SV_API void draw_debug_mesh_wireframe(Mesh* mesh, const XMMATRIX& transform, Color color, CommandList cmd);

    // Helper functions

    SV_API void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
    SV_API void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
    SV_API void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

    // High level draw calls

    SV_API void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
