#include "SilverEngine/core.h"

#include "SilverEngine/renderer.h"	
#include "renderer/renderer_internal.h"

namespace sv {

	u32 draw_text(const char* text, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextSpace space, TextAlignment alignment, Font* pFont, CommandList cmd)
	{
		SV_ASSERT_OFFSCREEN();

		if (text == nullptr) return 0u;

		// Text size
		size_t text_size = strlen(text);

		if (text_size == 0u) return 0u;
		if (text_size > TEXT_BATCH_COUNT) {
			SV_LOG_ERROR("The text exceed the limits");
			return 0u;
		}

		GPUImage* rendertarget = render_context[cmd].offscreen;
		const GPUImageInfo& info = graphics_image_info(rendertarget);

		// Select font
		// TODO: Default font
		Font& font = pFont ? *pFont : font_opensans;

		// Prepare
		graphics_rasterizerstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_topology_set(GraphicsTopology_Triangles, cmd);

		graphics_shader_bind(gfx.vs_text, cmd);
		graphics_shader_bind(gfx.ps_text, cmd);
		
		GPUBuffer* buffer = get_batch_buffer(cmd);
		graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(gfx.ibuffer_text, 0u, cmd);
		graphics_image_bind(font.image, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		graphics_inputlayoutstate_bind(gfx.ils_text, cmd);

		TextData& data = *reinterpret_cast<TextData*>(rend_utils[cmd].batch_data);

		// Text space transformation
		f32 xmult = 0.f;
		f32 ymult = 0.f;
		
		switch (space)
		{
		case TextSpace_Clip:
			xmult = font_size / aspect;
			ymult = font_size;
			break;

		case TextSpace_Normal:
			xmult = font_size / aspect * 2.f;
			ymult = font_size * 2.f;
			x = x * 2.f - 1.f;
			y = y * 2.f - 1.f;
			max_line_width *= 2.f;
			break;

		case TextSpace_Offscreen:
		{
			f32 inv_offwidth = 1.f / f32(info.width) * 2.f;
			f32 inv_offheight = 1.f / f32(info.height) * 2.f;

			aspect /= f32(info.width) / f32(info.height);

			xmult = font_size * inv_offwidth / aspect;
			ymult = font_size * inv_offheight;
			x = x * inv_offwidth - 1.f;
			y = y * inv_offheight - 1.f;
			max_line_width *= inv_offwidth;
		} break;
		}

		// Write line

		u32 vertex_count = 0u;
		f32 line_height = ymult;

		const char* it = text;
		const char* end = it + text_size;

		f32 xoff;
		f32 yoff = 0.f;

		u32 number_of_chars = 0u;

		while (it != end && max_lines--) {

			xoff = 0.f;
			yoff -= line_height;
			u32 line_begin_index = vertex_count;

			// Create line
			f32 line_width = 0.f;

			// Add begin offset
			if (*it != ' ') {
				auto res = font.glyphs.find(*it);

				if (res != font.glyphs.end()) {

					Glyph& g = res->second;
					
					f32 offset = abs(std::min(g.xoff, 0.f) * xmult);
					line_width += offset;
					xoff += offset;
				}
			}

			const char* line_end = it;
			const char* begin_word = line_end;

			while (line_end != end) {

				if (*line_end == '\n') {
					++line_end;
					break;
				}
				
				if (!char_is_letter(*line_end) && !char_is_number(*line_end)) {
					
					if ((*line_end == '.' || *line_end == ',') && char_is_number(*begin_word))  {
					}
					else begin_word = line_end + 1u;
				}

				auto res = font.glyphs.find(*line_end);

				if (res != font.glyphs.end()) {

					Glyph& g = res->second;

					f32 real_line_width = line_width + std::max(g.w, g.advance) * xmult;

					if (real_line_width > max_line_width) {
						if (begin_word != it && (char_is_letter(*line_end) || char_is_number(*line_end))) {

							line_end = begin_word;
						}
						break;
					}
					
					line_width += g.advance * xmult;
				}

				++line_end;
			}

			if (line_end == it) {
				++line_end;
			}

			// Fill batch
			while (it != line_end) {

				auto res = font.glyphs.find(*it);

				if (res != font.glyphs.end()) {

					Glyph& g = res->second;

					f32 advance = g.advance * xmult;

					if (*it != ' ') {
						f32 xpos = xoff + g.xoff * xmult + x;
						f32 ypos = yoff + g.yoff * ymult + y;
						f32 width = g.w * xmult;
						f32 height = g.h * ymult;

						data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.w }, Color::White() };
						data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.w }, Color::White() };
						data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.y }, Color::White() };
						data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.y }, Color::White() };
					}

					xoff += advance;
				}

				++it;
			}

			// Alignment
			if (vertex_count != line_begin_index) {

				switch (alignment)
				{
				case TextAlignment_Center:
				{
					f32 line_width = (data.vertices[vertex_count - 1u].position.x - x);
					f32 displacement = (max_line_width - line_width) * 0.5f;

					for (u32 i = line_begin_index; i < vertex_count; ++i) {

						data.vertices[i].position.x += displacement;
					}
				} break;

				case TextAlignment_Right:
					break;
				case TextAlignment_Justified:
					SV_LOG_ERROR("TODO-> Justified text rendering");
					break;
				}
			}
		}

		number_of_chars = it - text;

		// Send to GPU
		graphics_buffer_update(buffer, data.vertices, vertex_count * sizeof(TextVertex), 0u, cmd);

		GPUImage* att[1];
		att[0] = rendertarget;

		graphics_renderpass_begin(gfx.renderpass_text, att, nullptr, 1.f, 0u, cmd);
		graphics_draw_indexed(vertex_count / 4u * 6u, 1u, 0u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);

		return number_of_chars;
	}

}