#include "SilverEngine/core.h"

#include "sprite_internal.h"

namespace sv {

	constexpr u32 SPRITE_BATCH_COUNT = 1000u;

	struct {

		GPUImage*	white_texture;
		Sampler*	sampler;

		GPUBuffer* buffer_indices;
		GPUBuffer* buffer_vertices;

		RenderPass*			renderpass_default;
		BlendState*			bs_default;
		Shader*				vs_default;
		Shader*				ps_default;
		InputLayoutState*	ils_default;

	} static gfx = {};

	struct SpriteVertex {
		v4_f32 position;
		v2_f32 texCoord;
		Color color;
		Color emissionColor;
	};

	struct SpriteData {
		SpriteVertex data[SPRITE_BATCH_COUNT * 4u];
	};

	SpriteData* sprite_data[GraphicsLimit_CommandList];

	Result sprite_renderer_initialize()
	{
		// Sprite Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = SPRITE_BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, &gfx.buffer_vertices));

			// Index Buffer
			u32* indexData = new u32[SPRITE_BATCH_COUNT * 6u];
			{
				u32 indexCount = SPRITE_BATCH_COUNT * 6u;
				u32 index = 0u;
				for (u32 i = 0; i < indexCount; ) {

					indexData[i++] = index;
					indexData[i++] = index + 1;
					indexData[i++] = index + 2;
					indexData[i++] = index + 1;
					indexData[i++] = index + 3;
					indexData[i++] = index + 2;

					index += 4u;
				}
			}

			desc.bufferType = GPUBufferType_Index;
			desc.CPUAccess = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.pData = indexData;
			desc.size = SPRITE_BATCH_COUNT * 6u * sizeof(u32);
			desc.indexType = IndexType_32;

			svCheck(graphics_buffer_create(&desc, &gfx.buffer_indices));
			delete[] indexData;

			graphics_name_set(gfx.buffer_vertices, "Sprite_VertexBuffer");
			graphics_name_set(gfx.buffer_indices, "Sprite_IndexBuffer");
		}
		// Sprite RenderPass
		{
			AttachmentDesc rt_att;
			rt_att.loadOp = AttachmentOperation_Load;
			rt_att.storeOp = AttachmentOperation_Store;
			rt_att.stencilLoadOp = AttachmentOperation_DontCare;
			rt_att.stencilStoreOp = AttachmentOperation_DontCare;
			rt_att.format = OFFSCREEN_FORMAT;
			rt_att.initialLayout = GPUImageLayout_RenderTarget;
			rt_att.layout = GPUImageLayout_RenderTarget;
			rt_att.finalLayout = GPUImageLayout_RenderTarget;
			rt_att.type = AttachmentType_RenderTarget;

			//AttachmentDesc em_att;
			//em_att.loadOp = AttachmentOperation_Load;
			//em_att.storeOp = AttachmentOperation_Store;
			//em_att.stencilLoadOp = AttachmentOperation_DontCare;
			//em_att.stencilStoreOp = AttachmentOperation_DontCare;
			//em_att.format = GBuffer::FORMAT_EMISSIVE;
			//em_att.initialLayout = GPUImageLayout_RenderTarget;
			//em_att.layout = GPUImageLayout_RenderTarget;
			//em_att.finalLayout = GPUImageLayout_RenderTarget;
			//em_att.type = AttachmentType_RenderTarget;

			RenderPassDesc desc;
			desc.attachmentCount = 1u;
			desc.pAttachments = &rt_att;

			svCheck(graphics_renderpass_create(&desc, &gfx.renderpass_default));
			graphics_name_set(gfx.renderpass_default, "Sprite_DefaultPass");
		}
		// Sprite Shader Input Layout
		{
			InputSlotDesc slot = { 0u, sizeof(SpriteVertex), false };

			InputElementDesc elements[] = {
				{ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT },
				{ "TexCoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT },
				{ "Color", 0u, 0u, 6u * sizeof(f32), Format_R8G8B8A8_UNORM },
				//{ "EmissionColor", 0u, 0u, 7u * sizeof(f32), Format_R8G8B8A8_UNORM },
			};

			InputLayoutStateDesc desc;
			desc.pElements = elements;
			desc.elementCount = 3u;
			desc.pSlots = &slot;
			desc.slotCount = 1u;

			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_default));
			graphics_name_set(gfx.ils_default, "Sprite_DefaultInputLayout");
		}
		// Blend State
		{
			BlendAttachmentDesc rt_att;
			rt_att.blendEnabled = true;
			rt_att.srcColorBlendFactor = BlendFactor_SrcAlpha;
			rt_att.dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			rt_att.colorBlendOp = BlendOperation_Add;
			rt_att.srcAlphaBlendFactor = BlendFactor_One;
			rt_att.dstAlphaBlendFactor = BlendFactor_One;
			rt_att.alphaBlendOp = BlendOperation_Add;
			rt_att.colorWriteMask = ColorComponent_All;

			BlendStateDesc desc;
			desc.pAttachments = &rt_att;
			desc.attachmentCount = 1u;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			
			//blendState.attachments[1].blendEnabled = false;
			//blendState.attachments[1].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&desc, &gfx.bs_default));

			graphics_name_set(gfx.bs_default, "Sprite_DefaultBlendState");
		}

		// Sprite White Image
		{
			u8 bytes[4];
			for (u32 i = 0; i < 4; ++i) bytes[i] = 255u;

			GPUImageDesc desc;
			desc.pData = bytes;
			desc.size = sizeof(u8) * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.CPUAccess = CPUAccess_None;
			desc.width = 1u;
			desc.height = 1u;

			svCheck(graphics_image_create(&desc, &gfx.white_texture));
		}
		// Sprite Default Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, &gfx.sampler));
			graphics_name_set(gfx.sampler, "Sprite_Sampler");
		}

		// Sprite Shaders
		{
			svCheck(graphics_shader_compile_fastbin_from_file("sprite_vertex", ShaderType_Vertex, &gfx.vs_default, "library/shader_utils/sprite/default_vertex.hlsl"));
			svCheck(graphics_shader_compile_fastbin_from_file("sprite_pixel", ShaderType_Pixel, &gfx.ps_default, "library/shader_utils/sprite/default_pixel.hlsl"));
		}

		return Result_Success;
	}

	Result sprite_renderer_close()
	{
		svCheck(graphics_destroy_struct(&gfx, sizeof(gfx)));

		// deallocate context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				if (sprite_data[i]) {
					delete sprite_data[i];
				}
			}
		}

		return Result_Success;
	}

	static SV_INLINE void validate_sprite_data(CommandList cmd)
	{
		if (sprite_data[cmd] == nullptr)
			sprite_data[cmd] = new SpriteData();
	}

	void draw_sprites(const SpriteInstance* sprites, u32 count, const XMMATRIX& view_projection_matrix, GPUImage* offscreen, CommandList cmd)
	{
		if (count == 0u) return;

		validate_sprite_data(cmd);
		SpriteData& data = *sprite_data[cmd];

		// Prepare
		graphics_event_begin("Sprite_GeometryPass", cmd);

		graphics_topology_set(GraphicsTopology_Triangles, cmd);
		graphics_vertexbuffer_bind(gfx.buffer_vertices, 0u, 0u, cmd);
		graphics_indexbuffer_bind(gfx.buffer_indices, 0u, cmd);
		graphics_inputlayoutstate_bind(gfx.ils_default, cmd);
		graphics_sampler_bind(gfx.sampler, 0u, ShaderType_Pixel, cmd);
		graphics_shader_bind(gfx.vs_default, cmd);
		graphics_shader_bind(gfx.ps_default, cmd);
		graphics_blendstate_bind(gfx.bs_default, cmd);

		GPUImage* att[1];
		att[0] = offscreen;

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

				pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
				pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);

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

			graphics_buffer_update(gfx.buffer_vertices, data.data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

			graphics_renderpass_begin(gfx.renderpass_default, att, nullptr, 1.f, 0u, cmd);

			end = it;
			it -= instanceCount;

			const SpriteInstance* begin = it;
			const SpriteInstance* last = it;

			graphics_image_bind(it->image ? it->image : gfx.white_texture, 0u, ShaderType_Pixel, cmd);

			while (it != end) {

				if (it->image != last->image) {

					u32 spriteCount = u32(it - last);
					u32 startVertex = u32(last - begin) * 4u;

					graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

					if (it->image != last->image) {
						graphics_image_bind(it->image ? it->image : gfx.white_texture, 0u, ShaderType_Pixel, cmd);
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