#include "SilverEngine/core.h"

#include "SilverEngine/renderer.h"
#include "renderer/renderer_internal.h"

namespace sv {

	void draw_sprites(const SpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, bool linear_sampler, CommandList cmd)
	{
		SV_ASSERT_OFFSCREEN();

		if (count == 0u) return;

		SpriteData& data = *(SpriteData*)rend_utils[cmd].batch_data;
		GPUBuffer* batch_buffer = get_batch_buffer(cmd);

		// Prepare
		graphics_event_begin("Sprite_GeometryPass", cmd);

		graphics_topology_set(GraphicsTopology_Triangles, cmd);
		graphics_vertexbuffer_bind(batch_buffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(gfx.ibuffer_sprite, 0u, cmd);
		graphics_inputlayoutstate_bind(gfx.ils_sprite, cmd);
		graphics_sampler_bind(linear_sampler ? gfx.sampler_def_linear : gfx.sampler_def_nearest, 0u, ShaderType_Pixel, cmd);
		graphics_shader_bind(gfx.vs_sprite, cmd);
		graphics_shader_bind(gfx.ps_sprite, cmd);
		graphics_blendstate_bind(gfx.bs_sprite, cmd);

		GPUImage* att[1];
		att[0] = render_context[cmd].offscreen;

		XMMATRIX matrix;
		XMVECTOR pos0, pos1, pos2, pos3;

		// Batch data, used to update the vertex buffer
		SpriteVertex* batchIt = data.data;

		// Used to draw
		u32 instanceCount;

		const SpriteInstance* it = sprites;
		const SpriteInstance* end = it + count;

		while (it != end) {

			// Compute the end ptr of the vertex data
			SpriteVertex* batchEnd;
			{
				size_t batchCount = batchIt - data.data + SPRITE_BATCH_COUNT * 4u;
				instanceCount = std::min(batchCount / 4u, size_t(end - it));
				batchEnd = batchIt + instanceCount * 4u;
			}

			// Fill batch buffer
			while (batchIt != batchEnd) {

				const SpriteInstance& spr = *it++;

				matrix = XMMatrixMultiply(spr.tm, view_projection_matrix);

				pos0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				pos1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
				pos2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				pos3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

				pos0 = XMVector4Transform(pos0, matrix);
				pos1 = XMVector4Transform(pos1, matrix);
				pos2 = XMVector4Transform(pos2, matrix);
				pos3 = XMVector4Transform(pos3, matrix);

				*batchIt = { v4_f32(pos0), {spr.texcoord.x, spr.texcoord.y}, spr.color };
				++batchIt;

				*batchIt = { v4_f32(pos1), {spr.texcoord.z, spr.texcoord.y}, spr.color };
				++batchIt;

				*batchIt = { v4_f32(pos2), {spr.texcoord.x, spr.texcoord.w}, spr.color };
				++batchIt;

				*batchIt = { v4_f32(pos3), {spr.texcoord.z, spr.texcoord.w}, spr.color };
				++batchIt;
			}

			// Draw

			graphics_buffer_update(batch_buffer, data.data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

			graphics_renderpass_begin(gfx.renderpass_sprite, att, nullptr, 1.f, 0u, cmd);

			end = it;
			it -= instanceCount;

			const SpriteInstance* begin = it;
			const SpriteInstance* last = it;

			graphics_image_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);

			while (it != end) {

				if (it->image != last->image) {

					u32 spriteCount = u32(it - last);
					u32 startVertex = u32(last - begin) * 4u;

					graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

					if (it->image != last->image) {
						graphics_image_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);
					}

					last = it;
				}

				++it;
			}

			// Last draw call
			{
				u32 spriteCount = u32(it - last);
				u32 startVertex = u32(last - begin) * 4u;

				graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);
			}

			graphics_renderpass_end(cmd);

			end = sprites + count;
			batchIt = data.data;
		}
	}
}