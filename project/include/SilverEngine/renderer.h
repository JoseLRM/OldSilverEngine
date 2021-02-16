#pragma once

#include "SilverEngine/scene.h"

namespace sv {

	// UTILS

	constexpr Format OFFSCREEN_FORMAT = Format_R16G16B16A16_FLOAT;
	constexpr Format ZBUFFER_FORMAT = Format_D24_UNORM_S8_UINT;

	// Functions

	Result offscreen_create(u32 width, u32 height, GPUImage** pImage);
	Result zbuffer_create(u32 width, u32 height, GPUImage** pImage);

	// FONT

	struct Glyph {
		v4_f32 texCoord;
		f32 advance;
		f32 xoff, yoff;
		f32 w, h;
		f32 left_side_bearing;
	};

	enum FontFlag : u32 {
		FontFlag_None,
		FontFlag_Monospaced,
	};
	typedef u32 FontFlags;

	struct Font {

		std::unordered_map<char, Glyph>	glyphs;
		GPUImage* image = nullptr;

	};

	Result font_create(Font& font, const char* filepath, f32 pixelHeight, FontFlags flags);
	Result font_destroy(Font& font);

	f32 compute_text_width(const char* text, u32 count, f32 font_size, f32 aspect, Font* pFont);

	// TEXT RENDERING

	struct Font;

	enum TextSpace : u32 {
		TextSpace_Clip,
		TextSpace_Normal,
		TextSpace_Offscreen,
	};

	enum TextAlignment : u32 {
		TextAlignment_Left,
		TextAlignment_Center,
		TextAlignment_Right,
		TextAlignment_Justified,
	};

	/*
		Return the number of character rendered
	*/
	u32 draw_text(GPUImage* offscreen, const char* text, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextSpace space, TextAlignment alignment, Font* pFont, CommandList cmd);

	// SPRITE RENDERING

	struct SpriteInstance {
		XMMATRIX	tm;
		v4_f32		texcoord;
		GPUImage*	image;
		Color		color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color)
			: tm(m), texcoord(texcoord), image(image), color(color) {}
	};

	void draw_sprites(GPUImage* offscreen, const SpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, bool linear_sampler, CommandList cmd);

	// SCENE

	void draw_scene(ECS* ecs, GPUImage* offscreen, GPUImage* depthstencil);

	// DEBUG RENDERER

	void begin_debug_batch(CommandList cmd);
	void end_debug_batch(GPUImage* offscreen, const XMMATRIX& viewProjectionMatrix, CommandList cmd);

	void draw_debug_quad(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd);
	void draw_debug_ellipse(const XMMATRIX& matrix, Color color, CommandList cmd);
	void draw_debug_sprite(const XMMATRIX& matrix, Color color, GPUImage* image, CommandList cmd);

	// Helper functions

	void draw_debug_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_quad(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd);
	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, CommandList cmd);
	void draw_debug_ellipse(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, CommandList cmd);

	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, CommandList cmd);
	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, const v3_f32& rotation, Color color, GPUImage* image, CommandList cmd);
	void draw_debug_sprite(const v3_f32& position, const v2_f32& size, const v4_f32& rotationQuat, Color color, GPUImage* image, CommandList cmd);

	// Attributes

	// Line rasterization width (pixels)
	void set_debug_linewidth(f32 lineWidth, CommandList cmd);
	f32	 get_debug_linewidth(CommandList cmd);

	// Quad and ellipse stroke: 1.f renders normally, 0.01f renders thin stroke around
	void set_debug_stroke(f32 stroke, CommandList cmd);
	f32	 get_debug_stroke(CommandList cmd);

	// Sprite texCoords
	void	set_debug_texcoord(const v4_f32& texCoord, CommandList cmd);
	v4_f32	get_debug_texcoord(CommandList cmd);

	// Sprite sampler
	void set_debug_sampler_default(CommandList cmd);
	void set_debug_sampler(Sampler* sampler, CommandList cmd);

	// High level draw calls

	void draw_debug_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd);

}
