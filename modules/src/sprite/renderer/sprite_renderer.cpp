#include "core.h"

#include "sprite_renderer_internal.h"

namespace sv {
	
	static SpriteRendererContext g_Context[GraphicsLimit_CommandList];

	static GPUImage* g_SpriteWhiteTexture = nullptr;
	static Sampler* g_SpriteDefSampler = nullptr;

	static ShaderLibrary* g_DefShaderLibrary;

	static SubShaderID g_SpriteSubShader_Vertex;
	static SubShaderID g_SpriteSubShader_Surface;

	// FORWARD RENDERING

	static RenderPass* g_RenderPass_Geometry = nullptr;
	static RenderPass* g_RenderPass_Geometry3D = nullptr;
	static BlendState* g_BS_Geometry = nullptr;
	static BlendState* g_BS_FirstGeometry = nullptr;
	static DepthStencilState* g_DSS_GeometryOpaque = nullptr;
	static DepthStencilState* g_DSS_GeometryTransparent = nullptr;
	static InputLayoutState* g_ILS_Geometry = nullptr;
	static GPUBuffer* g_SpriteIndexBuffer = nullptr;
	static GPUBuffer* g_SpriteVertexBuffer = nullptr;
	static GPUBuffer* g_SpriteLightBuffer = nullptr;

