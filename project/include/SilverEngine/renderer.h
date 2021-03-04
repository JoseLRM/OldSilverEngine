#pragma once

#include "SilverEngine/scene.h"
#include "SilverEngine/font.h"

namespace sv {

	// UTILS

	constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
	constexpr Format ZBUFFER_FORMAT = Format_D24_UNORM_S8_UINT;

	// Functions

	Result offscreen_create(u32 width, u32 height, GPUImage** pImage);
	Result depthstencil_create(u32 width, u32 height, GPUImage** pImage);

	// TEXT RENDERING

	/*
		Clip space rendering
		Return the number of character rendered
	*/
	u32 draw_text(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd);

	SV_INLINE u32 draw_text(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, CommandList cmd)
	{
		return draw_text(offscreen, text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, pFont, Color::White(), cmd);
	}

	void draw_text_area(GPUImage* offscreen, const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd);

	// SCENE

	void draw_scene(Scene* scene);

	void draw_sky(GPUImage* offscreen, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd);

	XMMATRIX camera_view_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);
	XMMATRIX camera_projection_matrix(CameraComponent& camera);
	XMMATRIX camera_view_projection_matrix(const v3_f32& position, const v4_f32 rotation, CameraComponent& camera);

	// DEBUG RENDERER

	void begin_debug_batch(CommandList cmd);
	void end_debug_batch(GPUImage* offscreen, GPUImage* depthstencil, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);

	// Helper functions

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	// High level draw calls

	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
