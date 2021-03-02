#include "SilverEngine/core.h"

#define STB_TRUETYPE_IMPLEMENTATION

#include "external/stb_truetype.h"

#include "renderer/renderer_internal.h"

namespace sv {

	struct TempGlyph {
		u8* bitmap;
		i32 w, h;
		i32 advance;
		i32 xoff, yoff;
		i32 left_side_bearing;
		v4_f32 texCoord;
		char c;
	};

	struct GlyphLine {
		u32 begin_index;
		u32 end_index;
		u32 height;
	};

	SV_INLINE static void add_glyph(char c, stbtt_fontinfo& info, f32 heightScale, std::vector<TempGlyph>& glyphs)
	{
		i32 index = stbtt_FindGlyphIndex(&info, c);

		TempGlyph glyph;
		glyph.c = c;
		glyph.bitmap = (c == ' ') ? nullptr : stbtt_GetGlyphBitmap(&info, 0.f, heightScale, index, &glyph.w, &glyph.h, &glyph.xoff, &glyph.yoff);
		stbtt_GetGlyphHMetrics(&info, index, &glyph.advance, &glyph.left_side_bearing);

		glyph.advance = i32(f32(glyph.advance) * heightScale);

		glyphs.push_back(glyph);
	}

	Result font_create(Font& font, const char* filepath, f32 pixel_height, FontFlags flags)
	{
		std::vector<u8> data;
		svCheck(file_read_binary(filepath, data));

		stbtt_fontinfo info;
		stbtt_InitFont(&info, data.data(), 0);

		font.glyphs = new Glyph[Font::CHAR_COUNT];
		
		// TODO: Resize
		std::vector<TempGlyph> glyphs;
		std::vector<GlyphLine> lines;

		f32 height_scale = stbtt_ScaleForPixelHeight(&info, pixel_height);

		// Get Font metrix
		f32 line_height;
		{
			i32 ascent, descent, line_gap;
			stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

			line_height = f32(ascent - descent + line_gap) * height_scale;
		}

		for (char c = 'A'; c <= 'Z'; ++c) {
			add_glyph(c, info, height_scale, glyphs);
		}
		for (char c = 'a'; c <= 'z'; ++c) {
			add_glyph(c, info, height_scale, glyphs);
		}
		for (char c = '0'; c <= '9'; ++c) {
			add_glyph(c, info, height_scale, glyphs);
		}

		add_glyph('%', info, height_scale, glyphs);
		add_glyph('+', info, height_scale, glyphs);
		add_glyph('*', info, height_scale, glyphs);
		add_glyph('-', info, height_scale, glyphs);
		add_glyph('/', info, height_scale, glyphs);
		add_glyph('.', info, height_scale, glyphs);
		add_glyph(',', info, height_scale, glyphs);
		add_glyph('-', info, height_scale, glyphs);
		add_glyph('_', info, height_scale, glyphs);
		add_glyph('(', info, height_scale, glyphs);
		add_glyph(')', info, height_scale, glyphs);
		add_glyph('?', info, height_scale, glyphs);
		add_glyph('!', info, height_scale, glyphs);
		add_glyph('#', info, height_scale, glyphs);
		add_glyph('@', info, height_scale, glyphs);
		add_glyph('<', info, height_scale, glyphs);
		add_glyph('>', info, height_scale, glyphs);
		add_glyph(':', info, height_scale, glyphs);
		add_glyph(';', info, height_scale, glyphs);
		add_glyph('[', info, height_scale, glyphs);
		add_glyph(']', info, height_scale, glyphs);
		add_glyph('{', info, height_scale, glyphs);
		add_glyph('}', info, height_scale, glyphs);
		add_glyph('=', info, height_scale, glyphs);
		add_glyph('\"', info, height_scale, glyphs);
		add_glyph('\'', info, height_scale, glyphs);
		add_glyph('\\', info, height_scale, glyphs);
		add_glyph('|', info, height_scale, glyphs);
		add_glyph('~', info, height_scale, glyphs);
		add_glyph('$', info, height_scale, glyphs);
		add_glyph('&', info, height_scale, glyphs);
		add_glyph(' ', info, height_scale, glyphs);

		// Sort the glyphs by their heights
		std::sort(glyphs.begin(), glyphs.end(), 
			[](const TempGlyph& g0, const TempGlyph& g1)
		{
			return g0.h > g1.h;
		});

		// Calculate num of lines and atlas resolution
		u32 atlas_width;
		u32 atlas_height;
		u32 atlas_spacing = 5u;//u32(0.035f * pixel_height);

		{
			atlas_width = u32(pixel_height * 6.f);

			u32 xoffset = 0u;

			GlyphLine* current_line = &lines.emplace_back();
			current_line->begin_index = 0u;

			foreach (i, glyphs.size()) {

				TempGlyph& g = glyphs[i];
				if (g.bitmap == nullptr) continue;

				// Next line
				if (xoffset + g.w + atlas_spacing >= atlas_width) {
					
					xoffset = 0u;

					current_line->height = glyphs[current_line->begin_index].h + atlas_spacing;
					current_line->end_index = i;

					current_line = &lines.emplace_back();

					current_line->begin_index = i;
				}
				
				xoffset += g.w + atlas_spacing;
			}

			lines.back().end_index = u32(glyphs.size());
			lines.back().height = glyphs[lines.back().begin_index].h + atlas_spacing;

			// Calculate atlas height
			atlas_height = 0u;
			foreach (i, lines.size()) atlas_height += lines[i].height;
		}

		// Draw font atlas and set texcoords
		u8* atlas = new u8[atlas_width * atlas_height];
		SV_ZERO_MEMORY(atlas, atlas_width * atlas_height);

		size_t xoffset = 0u;
		size_t yoffset = 0u;

		for (const GlyphLine& line : lines) {

			for (u32 i = line.begin_index; i < line.end_index; ++i) {
				
				TempGlyph& g = glyphs[i];
				if (g.bitmap == nullptr) continue;

				// Compute texcoord
				g.texCoord.x = f32(xoffset) / f32(atlas_width);
				g.texCoord.y = f32(yoffset) / f32(atlas_height);
				g.texCoord.z = g.texCoord.x + f32(g.w) / f32(atlas_width);
				g.texCoord.w = g.texCoord.y + f32(g.h) / f32(atlas_height);

				for (size_t y = 0u; y < g.h; ++y) {

					size_t bitmap_offset = y * g.w;

					for (size_t x = 0u; x < g.w; ++x) {

						atlas[x + xoffset + (y + yoffset) * atlas_width] = g.bitmap[x + bitmap_offset];
					}
				}

				xoffset += g.w + atlas_spacing;
			}

			xoffset = 0u;
			yoffset += line.height;
		}

		// Free bitmaps and set data to the font
		for (const TempGlyph& g0 : glyphs) {
			stbtt_FreeBitmap(g0.bitmap, 0);

			Glyph* g_ = font.get(u32(g0.c));

			if (g_) {
			
				Glyph& g = *g_;

				g.advance = g0.advance / line_height;
				g.left_side_bearing = g0.left_side_bearing / line_height;
				g.xoff = g0.xoff / line_height;
				g.w = g0.w / line_height;
				g.h = g0.h / line_height;

				// Top - bottom to bottom - top
				g.yoff = (f32(-g0.yoff) - f32(g0.h)) / line_height;
				g.texCoord.x = g0.texCoord.x;
				g.texCoord.y = g0.texCoord.w;
				g.texCoord.z = g0.texCoord.z;
				g.texCoord.w = g0.texCoord.y;
			}
		}

		GPUImageDesc desc;
		desc.pData = atlas;
		desc.size = sizeof(u8) * atlas_width * atlas_height;
		desc.format = Format_R8_UNORM;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.width = atlas_width;
		desc.height = atlas_height;
		
		svCheck(graphics_image_create(&desc, &font.image));
		delete[] atlas;

		return Result_Success;
	}

