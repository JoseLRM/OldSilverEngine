#include "core.h"

#include "sprite_renderer_internal.h"
#include "utils/allocator.h"

namespace sv {

	static constexpr ui32 BATCH_COUNT = 1000u;

	// Sprite Primitives

	static RenderPass* g_SpriteRenderPass = nullptr;

	static InputLayoutState* g_SpriteInputLayoutState = nullptr;

	static BlendState* g_SpriteBlendState = nullptr;

	static GPUBuffer* g_SpriteIndexBuffer = nullptr;
	static GPUBuffer* g_SpriteVertexBuffer = nullptr;
	static GPUImage* g_SpriteWhiteTexture = nullptr;
	static Sampler* g_SpriteDefSampler = nullptr;

	static ShaderLibrary g_DefShaderLibrary;

	struct SpriteVertex {
		vec4f position;
		vec2f texCoord;
		Color color;
	};

	struct SpriteData {
		SpriteVertex data[BATCH_COUNT * 4u];
	};

	static SizedInstanceAllocator g_SpriteData(sizeof(SpriteData), 2u);

	constexpr void ComputeIndexData(ui32* indices)
	{
		ui32 indexCount = BATCH_COUNT * 6u;
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

	Result sprite_renderer_initialize()
	{
		// Sprite Buffers
		{
			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = BATCH_COUNT * sizeof(SpriteVertex) * 4u;

			svCheck(graphics_buffer_create(&desc, &g_SpriteVertexBuffer));

			// Index Buffer
			ui32* indexData = new ui32[BATCH_COUNT * 6u];
			ComputeIndexData(indexData);

			desc.bufferType = GPUBufferType_Index;
			desc.CPUAccess = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.pData = indexData;
			desc.size = BATCH_COUNT * 6u * sizeof(ui32);
			desc.indexType = IndexType_32;

			svCheck(graphics_buffer_create(&desc, &g_SpriteIndexBuffer));
			delete[] indexData;

		}
		// Sprite RenderPass
		{
			RenderPassDesc desc;
			desc.attachments.resize(1);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = SV_OFFSCREEN_FORMAT;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_SpriteRenderPass));
		}
		// Sprite Shader Input Layout
		{
			InputLayoutStateDesc inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT });
			inputLayout.elements.push_back({ "Color", 0u, 0u, 6u * sizeof(float), Format_R8G8B8A8_UNORM });

			svCheck(graphics_inputlayoutstate_create(&inputLayout, &g_SpriteInputLayoutState));
		}
		// Blend State
		{
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

			svCheck(graphics_blendstate_create(&blendState, &g_SpriteBlendState));
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
		}
		// Default Shader library
		{
			if (result_fail(g_DefShaderLibrary.createFromBinary("SilverEngine/DefaultSprite"))) {

				const char* src = 
					"#name DefaultSprite\n"
					"#package SilverEngine\n"
					"#VS_begin\n"
					"#include \"core.hlsl\"\n"
					"SV_DEFINE_CAMERA(b0);\n"
					"struct Input\n"
					"{\n"
					"	float4 position : Position;\n"
					"	float2 texCoord : TexCoord;\n"
					"	float4 color : Color;\n"
					"};\n"
					"struct Output\n"
					"{\n"
					"	float4 color : FragColor;\n"
					"	float2 texCoord : FragTexCoord;\n"
					"	float4 position : SV_Position;\n"
					"};\n"
					"Output main(Input input)\n"
					"{\n"
					"	Output output;\n"
					"	output.color = input.color;\n"
					"	output.texCoord = input.texCoord;\n"
					"	output.position = mul(camera.vpm, input.position);\n"
					"	return output;\n"
					"}\n"
					"#VS_end\n"
					"#PS_begin\n"
					"#include \"core.hlsl\"\n"
					"struct Input\n"
					"{\n"
					"	float4 fragColor : FragColor;\n"
					"	float2 fragTexCoord : FragTexCoord;\n"
					"};\n"
					"struct Output\n"
					"{\n"
					"	float4 color : SV_Target;\n"
					"};\n"
					"SV_SAMPLER(sam, s0);\n"
					"SV_TEXTURE(_Albedo, t0);\n"
					"Output main(Input input)\n"
					"{\n"
					"	Output output;\n"
					"	float4 texColor = _Albedo.Sample(sam, input.fragTexCoord);\n"
					"	if (texColor.a < 0.05f) discard;\n"
					"	output.color = input.fragColor * texColor;\n"
					"	return output;\n"
					"}\n"
					"#PS_end\n";

				svCheck(g_DefShaderLibrary.createFromString(src));
			}
		}

		return Result_Success;
	}

	Result sprite_renderer_close()
	{
		svCheck(graphics_destroy(g_SpriteBlendState));
		svCheck(graphics_destroy(g_SpriteRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));
		svCheck(graphics_destroy(g_SpriteInputLayoutState));

		svCheck(g_DefShaderLibrary.destroy());

		return Result_Success;
	}

	void renderer_sprite_draw_call(ui32 offset, ui32 size, CommandList cmd)
	{
		graphics_draw_indexed(size * 6u, 1u, offset * 6u, 0u, 0u, cmd);
	}

	void sprite_renderer_draw(const SpriteRendererDrawDesc* desc, CommandList cmd)
	{
		GPUImage* att[] = {
			desc->renderTarget
		};

		GPUImage* texture = nullptr;

		// Bind material
		if (desc->material) {
			ShaderLibrary* shaderLib = desc->material->getShaderLibrary();
			shaderLib->bind(desc->cameraBuffer, cmd);
			desc->material->bind(cmd);
		}
		// Bind default material
		else {
			g_DefShaderLibrary.bind(desc->cameraBuffer, cmd);
		}

		// Get batch data
		SpriteData* pSpriteDataInstance = reinterpret_cast<SpriteData*>(g_SpriteData.alloc());
		SpriteVertex* sprData = pSpriteDataInstance->data;
		SpriteVertex* sprDataIt = sprData;

		ui32 count = desc->count;
		const SpriteInstance* buffer = desc->pInstances;
		const SpriteInstance* initialPtr = buffer;

		while (buffer < initialPtr + count) {

			ui32 j = 0u;
			ui32 currentInstance = buffer - initialPtr;
			ui32 batchSize = BATCH_COUNT;
			if (currentInstance + batchSize > count) {
				batchSize = count - currentInstance;
			}

			// Fill Vertex Buffer
			while (j < batchSize) {

				const SpriteInstance& spr = buffer[j];

				XMVECTOR pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);

				pos0 = XMVector4Transform(pos0, spr.tm);
				pos1 = XMVector4Transform(pos1, spr.tm);
				pos2 = XMVector4Transform(pos2, spr.tm);
				pos3 = XMVector4Transform(pos3, spr.tm);

				*sprDataIt = { vec4f(pos0), {spr.texCoord.x, spr.texCoord.y}, spr.color };
				++sprDataIt;

				*sprDataIt = { vec4f(pos1), {spr.texCoord.z, spr.texCoord.y}, spr.color };
				++sprDataIt;

				*sprDataIt = { vec4f(pos2), {spr.texCoord.x, spr.texCoord.w}, spr.color };
				++sprDataIt;

				*sprDataIt = { vec4f(pos3), {spr.texCoord.z, spr.texCoord.w}, spr.color };
				++sprDataIt;

				j++;
			}

			graphics_buffer_update(g_SpriteVertexBuffer, sprData, batchSize * sizeof(SpriteVertex) * 4u, 0u, cmd);

			// Begin rendering
			graphics_renderpass_begin(g_SpriteRenderPass, att, nullptr, 1.f, 0u, cmd);

			graphics_vertexbuffer_bind(g_SpriteVertexBuffer, 0u, 0u, cmd);
			graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
			graphics_blendstate_bind(g_SpriteBlendState, cmd);
			graphics_inputlayoutstate_bind(g_SpriteInputLayoutState, cmd);
			graphics_sampler_bind(g_SpriteDefSampler, 0u, ShaderType_Pixel, cmd);

			const SpriteInstance* beginBuffer = buffer;
			const SpriteInstance* endBuffer;

			while (buffer < beginBuffer + batchSize) {

				endBuffer = buffer + batchSize;

				texture = buffer->pTexture;
				
				ui32 offset = buffer - beginBuffer;
				while (buffer != endBuffer) {

					if (buffer->pTexture != texture) {
						graphics_image_bind(texture ? texture : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
						texture = buffer->pTexture;
						ui32 batchPos = buffer - beginBuffer;
						renderer_sprite_draw_call(offset, batchPos - offset, cmd);
						offset = batchPos;
					}

					buffer++;
				}

				ui32 batchPos = buffer - beginBuffer;
				graphics_image_bind(texture ? texture : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
				renderer_sprite_draw_call(offset, batchPos - offset, cmd);

			}

			graphics_renderpass_end(cmd);
		}

		g_SpriteData.free(pSpriteDataInstance);

	}

}