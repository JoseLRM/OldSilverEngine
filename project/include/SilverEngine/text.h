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
		
		std::unordered_map<char, Glyph>	glyphs;
		GPUImage*						image;

	};

	Result font_create(Font& font, const char* filepath, f32 pixelHeight, FontFlags flags);

	f32 font_text_width(const char* text, Font* pFont);

	// TEXT RENDERING

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
	u32 draw_text(const char* text, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextSpace space, TextAlignment alignment, Font* pFont, GPUImage* rendertarget, CommandList cmd);

}