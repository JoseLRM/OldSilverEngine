#include "core.h"

#include "renderer_internal.h"

namespace sv {

	// Sprite Primitives

	static GraphicsPipeline g_SpritePipeline;
	static RenderPass g_SpriteRenderPass;

	static Shader g_SpriteVertexShader;
	static Shader g_SpritePixelShader;
	static InputLayoutState g_SpriteInputLayoutState;
	static BlendState g_SpriteBlendState;

	static GPUBuffer g_SpriteIndexBuffer;
	static GPUBuffer g_SpriteVertexBuffer;
	static GPUImage g_SpriteWhiteTexture;
	static Sampler g_SpriteDefSampler;

	struct SpriteVertex {
		vec4 position;
		vec2 texCoord;
		Color color;
	};

	static SpriteVertex g_SpriteData[SV_REND_LAYER_BATCH_COUNT * 4u];

	constexpr void ComputeIndexData(ui32* indices)
	{
		ui32 indexCount = SV_REND_LAYER_BATCH_COUNT * 6u;
		ui32 index = 0u;
		for (ui32 i = 0; i < indexCount; ) {

			indices[i++] = index;
			indices[i++] = index + 1;
			indices[i++] = index + 2;
			indices[i++] = index + 1;
			indices[i++] = index + 3;
			indices[i++] = index + 2;

			index += 4u;
		}
	}

