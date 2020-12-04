#include "core.h"

#include "sprite_renderer_internal.h"
#include "utils/allocator.h"

namespace sv {

	// Sprite Primitives

	static RenderPass* g_SpriteRenderPass = nullptr;

	static InputLayoutState* g_SpriteInputLayoutState = nullptr;

	static BlendState* g_SpriteBlendState = nullptr;

	static GPUBuffer* g_SpriteIndexBuffer = nullptr;
	static GPUBuffer* g_SpriteVertexBuffer = nullptr;
	static GPUImage* g_SpriteWhiteTexture = nullptr;
	static Sampler* g_SpriteDefSampler = nullptr;

	static ShaderLibrary* g_DefShaderLibrary;

	static SubShaderID g_SubShader_Vertex;
	static SubShaderID g_SubShader_Surface;

	static SpriteRendererContext g_Context[GraphicsLimit_CommandList];

	Result SpriteRenderer_internal::initialize()
	{
		// Init context
		{
			for (ui32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				ctx.pCameraBuffer = nullptr;
				ctx.renderTarget = nullptr;
				ctx.depthStencil = nullptr;
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
			ui32* indexData = new ui32[SPRITE_BATCH_COUNT * 6u];
			{
				ui32 indexCount = SPRITE_BATCH_COUNT * 6u;
				ui32 index = 0u;
				for (ui32 i = 0; i < indexCount; ) {

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
			desc.size = SPRITE_BATCH_COUNT * 6u * sizeof(ui32);
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
			desc.attachments[0].format = Format_R8G8B8A8_SRGB;
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
		// Sprite Shader Utils
		{
			svCheck(graphics_shader_include_write("sprite_vertex", R"(
#name SpriteVertex
#shadertype VertexShader

struct VertexInput {
	float4 position : Position;
	float2 texCoord : TexCoord;
	float4 color : Color;
};

struct VertexOutput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
	float4 position : SV_Position;
};

#userblock SpriteVertex
// User Callbacks
//struct UserVertexOutput : VertexOutput {};
//UserVertexOutput spriteVertex(VertexInput input);

// Main
UserVertexOutput main(VertexInput input)
{
	return spriteVertex(input);
}
			)"));

			svCheck(graphics_shader_include_write("sprite_surface", R"(
#name SpriteSurface
#shadertype PixelShader

struct SurfaceInput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
};

struct SurfaceOutput {
	float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);

#userblock SpriteSurface
// User Callbacks
// struct UserSurfaceInput : SurfaceInput {}
// SurfaceOutput spriteSurface(UserSurfaceInput input);

// Main
SurfaceOutput main(UserSurfaceInput input)
{
	return spriteSurface(input);
}
			)"));
		}
		// Register Shaderlibrary type
		{
			ShaderLibraryTypeDesc desc;
			desc.name = "Sprite";
			desc.subshaderIncludeNames[0] = "sprite_vertex";
			desc.subshaderIncludeNames[1] = "sprite_surface";
			desc.subShaderCount = 2u;
			
			svCheck(matsys_shaderlibrary_type_register(&desc));

			g_SubShader_Vertex = matsys_subshader_get("Sprite", "SpriteVertex");
			g_SubShader_Surface = matsys_subshader_get("Sprite", "SpriteSurface");

			if (g_SubShader_Vertex == ui32_max) return Result_UnknownError;
			if (g_SubShader_Surface == ui32_max) return Result_UnknownError;
		}
		// Default Shader library
		{
			if (result_fail(matsys_shaderlibrary_create_from_binary("SilverEngine/DefaultSprite", &g_DefShaderLibrary))) {

				const char* src = R"(
#name SilverEngine/DefaultSprite
#type Sprite

#begin SpriteVertex

struct UserVertexOutput : VertexOutput {};

SV_DEFINE_CAMERA(b0);

UserVertexOutput spriteVertex(VertexInput input)
{
	UserVertexOutput output;
	output.color = input.color;
	output.texCoord = input.texCoord;
	output.position = mul(input.position, camera.vpm);
	return output;
}

#end

#begin SpriteSurface

struct UserSurfaceInput : SurfaceInput {};

SurfaceOutput spriteSurface(UserSurfaceInput input)
{
	SurfaceOutput output;
	float4 texColor = _Albedo.Sample(sam, input.texCoord);
	output.color = input.color * texColor;
	if (output.color.a < 0.05f) discard;
	return output;
}

#end
				)";

				svCheck(matsys_shaderlibrary_create_from_string(src, &g_DefShaderLibrary));
			}
		}

		return Result_Success;
	}

	Result SpriteRenderer_internal::close()
	{
		svCheck(graphics_destroy(g_SpriteBlendState));
		svCheck(graphics_destroy(g_SpriteRenderPass));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));
		svCheck(graphics_destroy(g_SpriteInputLayoutState));

		svCheck(matsys_shaderlibrary_destroy(g_DefShaderLibrary));

		// deallocate context
		{
			for (ui32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				delete ctx.spriteData;
			}
		}

		return Result_Success;
	}

	void drawCall(ui32 offset, ui32 size, CommandList cmd)
	{
		graphics_draw_indexed(size * 6u, 1u, offset * 6u, 0u, 0u, cmd);
	}

	void SpriteRenderer::enableDepthTest(GPUImage* depthStencil, CommandList cmd)
	{
		g_Context[cmd].depthStencil = depthStencil;
	}

	void SpriteRenderer::disableDepthTest(CommandList cmd)
	{
		g_Context[cmd].depthStencil = nullptr;
	}

	void SpriteRenderer::prepare(GPUImage* renderTarget, CameraBuffer& cameraBuffer, CommandList cmd)
	{
		SpriteRendererContext& ctx = g_Context[cmd];

		ctx.renderTarget = renderTarget;
		ctx.pCameraBuffer = &cameraBuffer;

		graphics_state_unbind(cmd);

		graphics_vertexbuffer_bind(g_SpriteVertexBuffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
		graphics_blendstate_bind(g_SpriteBlendState, cmd);
		graphics_inputlayoutstate_bind(g_SpriteInputLayoutState, cmd);
		graphics_sampler_bind(g_SpriteDefSampler, 0u, ShaderType_Pixel, cmd);
	}

	void SpriteRenderer::draw(Material* material, const SpriteInstance* instances, ui32 count, CommandList cmd)
	{
		SpriteRendererContext& ctx = g_Context[cmd];

		GPUImage* att[] = {
			ctx.renderTarget, ctx.depthStencil
		};

		GPUImage* texture = nullptr;

		// Bind material
		ShaderLibrary* shaderLib;

		if (material) {
			shaderLib = matsys_material_get_shaderlibrary(material);
		}
		// Bind default material
		else {
			shaderLib = g_DefShaderLibrary;
		}

		SV_ASSERT(shaderLib);
		//SV_ASSERT(shaderLib->isType("Sprite"));

		matsys_shaderlibrary_bind_camerabuffer(shaderLib, *ctx.pCameraBuffer, cmd);

		matsys_shaderlibrary_bind_subshader(shaderLib, g_SubShader_Vertex, cmd);
		matsys_shaderlibrary_bind_subshader(shaderLib, g_SubShader_Surface, cmd);

		if (material) {
			matsys_material_bind(material, g_SubShader_Vertex, cmd);
			matsys_material_bind(material, g_SubShader_Surface, cmd);
		}

		// Get batch data
		SpriteData* pSpriteDataInstance = ctx.spriteData;
		SpriteVertex* sprData = pSpriteDataInstance->data;
		SpriteVertex* sprDataIt = sprData;

		const SpriteInstance* buffer = instances;
		const SpriteInstance* initialPtr = buffer;

		while (buffer < initialPtr + count) {

			ui32 j = 0u;
			ui32 currentInstance = ui32(buffer - initialPtr);
			ui32 batchSize = SPRITE_BATCH_COUNT;
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

			const SpriteInstance* beginBuffer = buffer;
			const SpriteInstance* endBuffer;

			while (buffer < beginBuffer + batchSize) {

				endBuffer = buffer + batchSize;

				texture = buffer->pTexture;

				ui32 offset = ui32(buffer - beginBuffer);
				while (buffer != endBuffer) {

					if (buffer->pTexture != texture) {
						graphics_image_bind(texture ? texture : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
						texture = buffer->pTexture;
						ui32 batchPos = ui32(buffer - beginBuffer);
						drawCall(offset, batchPos - offset, cmd);
						offset = batchPos;
					}

					buffer++;
				}

				ui32 batchPos = ui32(buffer - beginBuffer);
				graphics_image_bind(texture ? texture : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
				drawCall(offset, batchPos - offset, cmd);

			}

			graphics_renderpass_end(cmd);
		}
	}

}