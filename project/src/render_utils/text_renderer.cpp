#include "SilverEngine/core.h"

#include "render_utils/render_utils_internal.h"

namespace sv {

	// Num of letters
	constexpr u32 TEXT_BATCH_COUNT = 100u;

	struct TextVertex {
		v4_f32	position;
		v2_f32	texCoord;
		Color	color;
	};

	struct TextData {
		TextVertex vertices[TEXT_BATCH_COUNT * 4u];
	};

	struct {
		
		RenderPass* renderpass;

		GPUBuffer* buffer_vertices;
		GPUBuffer* buffer_indices;
		Sampler* sampler;
		
		Shader* vs;
		Shader* ps;

		InputLayoutState* ils;

	} static gfx = {};

	static TextData* text_data[GraphicsLimit_CommandList] = {};

	Result text_initialize()
	{
		// Create shaders
		{
			svCheck(graphics_shader_compile_fastbin_from_string("text_vertex", ShaderType_Vertex, &gfx.vs, 
R"(	
#include "core.hlsl"

struct Output {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

Output main(float4 position : Position, float4 color : Color, float2 texCoord : TexCoord)
{
	Output o;
	o.position = position;
	o.color = color;
	o.texCoord = texCoord;
	return o;
}
)", true));

			svCheck(graphics_shader_compile_fastbin_from_string("text_pixel", ShaderType_Pixel, &gfx.ps,
				R"(	
#include "core.hlsl"

SV_SAMPLER(sam, s0);
SV_TEXTURE(tex, t0);

struct Output {
	float4 color : SV_Target0;
};

Output main(float4 color : FragColor, float2 texCoord : FragTexCoord)
{
	Output o;
	
	f32 a = tex.Sample(sam, texCoord).r;
	if (a < 0.08) discard;	

	o.color = float4(color.xyz * a, color.w);
	return o;
}
)", true));
		}

		// Create buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.usage = ResourceUsage_Default;
			desc.CPUAccess = CPUAccess_Write;
			desc.size = sizeof(TextData);
			desc.pData = nullptr;

			svCheck(graphics_buffer_create(&desc, &gfx.buffer_vertices));
			graphics_name_set(gfx.buffer_vertices, "Text_VertexBuffer");

			// set index data
			constexpr u32 index_count = TEXT_BATCH_COUNT * 6u;
			u16* index_data = new u16[index_count];

			for (u32 i = 0u; i < TEXT_BATCH_COUNT; ++i) {
				
				u32 currenti = i * 6u;
				u32 currentv = i * 4u;

				index_data[currenti + 0u] = 0 + currentv;
				index_data[currenti + 1u] = 1 + currentv;
				index_data[currenti + 2u] = 2 + currentv;
						   							   
				index_data[currenti + 3u] = 1 + currentv;
				index_data[currenti + 4u] = 3 + currentv;
				index_data[currenti + 5u] = 2 + currentv;
			}

			// 10.922 characters
			desc.indexType = IndexType_16;
			desc.bufferType = GPUBufferType_Index;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.size = sizeof(u16) * index_count;
			desc.pData = index_data;

			svCheck(graphics_buffer_create(&desc, &gfx.buffer_indices));
			graphics_name_set(gfx.buffer_indices, "Text_IndexBuffer");

			delete[] index_data;
		}

		// Create InputLayoutState
		{
			InputSlotDesc slot = { 0u, sizeof(TextVertex), false };

			InputElementDesc elements[] =
			{
				{ "Position", 0u, 0u, sizeof(f32) * 0u, Format_R32G32B32A32_FLOAT },
				{ "TexCoord", 0u, 0u, sizeof(f32) * 4u, Format_R32G32_FLOAT },
				{ "Color", 0u, 0u, sizeof(f32) * 6u, Format_R8G8B8A8_UNORM },
			};

			InputLayoutStateDesc desc;
			desc.elementCount = 3u;
			desc.slotCount = 1u;
			desc.pElements = elements;
			desc.pSlots = &slot;

			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils));
		}

		// Create RenderPass
		{
			AttachmentDesc att;
			att.loadOp = AttachmentOperation_Load;
			att.storeOp = AttachmentOperation_Store;
			att.format = OFFSCREEN_FORMAT;
			att.initialLayout = GPUImageLayout_RenderTarget;
			att.layout = GPUImageLayout_RenderTarget;
			att.finalLayout = GPUImageLayout_RenderTarget;
			att.type = AttachmentType_RenderTarget;

			RenderPassDesc desc;
			desc.attachmentCount = 1u;
			desc.pAttachments = &att;

			svCheck(graphics_renderpass_create(&desc, &gfx.renderpass));
		}

		// Create Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Linear;
			desc.magFilter = SamplerFilter_Linear;

			svCheck(graphics_sampler_create(&desc, &gfx.sampler));
		}

		return Result_Success;
	}

	Result text_close()
	{
		svCheck(graphics_destroy_struct(&gfx, sizeof(gfx)));

		// Free text data
		foreach(i, GraphicsLimit_CommandList) {
			if (text_data[i]) delete text_data[i];
		}

		return Result_Success;
	}

	u32 draw_text(const char* text, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextSpace space, TextAlignment alignment, Font* pFont, GPUImage* rendertarget, CommandList cmd)
	{
		if (text == nullptr) return 0u;

		// Text size
		size_t text_size = strlen(text);

		if (text_size == 0u) return 0u;
		if (text_size > TEXT_BATCH_COUNT) {
			SV_LOG_ERROR("The text exceed the limits");
			return 0u;
		}

		graphics_offscreen_validation(rendertarget);
		const GPUImageInfo& info = graphics_image_info(rendertarget);

		// Select font
		// TODO: Default font
		Font& font = *pFont;

		// Prepare
		graphics_topology_set(GraphicsTopology_Triangles, cmd);

		graphics_shader_bind(gfx.vs, cmd);
		graphics_shader_bind(gfx.ps, cmd);
		
		graphics_vertexbuffer_bind(gfx.buffer_vertices, 0u, 0u, cmd);
		graphics_indexbuffer_bind(gfx.buffer_indices, 0u, cmd);
		graphics_image_bind(font.image, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler, 0u, ShaderType_Pixel, cmd);

		graphics_inputlayoutstate_bind(gfx.ils, cmd);

		if (text_data[cmd] == nullptr) {
			text_data[cmd] = new TextData();
		}
		TextData& data = *text_data[cmd];

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
		graphics_buffer_update(gfx.buffer_vertices, data.vertices, vertex_count * sizeof(TextVertex), 0u, cmd);

		GPUImage* att[1];
		att[0] = rendertarget;

		graphics_renderpass_begin(gfx.renderpass, att, nullptr, 1.f, 0u, cmd);
		graphics_draw_indexed(vertex_count / 4u * 6u, 1u, 0u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);

		return number_of_chars;
	}

}