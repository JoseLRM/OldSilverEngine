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
		GPUImage* image = nullptr;

	};

	Result font_create(Font& font, const char* filepath, f32 pixelHeight, FontFlags flags);
	Result font_destroy(Font& font);

	f32 font_text_width(const char* text, Font* pFont);

}