	Result font_destroy(Font& font)
	{
		delete[] font.glyphs;
		return graphics_destroy(font.image);
	}

	f32 compute_text_width(const char* text, u32 count, f32 font_size, f32 aspect, Font* pFont)
	{
		const char* it = text;
		const char* end = it + size_t(count);

		Font& font = pFont ? *pFont : font_opensans;

		f32 width = 0u;

		while (it != end) {

			Glyph* glyphit = font.get(*it);

			if (glyphit) {
				width += glyphit->advance;
			}

			++it;
		}

		return width * font_size / aspect;
	}

	f32 compute_text_height(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont)
	{
		u32 lines = compute_text_lines(text, count, font_size, max_line_width, aspect, pFont);
		return f32(lines) * font_size;
	}

	u32 compute_text_lines(const char* text, u32 count, f32 font_size, f32 max_line_width, f32 aspect, Font* pFont)
	{
		u32 lines = 0u;

		const char* it = text;
		const char* end = text + size_t(count);

		while (it != end) {
			it = process_text_line(it, count - (it - text), max_line_width / (font_size / aspect), *pFont);
			++lines;
		}

		return lines;
	}

	const char* process_text_line(const char* it, u32 count, f32 max_line_width, Font& font)
	{
		// Create line
		f32 line_width = 0.f;

		// Add begin offset
		if (*it != ' ') {
			auto g = font.get(*it);

			if (g) {

				f32 offset = abs(std::min(g->xoff, 0.f));
				line_width += offset;
				//xoff += offset;
			}
		}

		const char* end = it + size_t(count);
		const char* begin_line = it;
		const char* begin_word = it;

		while (it != end) {

			if (*it == '\n') {
				++it;
				break;
			}

			// Add line width
			auto g = font.get(*it);

			if (g) {

				f32 real_line_width = line_width + std::max(g->w, g->advance);

				if (real_line_width > max_line_width) {

					if (begin_word != begin_line && (char_is_letter(*it) || char_is_number(*it))) {

						it = begin_word;
					}
					break;
				}

				line_width += g->advance;
			}

			// Create lines
			if (*it == ' ') {
				begin_word = nullptr;
			}
			else if (begin_word == nullptr && *it != ' ') {
				begin_word = it;
			}

			++it;
		}

		return it;
	}

}