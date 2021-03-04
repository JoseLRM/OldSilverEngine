#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

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

		static constexpr u32 CHAR_COUNT = u32('~') + 1u;

		Glyph* glyphs = nullptr;
		GPUImage* image = nullptr;
		f32 vertical_offset = 0.f;

		Glyph* get(u32 index) {
			if (index < CHAR_COUNT)
				return glyphs + index;
			return nullptr;
		}

	};

	enum TextAlignment : u32 {
		TextAlignment_Left,
		TextAlignment_Center,
		TextAlignment_Right,
		TextAlignment_Justified,
	};

	Result font_create(Font& font, const char* filepath, f32 pixelHeight, FontFlags flags);
	void font_destroy(Font& font);

	// In Clip space
	f32 compute_text_width(const char* text, u32 count, f32 font_size, f32 aspect, Font* pFont);
	f32 compute_text_height(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont);
	u32 compute_text_lines(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont);

	const char* process_text_line(const char* it, u32 count, f32 max_line_width, Font& font);

}