	Result SpriteRenderer_internal::initialize()
	{
		// Init context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				ctx.spriteData = new SpriteData();
				ctx.lightData = new SpriteLightData();
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

			// Light Buffer

			desc.bufferType = GPUBufferType_Constant;
			desc.CPUAccess = CPUAccess_Write;
			desc.usage = ResourceUsage_Default;
			desc.pData = nullptr;
			desc.size = sizeof(SpriteLightData);

			svCheck(graphics_buffer_create(&desc, &g_SpriteLightBuffer));

			graphics_name_set(g_SpriteVertexBuffer, "Sprite_VertexBuffer");
			graphics_name_set(g_SpriteIndexBuffer, "Sprite_IndexBuffer");
			graphics_name_set(g_SpriteLightBuffer, "Sprite_LightBuffer");
		}
		// Sprite RenderPass
		{
			RenderPassDesc desc;
			desc.attachments.resize(1);

			desc.attachments[0].loadOp = AttachmentOperation_Load;
			desc.attachments[0].storeOp = AttachmentOperation_Store;
			desc.attachments[0].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[0].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[0].format = GBuffer::FORMAT_OFFSCREEN;
			desc.attachments[0].initialLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].layout = GPUImageLayout_RenderTarget;
			desc.attachments[0].finalLayout = GPUImageLayout_RenderTarget;
			desc.attachments[0].type = AttachmentType_RenderTarget;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Geometry));
			graphics_name_set(g_RenderPass_Geometry, "Sprite_GeometryPass");

			desc.attachments.resize(2u);

			desc.attachments[1].loadOp = AttachmentOperation_Load;
			desc.attachments[1].storeOp = AttachmentOperation_Store;
			desc.attachments[1].stencilLoadOp = AttachmentOperation_DontCare;
			desc.attachments[1].stencilStoreOp = AttachmentOperation_DontCare;
			desc.attachments[1].format = GBuffer::FORMAT_DEPTHSTENCIL;
			desc.attachments[1].initialLayout = GPUImageLayout_DepthStencil;
			desc.attachments[1].layout = GPUImageLayout_DepthStencil;
			desc.attachments[1].finalLayout = GPUImageLayout_DepthStencil;
			desc.attachments[1].type = AttachmentType_DepthStencil;

			svCheck(graphics_renderpass_create(&desc, &g_RenderPass_Geometry3D));
			graphics_name_set(g_RenderPass_Geometry3D, "Sprite_GeometryPass3D");
		}
		// Sprite Shader Input Layout
		{
			InputLayoutStateDesc inputLayout;
			inputLayout.slots.push_back({ 0u, sizeof(SpriteVertex), false });

			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT });
			inputLayout.elements.push_back({ "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT });
			inputLayout.elements.push_back({ "Color", 0u, 0u, 6u * sizeof(float), Format_R8G8B8A8_UNORM });

			svCheck(graphics_inputlayoutstate_create(&inputLayout, &g_ILS_Geometry));
			graphics_name_set(g_ILS_Geometry, "Sprite_GeometryInputLayout");
		}
		// Blend State
		{
			BlendStateDesc blendState;
			blendState.attachments.resize(1);
			blendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			blendState.attachments[0].blendEnabled = true;
			blendState.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			blendState.attachments[0].dstColorBlendFactor = BlendFactor_DstAlpha;
			blendState.attachments[0].colorBlendOp = BlendOperation_Add;
			blendState.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].alphaBlendOp = BlendOperation_Add;
			blendState.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&blendState, &g_BS_Geometry));

			blendState.attachments[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			blendState.attachments[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			blendState.attachments[0].colorBlendOp = BlendOperation_Add;
			blendState.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			blendState.attachments[0].alphaBlendOp = BlendOperation_Add;
			blendState.attachments[0].colorWriteMask = ColorComponent_All;

			svCheck(graphics_blendstate_create(&blendState, &g_BS_FirstGeometry));

			graphics_name_set(g_BS_Geometry, "Sprite_GeometryBlendState");
			graphics_name_set(g_BS_FirstGeometry, "Sprite_FirstGeometryBlendState");
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

		// Sprite 3D Depth Test
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = true;
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_Less;
			desc.stencilTestEnabled = false;

			svCheck(graphics_depthstencilstate_create(&desc, &g_DSS_GeometryOpaque));
			graphics_name_set(g_DSS_GeometryOpaque, "DSS_SpriteGeometry3D");

			desc.depthWriteEnabled = false;

			svCheck(graphics_depthstencilstate_create(&desc, &g_DSS_GeometryTransparent));
			graphics_name_set(g_DSS_GeometryTransparent, "DSS_SpriteTransparent3D");
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

// Main
UserVertexOutput main(VertexInput input)
{
	return spriteVertex(input);
}
			)"));

			svCheck(graphics_shader_include_write("sprite_surface", R"(
#name SpriteSurface
#shadertype PixelShader

struct Light {
	u32		type;
	float3	position;
	float3	color;
	float	range;
	float	intensity;
	float	smoothness;
};

struct SurfaceInput {
	float4 color : FragColor;
	float2 texCoord : FragTexCoord;
};

struct SurfaceOutput {
	float4 color : SV_Target;
};

SV_SAMPLER(sam, s0);
SV_TEXTURE(_Albedo, t0);
SV_CONSTANT_BUFFER(LightBuffer, b0) {

	float3 ambientLight;
	u32 lightCount;
	Light lights[10];

};

// Lighting functions

float3 lightingPoint(Light light, float3 fragPosition) 
{
	float distance = length(light.position.xyz - fragPosition);
	
	float att = 1.f - smoothstep(light.range * light.smoothness, light.range, distance);
	return light.color * att * light.intensity;
}

float3 lightingResult(float3 fragPosition) 
{
	float3 lightColor = 0.f;
	foreach (i, lightCount) {

		Light light = lights[i];

		switch(light.type) {
		case SV_LIGHT_TYPE_POINT:
			lightColor += lightingPoint(light, fragPosition);
			break;
		}
	}

	return max(lightColor, ambientLight);
}

#userblock SpriteSurface

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

			g_SpriteSubShader_Vertex = matsys_subshader_get("Sprite", "SpriteVertex");
			g_SpriteSubShader_Surface = matsys_subshader_get("Sprite", "SpriteSurface");

			if (g_SpriteSubShader_Vertex == u32_max) return Result_UnknownError;
			if (g_SpriteSubShader_Surface == u32_max) return Result_UnknownError;
		}
		// Default Shader library
		{
			if (result_fail(matsys_shaderlibrary_create_from_binary("SilverEngine/DefaultSprite", &g_DefShaderLibrary))) {

				const char* src = R"(
#name SilverEngine/DefaultSprite
#type Sprite

#begin SpriteVertex

struct UserVertexOutput : VertexOutput {
	float3 fragPos : FragPosition;
};

SV_DEFINE_CAMERA(b0);

UserVertexOutput spriteVertex(VertexInput input)
{
	UserVertexOutput output;
	output.color = input.color;
	output.texCoord = input.texCoord;
	output.fragPos = input.position.xyz;
	output.position = mul(input.position, camera.vpm);
	return output;
}

#end

#begin SpriteSurface

struct UserSurfaceInput : SurfaceInput {
	float3 position : FragPosition;
};

SurfaceOutput spriteSurface(UserSurfaceInput input)
{
	SurfaceOutput output;
	float4 texColor = _Albedo.Sample(sam, input.texCoord);
	if (texColor.a < 0.05f) discard;
	
	// Apply color
	output.color = input.color * texColor;
	
	// Apply lights
	output.color.xyz *= lightingResult(input.position);

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
		svCheck(graphics_destroy(g_SpriteWhiteTexture));
		svCheck(graphics_destroy(g_SpriteDefSampler));

		svCheck(matsys_shaderlibrary_destroy(g_DefShaderLibrary));

		// FORWARD RENDERING
		svCheck(graphics_destroy(g_BS_Geometry));
		svCheck(graphics_destroy(g_BS_FirstGeometry));
		svCheck(graphics_destroy(g_SpriteLightBuffer));
		svCheck(graphics_destroy(g_RenderPass_Geometry));
		svCheck(graphics_destroy(g_SpriteVertexBuffer));
		svCheck(graphics_destroy(g_ILS_Geometry));
		svCheck(graphics_destroy(g_SpriteIndexBuffer));

		// deallocate context
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

				SpriteRendererContext& ctx = g_Context[i];
				delete ctx.spriteData;
				delete ctx.lightData;
			}
		}

		return Result_Success;
	}

	inline void bindMaterial(Material* material, CameraBuffer& cameraBuffer, CommandList cmd)
	{
		if (material) {
			ShaderLibrary* lib = matsys_material_get_shaderlibrary(material);

			matsys_shaderlibrary_bind_camerabuffer(lib, cameraBuffer, cmd);

			matsys_shaderlibrary_bind_subshader(lib, g_SpriteSubShader_Vertex, cmd);
			matsys_shaderlibrary_bind_subshader(lib, g_SpriteSubShader_Surface, cmd);

			matsys_material_bind(material, g_SpriteSubShader_Vertex, cmd);
			matsys_material_bind(material, g_SpriteSubShader_Surface, cmd);
		}
		else {
			matsys_shaderlibrary_bind_camerabuffer(g_DefShaderLibrary, cameraBuffer, cmd);

			matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SpriteSubShader_Vertex, cmd);
			matsys_shaderlibrary_bind_subshader(g_DefShaderLibrary, g_SpriteSubShader_Surface, cmd);
		}
	}

	void SpriteRenderer::drawSprites(const SpriteRendererDrawDesc* desc, CommandList cmd)
	{
		if (desc->spriteCount == 0u) return;

		SpriteRendererContext& ctx = g_Context[cmd];

		// Prepare
		graphics_event_begin("Sprite_GeometryPass", cmd);
		graphics_state_unbind(cmd);

		graphics_topology_set(GraphicsTopology_Triangles, cmd);
		graphics_vertexbuffer_bind(g_SpriteVertexBuffer, 0u, 0u, cmd);
		graphics_indexbuffer_bind(g_SpriteIndexBuffer, 0u, cmd);
		graphics_constantbuffer_bind(g_SpriteLightBuffer, 0u, ShaderType_Pixel, cmd);
		graphics_blendstate_bind(g_BS_FirstGeometry, cmd);
		graphics_inputlayoutstate_bind(g_ILS_Geometry, cmd);
		graphics_sampler_bind(g_SpriteDefSampler, 0u, ShaderType_Pixel, cmd);

		if (desc->depthTest) {
			if (desc->transparent)
				graphics_depthstencilstate_bind(g_DSS_GeometryTransparent, cmd);
			else
				graphics_depthstencilstate_bind(g_DSS_GeometryOpaque, cmd);
		}

		GPUImage* att[2];
		att[0] = desc->pCameraData->pGBuffer->offscreen;
		att[1] = desc->pCameraData->pGBuffer->depthStencil;

		XMVECTOR pos0, pos1, pos2, pos3;

		const LightInstance* lightIt;
		const LightInstance* lightEnd;

		if (desc->lightCount) {
			lightIt = desc->pLights;
			lightEnd = lightIt + desc->lightCount;
		}
		else {
			lightIt = nullptr;
			lightEnd = nullptr;
		}

		bool firstDraw = true;
		ctx.lightData->ambient = desc->ambientLight;

		SpriteLightData& ld = *ctx.lightData;

		do {

			// Prepare light data
			{
				u32 currentLight = lightIt ? (lightIt - desc->pLights) : 0u;

				ld.lightCount = std::min(SPRITE_LIGHT_COUNT, desc->lightCount - currentLight);

				for (u32 i = 0u; i < ld.lightCount; ++i) {

					SpriteLight& sl = ld.lights[i];
					const LightInstance& light = desc->pLights[currentLight + i];

					sl.type = light.type;
					sl.intensity = light.intensity * desc->lightMult;
					sl.position = light.point.position;
					sl.color = light.color;
					sl.range = light.point.range;
					sl.smoothness = 1.f - light.point.smoothness;
				}

				if (lightIt) {
					lightIt += ld.lightCount;
				}

				graphics_buffer_update(g_SpriteLightBuffer, &ld, sizeof(SpriteLightData), 0u, cmd);
			}

			// Batch data, used to update the vertex buffer
			SpriteVertex* batchIt = ctx.spriteData->data;

			// Used to draw
			u32 instanceCount;

			const SpriteInstance* it = desc->pSprites;
			const SpriteInstance* end = it + desc->spriteCount;

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

					pos0 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
					pos1 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);
					pos2 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
					pos3 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);


					pos0 = XMVector4Transform(pos0, spr.tm);
					pos1 = XMVector4Transform(pos1, spr.tm);
					pos2 = XMVector4Transform(pos2, spr.tm);
					pos3 = XMVector4Transform(pos3, spr.tm);

					*batchIt = { v4_f32(pos0), {spr.texCoord.x, spr.texCoord.y}, spr.color };
					++batchIt;

					*batchIt = { v4_f32(pos1), {spr.texCoord.z, spr.texCoord.y}, spr.color };
					++batchIt;

					*batchIt = { v4_f32(pos2), {spr.texCoord.x, spr.texCoord.w}, spr.color };
					++batchIt;

					*batchIt = { v4_f32(pos3), {spr.texCoord.z, spr.texCoord.w}, spr.color };
					++batchIt;
				}

				// Draw

				graphics_buffer_update(g_SpriteVertexBuffer, ctx.spriteData->data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

				graphics_renderpass_begin(desc->depthTest ? g_RenderPass_Geometry3D : g_RenderPass_Geometry, att, nullptr, 1.f, 0u, cmd);

				end = it;
				it -= instanceCount;

				const SpriteInstance* begin = it;
				const SpriteInstance* last = it;

				graphics_image_bind(it->image ? it->image : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
				bindMaterial(it->material, *desc->pCameraData->pCameraBuffer, cmd);

				while (it != end) {

					if (it->material != last->material || it->image != last->image) {

						u32 spriteCount = u32(it - last);
						u32 startVertex = u32(last - begin) * 4u;

						graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

						if (it->image != last->image) {
							graphics_image_bind(it->image ? it->image : g_SpriteWhiteTexture, 0u, ShaderType_Pixel, cmd);
						}
						if (it->material != last->material) {
							bindMaterial(it->material, *desc->pCameraData->pCameraBuffer, cmd);
							
							// TODO: For some reason this image doesn't bind correctly
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

				end = desc->pSprites + desc->spriteCount;
				batchIt = ctx.spriteData->data;
			}

			if (firstDraw)
			{
				ctx.lightData->ambient = Color3f::Black();
				graphics_blendstate_bind(g_BS_Geometry, cmd);
				firstDraw = false;
			}

		} while (lightIt != lightEnd);
	}

	void SpriteRenderer::drawSameSprites(const SpriteRendererSameDrawDesc* desc, CommandList cmd)
	{
	}
}