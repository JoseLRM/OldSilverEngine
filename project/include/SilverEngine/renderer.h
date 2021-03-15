#pragma once

#include "SilverEngine/scene.h"
#include "SilverEngine/font.h"

namespace sv {

	// UTILS

	constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
	constexpr Format ZBUFFER_FORMAT = Format_D24_UNORM_S8_UINT;

	// Functions

	Result create_offscreen(u32 width, u32 height, GPUImage** pImage);
	Result create_depthstencil(u32 width, u32 height, GPUImage** pImage);

	Result load_skymap_image(const char* filepath, GPUImage** pimage);

	// TEXT RENDERING (Clip space rendering)

	// Return the number of character rendered
	u32 draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd);

	SV_INLINE u32 draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, CommandList cmd)
	{
		return draw_text(text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, pFont, Color::White(), cmd);
	}

	void draw_text_area(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd);

	// SCENE

	void draw_scene(Scene* scene);

	void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

	XMMATRIX camera_view_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);
	XMMATRIX camera_projection_matrix(CameraComponent& camera);
	XMMATRIX camera_view_projection_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);

	// POSTPROCESSING

	void postprocess_gaussian_blur(
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

	void postprocess_addition(
		GPUImage* src,
		GPUImage* dst,
		CommandList cmd
	);

	void postprocess_bloom(
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

	// DEBUG RENDERER

	void begin_debug_batch(CommandList cmd);
	void end_debug_batch(bool transparent_blend, bool depth_test, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);
	void draw_debug_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd);
	void draw_debug_mesh_wireframe(Mesh* mesh, const XMMATRIX& transform, Color color, CommandList cmd);

	// Helper functions

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	// High level draw calls

	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
