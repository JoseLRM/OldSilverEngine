#include "core.h"

#include "sprite_renderer_internal.h"

namespace sv {
	
	static SpriteRendererContext g_Context[GraphicsLimit_CommandList];

	static GPUImage* g_SpriteWhiteTexture = nullptr;
	static Sampler* g_SpriteDefSampler = nullptr;

	static RenderPass* g_RenderPass_Geometry = nullptr;
	static BlendState* g_BS_Geometry = nullptr;
	static Shader* g_VS_Geometry = nullptr;
	static Shader* g_PS_Geometry = nullptr;
	static InputLayoutState* g_ILS_Geometry = nullptr;
	static GPUBuffer* g_SpriteIndexBuffer = nullptr;
	static GPUBuffer* g_SpriteVertexBuffer = nullptr;

	Result sprite_module_initialize()
	{
		// Init context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				ctx.spriteData = new SpriteData();
			}
		}

		// Sprite Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = SPRITE_BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, &g_SpriteVertexBuffer));

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

			svCheck(graphics_buffer_create(&desc, &g_SpriteIndexBuffer));
			delete[] indexData;

			graphics_name_set(g_SpriteVertexBuffer, "Sprite_VertexBuffer");
			graphics_name_set(g_SpriteIndexBuffer, "Sprite_IndexBuffer");
		}
		// Sprite RenderPass
		{
			RenderPassDesc desc;
			desc.attachments.resize(2);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = GBuffer::FORMAT_OFFSCREEN;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			desc.attachments[1].loadOp = AttachmentOperation_Load;
			desc.attachments[1].storeOp = AttachmentOperation_Store;
			desc.attachments[1].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[1].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[1].format = GBuffer::FORMAT_EMISSIVE;
			desc.attachments[1].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[1].layout = GPUImageLayout_RenderTarget;
			desc.attachments[1].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[1].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Geometry));
			graphics_name_set(g_RenderPass_Geometry, "Sprite_GeometryPass");
		}
		// Sprite Shader Input Layout
		{
			InputLayoutStateDesc inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ "TexCoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT });
			inputLayout.elements.push_back({ "Color", 0u, 0u, 6u * sizeof(f32), Format_R8G8B8A8_UNORM });
			inputLayout.elements.push_back({ "EmissionColor", 0u, 0u, 7u * sizeof(f32), Format_R8G8B8A8_UNORM });

			svCheck(graphics_inputlayoutstate_create(&inputLayout, &g_ILS_Geometry));
			graphics_name_set(g_ILS_Geometry, "Sprite_GeometryInputLayout");
		}
		// Blend State
		{
			BlendStateDesc blendState;
			blendState.attachments.resize(2);
			blendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			blendState.attachments[0].blendEnabled = true;
			blendState.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			blendState.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			blendState.attachments[0].colorBlendOp = BlendOperation_Add;
			blendState.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].alphaBlendOp = BlendOperation_Add;
			blendState.attachments[0].colorWriteMask = ColorComponent_All;

			blendState.attachments[1].blendEnabled = false;
			blendState.attachments[1].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&blendState, &g_BS_Geometry));

			graphics_name_set(g_BS_Geometry, "Sprite_GeometryBlendState");
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
			desc.dimension = 2u;
			desc.width = 1u;
			desc.height = 1u;
			desc.depth = 1u;
			desc.layers = 1u;

			svCheck(graphics_image_create(&desc, &g_SpriteWhiteTexture));
		}
		// Sprite Default Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, &g_SpriteDefSampler));
			graphics_name_set(g_SpriteDefSampler, "Sprite_DefSampler");
		}

		// Sprite Shaders
		{
			svCheck(graphics_shader_compile_fastbin_from_file("sprite_vertex", ShaderType_Vertex, &g_VS_Geometry, "library/shader_utils/sprite_vertex.hlsl"));
			svCheck(graphics_shader_compile_fastbin_from_file("sprite_pixel", ShaderType_Pixel, &g_PS_Geometry, "library/shader_utils/sprite_pixel.hlsl"));
		}

		return Result_Success;
	}

	Result sprite_module_close()
	{
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));

		// FORWARD RENDERING
		svCheck(graphics_destroy(g_BS_Geometry));
		svCheck(graphics_destroy(g_RenderPass_Geometry));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_ILS_Geometry));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));

		// deallocate context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				delete ctx.spriteData;
			}
		}

		return Result_Success;
	}

	void draw_sprites(const SpriteInstance* pSprites, u32 count, const XMMATRIX& viewProjectionMatrix, GPUImage* offscreen, GPUImage* emissive, CommandList cmd)
	{
		if (count == 0u) return;

		SpriteRendererContext& ctx = g_Context[cmd];

		// Prepare
		graphics_event_begin("Sprite_GeometryPass", cmd);

		graphics_topology_set(GraphicsTopology_Triangles, cmd);
		graphics_vertexbuffer_bind(g_SpriteVertexBuffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
		graphics_inputlayoutstate_bind(g_ILS_Geometry, cmd);
		graphics_sampler_bind(g_SpriteDefSampler, 0u, ShaderType_Pixel, cmd);
		graphics_shader_bind(g_VS_Geometry, cmd);
		graphics_shader_bind(g_PS_Geometry, cmd);
		graphics_blendstate_bind(g_BS_Geometry, cmd);

		GPUImage* att[2];
		att[0] = offscreen;
		att[1] = emissive;

		XMMATRIX matrix;
		XMVECTOR pos0, pos1, pos2, pos3;

		// Batch data, used to update the vertex buffer
		SpriteVertex* batchIt = ctx.spriteData->data;

		// Used to draw
		u32 instanceCount;

		const SpriteInstance* it = pSprites;
		const SpriteInstance* end = it + count;

		while (it != end) {

			// Compute the end ptr of the vertex data
			SpriteVertex* batchEnd;
			{
				size_t batchCount = batchIt - ctx.spriteData->data + SPRITE_BATCH_COUNT * 4u;
				instanceCount = std::min(batchCount / 4u, size_t(end - it));
				batchEnd = batchIt + instanceCount * 4u;
			}

			// Fill batch buffer
			while (batchIt != batchEnd) {

				const SpriteInstance& spr = *it++;

				matrix = XMMatrixMultiply(spr.tm, viewProjectionMatrix);

				pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
				pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);

				pos0 = XMVector4Transform(pos0, matrix);
				pos1 = XMVector4Transform(pos1, matrix);
				pos2 = XMVector4Transform(pos2, matrix);
				pos3 = XMVector4Transform(pos3, matrix);

				*batchIt = { v4_f32(pos0), {spr.texCoord.x, spr.texCoord.y}, spr.color, spr.emissionColor };
				++batchIt;

				*batchIt = { v4_f32(pos1), {spr.texCoord.z, spr.texCoord.y}, spr.color, spr.emissionColor };
				++batchIt;

				*batchIt = { v4_f32(pos2), {spr.texCoord.x, spr.texCoord.w}, spr.color, spr.emissionColor };
				++batchIt;

				*batchIt = { v4_f32(pos3), {spr.texCoord.z, spr.texCoord.w}, spr.color, spr.emissionColor };
				++batchIt;
			}

			// Draw

			graphics_buffer_update(g_SpriteVertexBuffer, ctx.spriteData->data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

			graphics_renderpass_begin(g_RenderPass_Geometry, att, nullptr, 1.f, 0u, cmd);

			end = it;
			it -= instanceCount;

			const SpriteInstance* begin = it;
			const SpriteInstance* last = it;

			graphics_image_bind(it->image ? it->image : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);

			while (it != end) {

				if (it->image != last->image) {

					u32 spriteCount = u32(it - last);
					u32 startVertex = u32(last - begin) * 4u;

					graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

					if (it->image != last->image) {
						graphics_image_bind(it->image ? it->image : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
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

			end = pSprites + count;
			batchIt = ctx.spriteData->data;
		}
	}
}