	bool renderer_sprite_initialize(const InitializationRendererDesc& desc)
	{
		// Sprite Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = SV_REND_LAYER_BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, g_SpriteVertexBuffer));

			// Index Buffer
			ui32 indexData[SV_REND_LAYER_BATCH_COUNT * 6u];
			ComputeIndexData(indexData);

			desc.bufferType = GPUBufferType_Index;
			desc.CPUAccess = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.pData = indexData;
			desc.size = SV_REND_LAYER_BATCH_COUNT * 6u * sizeof(ui32);
			desc.indexType = IndexType_32;

			svCheck(graphics_buffer_create(&desc, g_SpriteIndexBuffer));

		}
		// Sprite Shaders
		{
			svCheck(renderer_shader_create("Sprite", ShaderType_Vertex, g_SpriteVertexShader));
			svCheck(renderer_shader_create("Sprite", ShaderType_Pixel, g_SpritePixelShader));
		}
		// Sprite RenderPass
		{
			RenderPassDesc desc;
			desc.attachments.resize(1);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = SV_REND_OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, g_SpriteRenderPass));
		}
		// Sprite Pipeline
		{
			InputLayoutStateDesc inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT });
			inputLayout.elements.push_back({ "Color", 0u, 0u, 6u * sizeof(float), Format_R8G8B8A8_UNORM });

			svCheck(graphics_inputlayoutstate_create(&inputLayout, g_SpriteInputLayoutState));

			BlendStateDesc blendState;
			blendState.attachments.resize(1);
			blendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			blendState.attachments[0].blendEnabled = true;
			blendState.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			blendState.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			blendState.attachments[0].colorBlendOp = BlendOperation_Add;
			blendState.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].alphaBlendOp = BlendOperation_Add;
			blendState.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&blendState, g_SpriteBlendState));

			g_SpritePipeline.pVertexShader = &g_SpriteVertexShader;
			g_SpritePipeline.pPixelShader = &g_SpritePixelShader;
			g_SpritePipeline.pGeometryShader = nullptr;
			g_SpritePipeline.pInputLayoutState = &g_SpriteInputLayoutState;
			g_SpritePipeline.pBlendState = &g_SpriteBlendState;
			g_SpritePipeline.pRasterizerState = nullptr;
			g_SpritePipeline.pDepthStencilState = nullptr;
			g_SpritePipeline.topology = GraphicsTopology_Triangles;
			g_SpritePipeline.stencilRef = 0u;
		}
		// Sprite White Image
		{
			ui8 bytes[4];
			for (ui32 i = 0; i < 4; ++i) bytes[i] = 255u;

			GPUImageDesc desc;
			desc.pData = bytes;
			desc.size = sizeof(ui8) * 4u;
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

			svCheck(graphics_image_create(&desc, g_SpriteWhiteTexture));
		}
		// Sprite Default Sampler
		{
			SamplerDesc desc;
			desc.addressModeU = SamplerAddressMode_Wrap;
			desc.addressModeV = SamplerAddressMode_Wrap;
			desc.addressModeW = SamplerAddressMode_Wrap;
			desc.minFilter = SamplerFilter_Nearest;
			desc.magFilter = SamplerFilter_Nearest;

			svCheck(graphics_sampler_create(&desc, g_SpriteDefSampler));
		}

		return true;
	}

	bool renderer_sprite_close()
	{
		g_SpritePipeline = {};
		svCheck(graphics_destroy(g_SpriteInputLayoutState));
		svCheck(graphics_destroy(g_SpriteBlendState));
		svCheck(graphics_destroy(g_SpriteRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexShader));
		svCheck(graphics_destroy(g_SpritePixelShader));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));

		return true;
	}

	void RenderSpriteBatch(ui32 offset, ui32 size, Texture* texture, CommandList cmd)
	{
		GPUBuffer* vBuffers[] = {
			&g_SpriteVertexBuffer,
		};
		ui32 offsets[] = {
			0u,
		};
		ui32 strides[] = {
			size * 4u * sizeof(SpriteVertex),
		};
		GPUImage* images[1];
		Sampler* samplers[1];

		if (texture) {
			images[0] = &texture->GetImage();
			samplers[0] = &texture->GetSampler();
		}
		else {
			images[0] = &g_SpriteWhiteTexture;
			samplers[0] = &g_SpriteDefSampler;
		}

		graphics_vertexbuffer_bind(vBuffers, offsets, strides, 1u, cmd);

		graphics_image_bind(images, 1u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(samplers, 1u, ShaderType_Pixel, cmd);

		graphics_draw_indexed(size * 6u, 1u, offset * 6u, 0u, 0u, cmd);

	}

	void renderer_sprite_rendering(const SpriteRenderingDesc* desc, CommandList cmd)
	{
		GPUImage* att[] = {
			desc->pRenderTarget
		};

		Texture* texture = nullptr;

		XMMATRIX viewProjectionMatrix = *desc->pViewProjectionMatrix;
		ui32 count = desc->count;
		const SpriteInstance* buffer = desc->pInstances;
		const SpriteInstance* initialPtr = buffer;

		while (buffer < initialPtr + count) {

			ui32 j = 0u;
			ui32 currentInstance = buffer - initialPtr;
			ui32 batchSize = SV_REND_LAYER_BATCH_COUNT;
			if (currentInstance + batchSize > count) {
				batchSize = count - currentInstance;
			}

			// Fill Vertex Buffer
			while (j < batchSize) {

				const SpriteInstance& spr = buffer[j];

				// Compute Matrices form WorldSpace to ScreenSpace
				XMMATRIX matrix = spr.tm * viewProjectionMatrix;

				XMVECTOR pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);

				pos0 = XMVector4Transform(pos0, matrix);
				pos1 = XMVector4Transform(pos1, matrix);
				pos2 = XMVector4Transform(pos2, matrix);
				pos3 = XMVector4Transform(pos3, matrix);

				vec4 texCoord;
				if (spr.sprite.pTextureAtlas)
					texCoord = spr.sprite.pTextureAtlas->GetSprite(spr.sprite.index);

				ui32 index = j * 4u;
				g_SpriteData[index + 0] = { pos0, {texCoord.x, texCoord.y}, spr.color };
				g_SpriteData[index + 1] = { pos1, {texCoord.z, texCoord.y}, spr.color };
				g_SpriteData[index + 2] = { pos2, {texCoord.x, texCoord.w}, spr.color };
				g_SpriteData[index + 3] = { pos3, {texCoord.z, texCoord.w}, spr.color };

				j++;
			}

			graphics_buffer_update(g_SpriteVertexBuffer, g_SpriteData, batchSize * sizeof(SpriteVertex) * 4u, 0u, cmd);

			// Begin rendering
			graphics_renderpass_begin(g_SpriteRenderPass, att, nullptr, 1.f, 0u, cmd);

			graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
			graphics_pipeline_bind(g_SpritePipeline, cmd);

			const SpriteInstance* beginBuffer = buffer;
			const SpriteInstance* endBuffer;

			while (buffer < beginBuffer + batchSize) {

				endBuffer = buffer + batchSize;

				texture = buffer->sprite.pTextureAtlas;
				
				ui32 offset = buffer - beginBuffer;
				while (buffer != endBuffer) {

					if (buffer->sprite.pTextureAtlas != texture) {
						ui32 batchPos = buffer - beginBuffer;
						RenderSpriteBatch(offset, batchPos - offset, texture, cmd);
						offset = batchPos;
						texture = buffer->sprite.pTextureAtlas;
					}

					buffer++;
				}

				ui32 batchPos = buffer - beginBuffer;
				RenderSpriteBatch(offset, batchPos - offset, texture, cmd);

			}

			graphics_renderpass_end(cmd);
		}

	}

}