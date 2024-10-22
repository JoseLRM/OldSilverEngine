#include "defines.h"

#include "core/renderer/renderer_internal.h"
#include "core/mesh.h"
#include "debug/console.h"

#include "shared_headers/lighting.h"
#include "shared_headers/ssao.h"

namespace sv {

    RendererState* renderer = nullptr;

    // SHADER COMPILATION

#define COMPILE_SHADER(type, shader, path, alwais_compile) SV_CHECK(graphics_shader_compile_fastbin_from_file(path, type, &shader, "$system/shaders/" path, alwais_compile));
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)
#define COMPILE_CS(shader, path) COMPILE_SHADER(ShaderType_Compute, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)
#define COMPILE_CS_(shader, path) COMPILE_SHADER(ShaderType_Compute, shader, path, true)

    static bool compile_shaders()
    {
		auto& gfx = renderer->gfx;

		COMPILE_VS(gfx.vs_text, "text.hlsl");
		COMPILE_PS(gfx.ps_text, "text.hlsl");

		COMPILE_VS(gfx.vs_sprite, "sprite/default.hlsl");
		COMPILE_PS(gfx.ps_sprite, "sprite/default.hlsl");

		COMPILE_VS(gfx.vs_terrain, "terrain.hlsl");
		COMPILE_PS(gfx.ps_terrain, "terrain.hlsl");

		COMPILE_VS(gfx.vs_mesh_default, "mesh_default.hlsl");
		COMPILE_PS(gfx.ps_mesh_default, "mesh_default.hlsl");

		COMPILE_VS(gfx.vs_sky, "skymapping.hlsl");
		COMPILE_PS(gfx.ps_sky, "skymapping.hlsl");

		COMPILE_CS_(gfx.cs_gaussian_blur_float4, "postprocessing/gaussian_blur_float4.hlsl");
		COMPILE_CS_(gfx.cs_gaussian_blur_float, "postprocessing/gaussian_blur_float.hlsl");
		COMPILE_CS_(gfx.cs_bloom_threshold, "postprocessing/bloom_threshold.hlsl");
		COMPILE_CS_(gfx.cs_bloom_addition, "postprocessing/bloom_addition.hlsl");
		COMPILE_CS(gfx.cs_ssao, "postprocessing/ssao.hlsl");
		COMPILE_CS(gfx.cs_ssao_addition, "postprocessing/ssao_addition.hlsl");

		COMPILE_VS(gfx.vs_shadow, "shadow_mapping.hlsl");

		return true;
    }

    // RENDERPASSES CREATION

#define CREATE_RENDERPASS(name, renderpass) SV_CHECK(graphics_renderpass_create(&desc, &renderpass));

    static bool create_renderpasses()
    {
		auto& gfx = renderer->gfx;
	
		RenderPassDesc desc;
		AttachmentDesc att[GraphicsLimit_Attachment];
		desc.pAttachments = att;

		// Offscreen pass
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		desc.attachmentCount = 1u;
		CREATE_RENDERPASS("OffscreenRenderpass", gfx.renderpass_off);

		// Shadow mapping pass
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = GBUFFER_DEPTH_FORMAT;
		att[0].initialLayout = GPUImageLayout_DepthStencil;
		att[0].layout = GPUImageLayout_DepthStencil;
		att[0].finalLayout = GPUImageLayout_DepthStencil;
		att[0].type = AttachmentType_DepthStencil;

		desc.attachmentCount = 1u;
		CREATE_RENDERPASS("DepthRenderpass", gfx.renderpass_shadow_mapping);
		
		// World
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		att[1].loadOp = AttachmentOperation_Load;
		att[1].storeOp = AttachmentOperation_Store;
		att[1].stencilLoadOp = AttachmentOperation_Load;
		att[1].stencilStoreOp = AttachmentOperation_Store;
		att[1].format = GBUFFER_DEPTH_FORMAT;
		att[1].initialLayout = GPUImageLayout_DepthStencil;
		att[1].layout = GPUImageLayout_DepthStencil;
		att[1].finalLayout = GPUImageLayout_DepthStencil;
		att[1].type = AttachmentType_DepthStencil;

		desc.attachmentCount = 2u;
		CREATE_RENDERPASS("WorldRenderPass", gfx.renderpass_world);

		// GBuffer
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].stencilLoadOp = AttachmentOperation_DontCare;
		att[0].stencilStoreOp = AttachmentOperation_DontCare;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		att[1].loadOp = AttachmentOperation_Load;
		att[1].storeOp = AttachmentOperation_Store;
		att[1].stencilLoadOp = AttachmentOperation_DontCare;
		att[1].stencilStoreOp = AttachmentOperation_DontCare;
		att[1].format = GBUFFER_NORMAL_FORMAT;
		att[1].initialLayout = GPUImageLayout_RenderTarget;
		att[1].layout = GPUImageLayout_RenderTarget;
		att[1].finalLayout = GPUImageLayout_RenderTarget;
		att[1].type = AttachmentType_RenderTarget;

		att[2].loadOp = AttachmentOperation_Load;
		att[2].storeOp = AttachmentOperation_Store;
		att[2].stencilLoadOp = AttachmentOperation_DontCare;
		att[2].stencilStoreOp = AttachmentOperation_DontCare;
		att[2].format = GBUFFER_EMISSION_FORMAT;
		att[2].initialLayout = GPUImageLayout_RenderTarget;
		att[2].layout = GPUImageLayout_RenderTarget;
		att[2].finalLayout = GPUImageLayout_RenderTarget;
		att[2].type = AttachmentType_RenderTarget;

		att[3].loadOp = AttachmentOperation_Load;
		att[3].storeOp = AttachmentOperation_Store;
		att[3].stencilLoadOp = AttachmentOperation_Load;
		att[3].stencilStoreOp = AttachmentOperation_Store;
		att[3].format = GBUFFER_DEPTH_FORMAT;
		att[3].initialLayout = GPUImageLayout_DepthStencil;
		att[3].layout = GPUImageLayout_DepthStencil;
		att[3].finalLayout = GPUImageLayout_DepthStencil;
		att[3].type = AttachmentType_DepthStencil;

		desc.attachmentCount = 4u;
		CREATE_RENDERPASS("GBufferRenderPass", gfx.renderpass_gbuffer);

		return true;
    }

    // BUFFER CREATION

    static bool create_buffers()
    {
		auto& gfx = renderer->gfx;
	
		GPUBufferDesc desc;

		// Environment
		{
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_EnvironmentData);
			desc.data = nullptr;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_environment));
		}

		// SSAO
		{
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_SSAOData);
			desc.data = nullptr;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_ssao));
		}

		// Gaussian blur
		{
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_GaussianBlurData);
			desc.data = nullptr;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_gaussian_blur));
		}

		// Bloom threshold
		{
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_BloomThresholdData);
			desc.data = nullptr;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_bloom_threshold));
		}

		// Camera buffer
		{
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_CameraData);
			desc.data = nullptr;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_camera));
		}

		// set text index data
		{
			constexpr u32 index_count = TEXT_BATCH_COUNT * 6u;
			u16* index_data = (u16*)SV_ALLOCATE_MEMORY(sizeof(u16) * index_count, "Renderer");

			for (u32 i = 0u; i < TEXT_BATCH_COUNT; ++i) {

				u32 currenti = i * 6u;
				u32 currentv = i * 4u;

				index_data[currenti + 0u] = u16(0 + currentv);
				index_data[currenti + 1u] = u16(1 + currentv);
				index_data[currenti + 2u] = u16(2 + currentv);

				index_data[currenti + 3u] = u16(1 + currentv);
				index_data[currenti + 4u] = u16(3 + currentv);
				index_data[currenti + 5u] = u16(2 + currentv);
			}

			// 10.922 characters
			desc.index_type = IndexType_16;
			desc.buffer_type = GPUBufferType_Index;
			desc.usage = ResourceUsage_Static;
			desc.cpu_access = CPUAccess_None;
			desc.size = sizeof(u16) * index_count;
			desc.data = index_data;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.ibuffer_text));
			graphics_name_set(gfx.ibuffer_text, "Text_IndexBuffer");

			SV_FREE_MEMORY(index_data);
		}

		// Sprite index buffer
		{
			u32* index_data = (u32*)SV_ALLOCATE_MEMORY(sizeof(u32) * SPRITE_BATCH_COUNT * 6u, "Renderer");
			{
				u32 indexCount = SPRITE_BATCH_COUNT * 6u;
				u32 index = 0u;
				for (u32 i = 0; i < indexCount; ) {

					index_data[i++] = index;
					index_data[i++] = index + 1;
					index_data[i++] = index + 2;
					index_data[i++] = index + 1;
					index_data[i++] = index + 3;
					index_data[i++] = index + 2;

					index += 4u;
				}
			}

			desc.buffer_type = GPUBufferType_Index;
			desc.cpu_access = CPUAccess_None;
			desc.usage = ResourceUsage_Static;
			desc.data = index_data;
			desc.size = SPRITE_BATCH_COUNT * 6u * sizeof(u32);
			desc.index_type = IndexType_32;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.ibuffer_sprite));
			SV_FREE_MEMORY(index_data);

			graphics_name_set(gfx.ibuffer_sprite, "Sprite_IndexBuffer");
		}

		// Mesh
		{
			desc.data = nullptr;
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Dynamic;
			desc.cpu_access = CPUAccess_Write;

			desc.size = sizeof(GPU_MeshInstanceData);
			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance));

			desc.size = sizeof(Material);
			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_material));

			desc.size = sizeof(GPU_LightData);
			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_light_instances));
		}

		// Mesh
		{
			desc.data = nullptr;
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Dynamic;
			desc.cpu_access = CPUAccess_Write;

			desc.size = sizeof(GPU_TerrainInstanceData);
			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_terrain_instance));
		}

		// Shadow mapping
		{
			desc.data = NULL;
			desc.buffer_type = GPUBufferType_Constant;
			desc.usage = ResourceUsage_Dynamic;
			desc.cpu_access = CPUAccess_Write;
			desc.size = sizeof(GPU_ShadowMappingData);

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_shadow_mapping));

			desc.size = sizeof(GPU_ShadowData);
			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_shadow_data));
		}

		// Sky box
		{
			f32 box[] = {
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,

				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f, -1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,

				-1.0f,  1.0f, -1.0f,
				1.0f,  1.0f, -1.0f,
				1.0f,  1.0f,  1.0f,
				1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,

				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f, -1.0f,
				1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				1.0f, -1.0f,  1.0f
			};


			desc.data = box;
			desc.buffer_type = GPUBufferType_Vertex;
			desc.size = sizeof(v3_f32) * 36u;
			desc.usage = ResourceUsage_Static;
			desc.cpu_access = CPUAccess_None;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.vbuffer_skybox));

			desc.data = nullptr;
			desc.buffer_type = GPUBufferType_Constant;
			desc.size = sizeof(XMMATRIX);
			desc.usage = ResourceUsage_Default;
			desc.cpu_access = CPUAccess_Write;

			SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_skybox));
		}

		return true;
    }

    // IMAGE CREATION

    static bool create_images()
    {
		auto& gfx = renderer->gfx;
	
		GPUImageDesc desc;

		// TODO: resolution variable
		constexpr u32 RES_WIDTH = 1920u;
		constexpr u32 RES_HEIGHT = 1080u;

		// GBuffer
		{
			desc.width = RES_WIDTH;
			desc.height = RES_HEIGHT;
	    
			desc.format = OFFSCREEN_FORMAT;
			desc.layout = GPUImageLayout_RenderTarget;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource | GPUImageType_UnorderedAccessView;
			SV_CHECK(graphics_image_create(&desc, &gfx.offscreen));

			desc.format = GBUFFER_DEPTH_FORMAT;
			desc.layout = GPUImageLayout_DepthStencil;
			desc.type = GPUImageType_DepthStencil | GPUImageType_ShaderResource;
			SV_CHECK(graphics_image_create(&desc, &gfx.gbuffer_depthstencil));

			desc.format = GBUFFER_NORMAL_FORMAT;
			desc.layout = GPUImageLayout_RenderTarget;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
			SV_CHECK(graphics_image_create(&desc, &gfx.gbuffer_normal));

			desc.format = GBUFFER_EMISSION_FORMAT;
			desc.layout = GPUImageLayout_RenderTarget;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
			SV_CHECK(graphics_image_create(&desc, &gfx.gbuffer_emission));

			desc.format = GBUFFER_SSAO_FORMAT;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_UnorderedAccessView | GPUImageType_ShaderResource;
			SV_CHECK(graphics_image_create(&desc, &gfx.gbuffer_ssao));
		}
	
		// Aux Image
		{
			desc.data = nullptr;
			desc.size = 0u;
			desc.format = OFFSCREEN_FORMAT;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource | GPUImageType_UnorderedAccessView;
			desc.usage = ResourceUsage_Static;
			desc.cpu_access = CPUAccess_None;
			desc.width = 1920u / 8;
			desc.height = 1080u / 8;

			SV_CHECK(graphics_image_create(&desc, &gfx.image_aux0));
			SV_CHECK(graphics_image_create(&desc, &gfx.image_aux1));

			desc.type = GPUImageType_ShaderResource | GPUImageType_UnorderedAccessView;
			desc.width = 1920;
			desc.height = 1080;
			desc.format = GBUFFER_SSAO_FORMAT;

			SV_CHECK(graphics_image_create(&desc, &gfx.image_ssao_aux));
		}

		// White Image
		{
			u8 bytes[4];
			for (u32 i = 0; i < 4; ++i) bytes[i] = 255u;

			desc.data = bytes;
			desc.size = sizeof(u8) * 4u;
			desc.format = Format_R8G8B8A8_UNORM;
			desc.layout = GPUImageLayout_ShaderResource;
			desc.type = GPUImageType_ShaderResource;
			desc.usage = ResourceUsage_Static;
			desc.cpu_access = CPUAccess_None;
			desc.width = 1u;
			desc.height = 1u;

			SV_CHECK(graphics_image_create(&desc, &gfx.image_white));
		}
		
		return true;
    }

    // STATE CREATION

    static bool create_states()
    {
		auto& gfx = renderer->gfx;
	
		// Create InputlayoutStates
		{
			InputSlotDesc slots[GraphicsLimit_InputSlot];
			InputElementDesc elements[GraphicsLimit_InputElement];
			InputLayoutStateDesc desc;
			desc.pSlots = slots;
			desc.pElements = elements;

			// TEXT
			slots[0] = { 0u, sizeof(TextVertex), false };

			elements[0] = { "Position", 0u, 0u, sizeof(f32) * 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "TexCoord", 0u, 0u, sizeof(f32) * 4u, Format_R32G32_FLOAT };
			elements[2] = { "Color", 0u, 0u, sizeof(f32) * 6u, Format_R8G8B8A8_UNORM };

			desc.elementCount = 3u;
			desc.slotCount = 1u;
			SV_CHECK(graphics_inputlayoutstate_create(&desc, &gfx.ils_text));

			// SPRITE
			slots[0] = { 0u, sizeof(SpriteVertex), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "TexCoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT };
			elements[2] = { "Color", 0u, 0u, 6u * sizeof(f32), Format_R8G8B8A8_UNORM };
			elements[3] = { "EmissiveColor", 0u, 0u, 7u * sizeof(f32), Format_R8G8B8A8_UNORM };

			desc.elementCount = 4u;
			desc.slotCount = 1u;
			SV_CHECK(graphics_inputlayoutstate_create(&desc, &gfx.ils_sprite));

			// MESH
			slots[0] = { 0u, sizeof(MeshVertex), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
			elements[1] = { "Normal", 0u, 0u, 3u * sizeof(f32), Format_R32G32B32_FLOAT };
			elements[2] = { "Tangent", 0u, 0u, 6u * sizeof(f32), Format_R32G32B32A32_FLOAT };
			elements[3] = { "Texcoord", 0u, 0u, 10u * sizeof(f32), Format_R32G32_FLOAT };

			desc.elementCount = 4u;
			desc.slotCount = 1u;
			SV_CHECK(graphics_inputlayoutstate_create(&desc, &gfx.ils_mesh));

			// TERRAIN
			slots[0] = { 0u, sizeof(TerrainVertex), false };

			elements[0] = { "Height", 0u, 0u, 0u, Format_R32_FLOAT };
			elements[1] = { "Normal", 0u, 0u, 1u * sizeof(f32), Format_R32G32B32_FLOAT };
			elements[2] = { "Texcoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT };

			desc.elementCount = 3u;
			desc.slotCount = 1u;
			SV_CHECK(graphics_inputlayoutstate_create(&desc, &gfx.ils_terrain));

			// SKY BOX
			slots[0] = { 0u, sizeof(v3_f32), false };

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };

			desc.elementCount = 1u;
			desc.slotCount = 1u;
			SV_CHECK(graphics_inputlayoutstate_create(&desc, &gfx.ils_sky));
		}

		// Create BlendStates
		{
			BlendAttachmentDesc att[GraphicsLimit_AttachmentRT];
			BlendStateDesc desc;
			desc.pAttachments = att;

			// TRANSPARENT
			att[0].blendEnabled = true;
			att[0].srcColorBlendFactor = BlendFactor_SrcAlpha;
			att[0].dstColorBlendFactor = BlendFactor_OneMinusSrcAlpha;
			att[0].colorBlendOp = BlendOperation_Add;
			att[0].srcAlphaBlendFactor = BlendFactor_One;
			att[0].dstAlphaBlendFactor = BlendFactor_Zero;
			att[0].alphaBlendOp = BlendOperation_Add;
			att[0].colorWriteMask = ColorComponent_RGB;
			att[1] = att[0];
			att[2] = att[0];

			desc.attachmentCount = 3u;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			SV_CHECK(graphics_blendstate_create(&desc, &gfx.bs_transparent));

			// MESH
			foreach(i, GraphicsLimit_AttachmentRT) {
				att[i].blendEnabled = false;
				att[i].colorWriteMask = ColorComponent_All;
			}

			desc.attachmentCount = GraphicsLimit_AttachmentRT;

			SV_CHECK(graphics_blendstate_create(&desc, &gfx.bs_mesh));
		}

		// Create DepthStencilStates
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = true;
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_Less;
			desc.stencilTestEnabled = false;
			
			graphics_depthstencilstate_create(&desc, &gfx.dss_default_depth);

			desc.depthWriteEnabled = false;
			graphics_depthstencilstate_create(&desc, &gfx.dss_read_depth);
			
			desc.depthWriteEnabled = true;
			desc.depthCompareOp = CompareOperation_Never;
			graphics_depthstencilstate_create(&desc, &gfx.dss_write_depth);
		}

		// Create RasterizerStates
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.cullMode = RasterizerCullMode_Back;
			desc.clockwise = true;

			graphics_rasterizerstate_create(&desc, &gfx.rs_back_culling);

			desc.cullMode = RasterizerCullMode_Front;
			graphics_rasterizerstate_create(&desc, &gfx.rs_front_culling);

			desc.wireframe = true;
			desc.cullMode = RasterizerCullMode_None;
			desc.clockwise = true;
			graphics_rasterizerstate_create(&desc, &gfx.rs_wireframe);

			desc.wireframe = true;
			desc.cullMode = RasterizerCullMode_Front;
			desc.clockwise = true;
			graphics_rasterizerstate_create(&desc, &gfx.rs_wireframe_front_culling);

			desc.wireframe = true;
			desc.cullMode = RasterizerCullMode_Back;
			desc.clockwise = true;
			graphics_rasterizerstate_create(&desc, &gfx.rs_wireframe_back_culling);
		}

		return true;
    }

    // SAMPLER CREATION

    static bool create_samplers()
    {
		auto& gfx = renderer->gfx;
	
		SamplerDesc desc;

		desc.addressModeU = SamplerAddressMode_Wrap;
		desc.addressModeV = SamplerAddressMode_Wrap;
		desc.addressModeW = SamplerAddressMode_Wrap;
		desc.minFilter = SamplerFilter_Linear;
		desc.magFilter = SamplerFilter_Linear;
		SV_CHECK(graphics_sampler_create(&desc, &gfx.sampler_def_linear));

		desc.addressModeU = SamplerAddressMode_Wrap;
		desc.addressModeV = SamplerAddressMode_Wrap;
		desc.addressModeW = SamplerAddressMode_Wrap;
		desc.minFilter = SamplerFilter_Nearest;
		desc.magFilter = SamplerFilter_Nearest;
		SV_CHECK(graphics_sampler_create(&desc, &gfx.sampler_def_nearest));

		desc.addressModeU = SamplerAddressMode_Mirror;
		desc.addressModeV = SamplerAddressMode_Mirror;
		desc.addressModeW = SamplerAddressMode_Mirror;
		desc.minFilter = SamplerFilter_Linear;
		desc.magFilter = SamplerFilter_Linear;
		SV_CHECK(graphics_sampler_create(&desc, &gfx.sampler_linear_mirror));

		desc.addressModeU = SamplerAddressMode_Clamp;
		desc.addressModeV = SamplerAddressMode_Clamp;
		desc.addressModeW = SamplerAddressMode_Clamp;
		desc.minFilter = SamplerFilter_Linear;
		desc.magFilter = SamplerFilter_Linear;
		SV_CHECK(graphics_sampler_create(&desc, &gfx.sampler_linear_clamp));

		return true;
    }

    // RENDERER RUNTIME
    
    bool _imrend_initialize();
    void _imrend_close();

#if SV_EDITOR
	void display_debug_renderer();
#endif

    bool _renderer_initialize()
    {
		renderer = SV_ALLOCATE_STRUCT(RendererState, "Renderer");
	
		SV_CHECK(compile_shaders());
		SV_CHECK(create_renderpasses());
		SV_CHECK(create_samplers());
		SV_CHECK(create_buffers());
		SV_CHECK(create_images());
		SV_CHECK(create_states());

		SV_CHECK(_imrend_initialize());

		// Create default fonts
		SV_CHECK(font_create(renderer->font_opensans, "$system/fonts/OpenSans/OpenSans-Regular.ttf", 128.f, 0u));
		SV_CHECK(font_create(renderer->font_console, "$system/fonts/Cousine/Cousine-Regular.ttf", 128.f, 0u));

#if SV_EDITOR
		event_register("display_gui", display_debug_renderer, 0u);
#endif

		return true;
    }

    bool _renderer_close()
    {
		auto& gfx = renderer->gfx;

		_imrend_close();
	
		if (renderer) {
			// Free graphics objects
			graphics_destroy_struct(&gfx, sizeof(gfx));

			// Deallocte batch memory
			{
				for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

					if (renderer->batch_data[i]) {
						SV_FREE_MEMORY(renderer->batch_data[i]);
						renderer->batch_data[i] = nullptr;
					}
				}
			}

			// Free shadow maps
			for (ShadowMapRef ref : renderer->shadow_maps) {
				foreach(i, 4)
					graphics_destroy(ref.image[i]);
			}
			renderer->shadow_maps.clear();

			font_destroy(renderer->font_opensans);
			font_destroy(renderer->font_console);

			SV_FREE_STRUCT(renderer);
		}

		return true;
    }

    void _renderer_begin()
    {
		graphics_commandlist_begin();

		// TODO: Free unused shadow maps
    }

    void _renderer_end()
    {
		auto& gfx = renderer->gfx;
	
		graphics_present_image(gfx.offscreen, GPUImageLayout_RenderTarget);
    }

    bool load_skymap_image(const char* filepath, GPUImage** pimage)
    {
		Color* data;
		u32 w, h;
		SV_CHECK(load_image(filepath, (void**)& data, &w, &h));

		u32 image_width = w / 4u;
		u32 image_height = h / 3u;

		Color* images[6u] = {};
		Color* mem = (Color*)SV_ALLOCATE_MEMORY(image_width * image_height * 4u * 6u, "Renderer");

		foreach(i, 6u) {

			images[i] = mem + image_width * image_height * i;

			u32 xoff;
			u32 yoff;

			switch (i)
			{
			case 0:
				xoff = image_width;
				yoff = image_height;
				break;
			case 1:
				xoff = image_width * 3u;
				yoff = image_height;
				break;
			case 2:
				xoff = image_width;
				yoff = 0u;
				break;
			case 3:
				xoff = image_width;
				yoff = image_height * 2u;
				break;
			case 4:
				xoff = image_width * 2u;
				yoff = image_height;
				break;
			default:
				xoff = 0u;
				yoff = image_height;
				break;
			}

			for (u32 y = yoff; y < yoff + image_height; ++y) {

				Color* src = data + xoff + y * w;

				Color* dst = images[i] + (y - yoff) * image_width;
				Color* dst_end = dst + image_width;

				while (dst != dst_end) {

					*dst = *src;

					++src;
					++dst;
				}
			}
		}

		GPUImageDesc desc;
		desc.data = images;
		desc.size = image_width * image_height * 4u;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource | GPUImageType_CubeMap;
		desc.usage = ResourceUsage_Static;
		desc.cpu_access = CPUAccess_None;
		desc.width = image_width;
		desc.height = image_height;

		bool res = graphics_image_create(&desc, pimage);

		SV_FREE_MEMORY(mem);
		SV_FREE_MEMORY(data);

		return res;
    }

    struct SpriteInstance {
		XMMATRIX  tm;
		v4_f32	  texcoord;
		GPUImage* image;
		Color	  color;
		Color	  emissive_color;
		u32       layer;
    };

    struct MeshInstance {

		XMMATRIX world_matrix;
		Mesh* mesh;
		Material* material;

    };

	struct TerrainInstance {

		XMMATRIX world_matrix;
		TerrainComponent* terrain;
		Material* material;
		
	};

    struct LightInstance {

		Entity entity;
		LightComponent* comp;

		union {

			struct {
				v3_f32 position;
			} point;

			struct {
				f32 cascade_distance[3u];
				v3_f32 view_direction;
				v4_f32 world_rotation;
				f32 cascade_far[3u];
				f32 shadow_bias;
				XMMATRIX light_matrix[4u];
			} direction;
		};

		LightInstance() { point = {};}
    };

	struct ParticlesInstance {

		v3_f32 position;
		ParticleSystem* particles;
		ParticleSystemModel* model;
		u32 layer;
		//Material* material;
		
	};

    SV_INTERNAL void screenspace_ambient_occlusion(u32 samples, f32 radius, f32 bias, CommandList cmd)
	{
		auto& gfx = renderer->gfx;
		
		graphics_event_begin("SSAO", cmd);

		GPU_SSAOData data;
		data.samples = samples;
		data.radius = radius;
		data.bias = bias;

		graphics_buffer_update(gfx.cbuffer_ssao, GPUBufferState_Constant, &data, sizeof(GPU_SSAOData), 0u, cmd);

		GPUBarrier barriers[1];
		barriers[0] = GPUBarrier::Image(gfx.gbuffer_ssao, GPUImageLayout_ShaderResource, GPUImageLayout_UnorderedAccessView);

		graphics_barrier(barriers, 1, cmd);

		graphics_shader_bind(gfx.cs_ssao, cmd);

		graphics_constant_buffer_bind(gfx.cbuffer_ssao, 0u, ShaderType_Compute, cmd);
		graphics_shader_resource_bind(gfx.gbuffer_normal, 0u, ShaderType_Compute, cmd);
		graphics_shader_resource_bind(gfx.gbuffer_depthstencil, 1u, ShaderType_Compute, cmd);
		graphics_unordered_access_view_bind(gfx.gbuffer_ssao, 0u, ShaderType_Compute, cmd);

		graphics_dispatch_image(gfx.gbuffer_ssao, 16, 16, cmd);
		
		postprocess_blur(
				BlurType_GaussianFloat,
				gfx.gbuffer_ssao,
				GPUImageLayout_UnorderedAccessView,
				GPUImageLayout_ShaderResource,
				gfx.image_ssao_aux,
				GPUImageLayout_ShaderResource,
				GPUImageLayout_ShaderResource,
				0.025f,
				os_window_aspect(),
				cmd
			);

		barriers[0] = GPUBarrier::Image(gfx.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_UnorderedAccessView);
		graphics_barrier(barriers, 1, cmd);

		graphics_shader_bind(gfx.cs_ssao_addition, cmd);

		graphics_shader_resource_bind(gfx.gbuffer_ssao, 0u, ShaderType_Compute, cmd);
		graphics_unordered_access_view_bind(gfx.offscreen, 0u, ShaderType_Compute, cmd);

		graphics_dispatch_image(gfx.offscreen, 16, 16, cmd);

		barriers[0] = GPUBarrier::Image(gfx.offscreen, GPUImageLayout_UnorderedAccessView, GPUImageLayout_RenderTarget);
		graphics_barrier(barriers, 1, cmd);

		graphics_event_end(cmd);
	}

    // TEMP
    static List<SpriteInstance> sprite_instances;
    static List<MeshInstance> mesh_instances;
	static List<TerrainInstance> terrain_instances;
    static List<LightInstance> light_instances;
	static List<ParticlesInstance> particles_instances;
    
    SV_INTERNAL void draw_sprites(GPU_CameraData& camera_data, u32 offset, u32 count, CommandList cmd)
    {
		auto& gfx = renderer->gfx;

		graphics_viewport_set(gfx.offscreen, 0u, cmd);
		graphics_scissor_set(gfx.offscreen, 0u, cmd);

		// TODO: Sort

		if (sprite_instances.size()) {

			GPUBuffer* batch_buffer = get_batch_buffer(sizeof(GPU_SpriteData), cmd);
			GPU_SpriteData& data = *(GPU_SpriteData*)renderer->batch_data[cmd];

			// Prepare
			graphics_event_begin("Sprite_GeometryPass", cmd);

			graphics_topology_set(GraphicsTopology_Triangles, cmd);
			graphics_vertex_buffer_bind(batch_buffer, 0u, 0u, cmd);
			graphics_index_buffer_bind(gfx.ibuffer_sprite, 0u, cmd);
			graphics_inputlayoutstate_bind(gfx.ils_sprite, cmd);
			graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);
			graphics_shader_bind(gfx.vs_sprite, cmd);
			graphics_shader_bind(gfx.ps_sprite, cmd);
			graphics_blendstate_bind(gfx.bs_transparent, cmd);
			graphics_depthstencilstate_bind(gfx.dss_read_depth, cmd);

			GPUImage* att[4];
			att[0] = gfx.offscreen;
			att[1] = gfx.gbuffer_normal;
			att[2] = gfx.gbuffer_emission;
			att[3] = gfx.gbuffer_depthstencil;

			XMMATRIX matrix;
			XMVECTOR pos0, pos1, pos2, pos3;

			// Batch data, used to update the vertex buffer
			SpriteVertex* batchIt = data.data;

			// Used to draw
			u32 instanceCount;

			const SpriteInstance* it = sprite_instances.data() + size_t(offset);
			const SpriteInstance* end = it + size_t(count);

			while (it != end) {

				// Compute the end ptr of the vertex data
				SpriteVertex* batchEnd;
				{
					size_t batchCount = batchIt - data.data + SPRITE_BATCH_COUNT * 4u;
					instanceCount = u32(SV_MIN(batchCount / 4U, size_t(end - it)));
					batchEnd = batchIt + instanceCount * 4u;
				}

				// Fill batch buffer
				while (batchIt != batchEnd) {

					const SpriteInstance& spr = *it++;

					matrix = XMMatrixMultiply(spr.tm, camera_data.vpm);

					pos0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
					pos1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
					pos2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
					pos3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

					pos0 = XMVector4Transform(pos0, matrix);
					pos1 = XMVector4Transform(pos1, matrix);
					pos2 = XMVector4Transform(pos2, matrix);
					pos3 = XMVector4Transform(pos3, matrix);

					*batchIt = { v4_f32(pos0), {spr.texcoord.x, spr.texcoord.y}, spr.color, spr.emissive_color };
					++batchIt;

					*batchIt = { v4_f32(pos1), {spr.texcoord.z, spr.texcoord.y}, spr.color, spr.emissive_color };
					++batchIt;

					*batchIt = { v4_f32(pos2), {spr.texcoord.x, spr.texcoord.w}, spr.color, spr.emissive_color };
					++batchIt;

					*batchIt = { v4_f32(pos3), {spr.texcoord.z, spr.texcoord.w}, spr.color, spr.emissive_color };
					++batchIt;
				}

				// Draw

				graphics_buffer_update(batch_buffer, GPUBufferState_Vertex, data.data, instanceCount * 4u * sizeof(SpriteVertex), 0u, cmd);

				graphics_renderpass_begin(gfx.renderpass_gbuffer, att, nullptr, 1.f, 0u, cmd);

				end = it;
				it -= instanceCount;

				const SpriteInstance* begin = it;
				const SpriteInstance* last = it;

				graphics_shader_resource_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);

				while (it != end) {

					if (it->image != last->image) {

						u32 spriteCount = u32(it - last);
						u32 startVertex = u32(last - begin) * 4u;

						graphics_draw_indexed(spriteCount * 6u, 1u, 0u, startVertex, 0u, cmd);

						if (it->image != last->image) {
							graphics_shader_resource_bind(it->image ? it->image : gfx.image_white, 0u, ShaderType_Pixel, cmd);
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

				end = sprite_instances.data() + size_t(offset) + size_t(count);
				batchIt = data.data;

				graphics_event_end(cmd);
			}
		}
    }

	SV_AUX GPUImage* const* get_shadow_map(Entity entity, LightComponent* light)
	{
		for (const ShadowMapRef& ref : renderer->shadow_maps) {
			if (ref.entity == entity)
				return ref.image;
		}

		// Create new shadow map
		
		ShadowMapRef& ref = renderer->shadow_maps.emplace_back();
		ref.entity = entity;

		GPUImageDesc desc;
		desc.width = 4000u;
		desc.height = 4000u;
		desc.format = GBUFFER_DEPTH_FORMAT;
		desc.layout = GPUImageLayout_DepthStencilReadOnly;
		desc.type = GPUImageType_DepthStencil | GPUImageType_ShaderResource;

		// TODO: Handle error

		foreach(i, 4u)
			graphics_image_create(&desc, &ref.image[i]);

		return ref.image;
	}

	SV_INTERNAL void bind_material(Material* material, CommandList cmd)
	{
		auto& gfx = renderer->gfx;
		
		GPU_MaterialData material_data;
		material_data.flags = 0u;

		if (material) {

			GPUImage* diffuse_map = material->diffuse_map.get();
			GPUImage* normal_map = material->normal_map.get();
			GPUImage* specular_map = material->specular_map.get();
			GPUImage* emissive_map = material->emissive_map.get();

			graphics_shader_resource_bind(diffuse_map ? diffuse_map : gfx.image_white, 0u, ShaderType_Pixel, cmd);
			if (normal_map) { graphics_shader_resource_bind(normal_map, 1u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_NORMAL_MAPPING; }
			else graphics_shader_resource_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);

			if (specular_map) { graphics_shader_resource_bind(specular_map, 2u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_SPECULAR_MAPPING; }
			else graphics_shader_resource_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);

			if (emissive_map) { graphics_shader_resource_bind(emissive_map, 3u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_EMISSIVE_MAPPING; }
			else graphics_shader_resource_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

			material_data.diffuse_color = color_to_vec3(material->diffuse_color);
			material_data.specular_color = color_to_vec3(material->specular_color);
			material_data.emissive_color = color_to_vec3(material->emissive_color);
			material_data.shininess = material->shininess;

			switch (material->culling) {

			case RasterizerCullMode_Back:
				graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
				break;
					
			case RasterizerCullMode_Front:
				graphics_rasterizerstate_bind(gfx.rs_front_culling, cmd);
				break;

			case RasterizerCullMode_None:
				graphics_rasterizerstate_unbind(cmd);
				break;
					
			}
		}
		else {

			graphics_shader_resource_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);
			graphics_shader_resource_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);
			graphics_shader_resource_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);
			graphics_shader_resource_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

			material_data.diffuse_color = color_to_vec3(Color::Gray(160u));
			material_data.specular_color = color_to_vec3(Color::Gray(10u));
			material_data.emissive_color = color_to_vec3(Color::Black());
			material_data.shininess = 1.f;

			graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
		}

		graphics_buffer_update(gfx.cbuffer_material, GPUBufferState_Constant, &material_data, sizeof(GPU_MaterialData), 0u, cmd);
	}

	static void draw_scene(CameraComponent& camera, v3_f32 camera_position, v4_f32 camera_rotation)
	{
		auto& gfx = renderer->gfx;
	    
		mesh_instances.reset();
		terrain_instances.reset();
		light_instances.reset();
		sprite_instances.reset();
		particles_instances.reset();

		CommandList cmd = graphics_commandlist_get();

		graphics_image_clear(gfx.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Black(), 1.f, 0u, cmd);
		graphics_image_clear(gfx.gbuffer_normal, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Transparent(), 1.f, 0u, cmd);
		graphics_image_clear(gfx.gbuffer_emission, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Black(), 1.f, 0u, cmd);
		graphics_image_clear(gfx.gbuffer_ssao, GPUImageLayout_ShaderResource, GPUImageLayout_ShaderResource, Color::Red(), 1.f, 0u, cmd);
		graphics_image_clear(gfx.gbuffer_depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color::Black(), 1.f, 0u, cmd);

		SceneData* scene = get_scene_data();

		// Create environment buffer
		{
			GPU_EnvironmentData data;
			data.ambient_light = color_to_vec3(scene->ambient_light);
			graphics_buffer_update(gfx.cbuffer_environment, GPUBufferState_Constant, &data, sizeof(GPU_EnvironmentData), 0u, cmd);
		}

		// Draw cameras
		{
			GPU_CameraData camera_data;

			camera_data.pm = camera.projection_matrix;
			camera_data.position = camera_position;
			camera_data.rotation = camera_rotation;
			camera_data.vm = camera.view_matrix;
			camera_data.vpm = camera.view_projection_matrix;
			camera_data.ivm = camera.inverse_view_matrix;
			camera_data.ipm = camera.inverse_projection_matrix;
			camera_data.ivpm = camera.inverse_view_projection_matrix;
			// TODO
			camera_data.screen_size.x = 1920.f;
			camera_data.screen_size.y = 1080.f;
			camera_data.near = camera.near;
			camera_data.far = camera.far;
			camera_data.width = camera.width;
			camera_data.height = camera.height;
		    
			graphics_buffer_update(gfx.cbuffer_camera, GPUBufferState_Constant, &camera_data, sizeof(GPU_CameraData), 0u, cmd);

			// BIND GLOBALS

			foreach(shader, 2) {
				graphics_constant_buffer_bind(gfx.cbuffer_camera, SV_SLOT_CAMERA, ShaderType(shader), cmd);
			}
			graphics_constant_buffer_bind(gfx.cbuffer_camera, SV_SLOT_CAMERA, ShaderType_Compute, cmd);

			if (scene->skybox.image.get() && camera.projection_type == ProjectionType_Perspective)
				draw_sky(scene->skybox.image.get(), camera_data.vm, camera_data.pm, cmd);

			// GET SPRITES
			{
				CompID sprite_id = get_component_id("Sprite");
				CompID textured_sprite_id = get_component_id("Textured Sprite");
				CompID animated_sprite_id = get_component_id("Animated Sprite");
		
				foreach_component(sprite_id, it, 0) {
			
					SpriteComponent& spr = *(SpriteComponent*)it.comp;
					Entity entity = it.entity;
			
					SpriteSheet* sprite_sheet = spr.sprite_sheet.get();
			
					v4_f32 tc;
					GPUImage* image = NULL;
			
					if (sprite_sheet) {
				
						tc = sprite_sheet->get_sprite_texcoord(spr.sprite_id);
						image = sprite_sheet->texture.get();
					}
			
					if (spr.flags & SpriteComponentFlag_XFlip) std::swap(tc.x, tc.z);
					if (spr.flags & SpriteComponentFlag_YFlip) std::swap(tc.y, tc.w);
		    
					SpriteInstance& inst = sprite_instances.emplace_back();
					inst.tm = get_entity_world_matrix(entity);
					inst.texcoord = tc;
					inst.image = image;
					inst.color = spr.color;
					inst.emissive_color = spr.emissive_color;
					inst.layer = spr.layer;
				}
				foreach_component(textured_sprite_id, it, 0) {
			
					TexturedSpriteComponent& spr = *(TexturedSpriteComponent*)it.comp;
					Entity entity = it.entity;
			
					GPUImage* image = spr.texture.get();
					v4_f32 tc = spr.texcoord;
			
					if (spr.flags & SpriteComponentFlag_XFlip) std::swap(tc.x, tc.z);
					if (spr.flags & SpriteComponentFlag_YFlip) std::swap(tc.y, tc.w);
		    
					SpriteInstance& inst = sprite_instances.emplace_back();
					inst.tm = get_entity_world_matrix(entity);
					inst.texcoord = tc;
					inst.image = image;
					inst.color = spr.color;
					inst.emissive_color = Color::Black();
					inst.layer = spr.layer;
				}
				foreach_component(animated_sprite_id, it, 0) {
			
					AnimatedSpriteComponent& s = *(AnimatedSpriteComponent*)it.comp;
					Entity entity = it.entity;
			
					SpriteSheet* sprite_sheet = s.sprite_sheet.get();
					GPUImage* image = NULL;
			
					v4_f32 tc;
			
					if (sprite_sheet && sprite_sheet->sprite_animations.exists(s.animation_id)) {
				
						SpriteAnimation& anim = sprite_sheet->sprite_animations[s.animation_id];
				
						s.simulation_time += engine.deltatime * s.time_mult;
				
						while (s.simulation_time >= anim.frame_time) {
					
							s.simulation_time -= anim.frame_time;
							++s.index;
						}
				
						s.index = s.index % anim.frames;
				
						tc = sprite_sheet->get_sprite_texcoord(anim.sprites[s.index]);
						image = sprite_sheet->texture.get();
					}
			
					if (s.flags & SpriteComponentFlag_XFlip) std::swap(tc.x, tc.z);
					if (s.flags & SpriteComponentFlag_YFlip) std::swap(tc.y, tc.w);
		    
					SpriteInstance& inst = sprite_instances.emplace_back();
					inst.tm = get_entity_world_matrix(entity);
					inst.texcoord = tc;
					inst.image = image;
					inst.color = s.color;
					inst.emissive_color = Color::Black();
					inst.layer = s.layer;
				}

				std::sort(sprite_instances.data(), sprite_instances.data() + sprite_instances.size(), [](SpriteInstance& s0, SpriteInstance& s1){
					return s0.layer < s1.layer;
				});
			}

			// GET LIGHTS
			{
				XMVECTOR camera_quat = vec4_to_dx(camera_data.rotation);
				XMVECTOR quat;

				CompID light_id = get_component_id("Light");

				foreach_component(light_id, it, 0) {
					
					Entity entity = it.entity;
					LightComponent& l = *(LightComponent*)it.comp;
						
					LightInstance& inst = light_instances.emplace_back();
					inst.entity = entity;
					inst.comp = &l;
						
					switch (l.light_type)
					{
					case LightType_Point:
					{
						XMVECTOR position = vec3_to_dx(get_entity_world_position(entity), 1.f);
						position = XMVector4Transform(position, camera_data.vm);

						inst.point.position = position;
								
						break;
					}

					case LightType_Direction:
					{
						inst.direction.world_rotation = get_entity_world_rotation(entity);
						inst.direction.cascade_distance[0] = l.cascade_distance[0];
						inst.direction.cascade_distance[1] = l.cascade_distance[1];
						inst.direction.cascade_distance[2] = l.cascade_distance[2];
						inst.direction.shadow_bias = l.shadow_bias;
								
						XMVECTOR direction = XMVectorSet(0.f, 0.f, -1.f, 0.f);
						quat = XMQuaternionMultiply(vec4_to_dx(inst.direction.world_rotation), XMQuaternionInverse(camera_quat));

						direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(quat));

						inst.direction.view_direction = direction;
					}
					break;

					}
				}
			}

			// GET PARTICLES
			{
				CompID ps_model_id = get_component_id("Particle System Model");
				CompID ps_id = get_component_id("Particle System");
				
				foreach_component(ps_model_id, it, 0) {
					
					ParticleSystemModel& psm = *(ParticleSystemModel*)it.comp;
					ParticleSystem* ps = (ParticleSystem*)get_entity_component(it.entity, ps_id);

					if (ps == NULL) continue;

					ParticlesInstance& inst = particles_instances.emplace_back();
					inst.position = get_entity_world_position(it.entity);
					inst.particles = ps;
					inst.model = &psm;
					inst.layer = ps->layer;
				}
			}

			// GET TERRAINS
			{
				CompID terrain_id = get_component_id("Terrain");

				for (CompIt it = comp_it_begin(terrain_id);
					 it.has_next;
					 comp_it_next(it))
				{
					TerrainComponent& terrain = *(TerrainComponent*)it.comp;
					Entity entity = it.entity;

					if (!terrain_valid(terrain))
						continue;
						
					TerrainInstance& inst = terrain_instances.emplace_back();
					inst.world_matrix = get_entity_world_matrix(entity);
					inst.terrain = &terrain;
					inst.material = terrain.material.get();							
				}
			}

			// GET MESHES
			{
				CompID mesh_id = get_component_id("Mesh");

				for (CompIt it = comp_it_begin(mesh_id);
					 it.has_next;
					 comp_it_next(it))
				{
					MeshComponent& mesh = *(MeshComponent*)it.comp;
					Entity entity = it.entity;
						
					Mesh* m = mesh.mesh.get();
					if (m == nullptr || m->vbuffer == nullptr || m->ibuffer == nullptr) continue;
						
					//XMMATRIX tm = get_entity_world_matrix(entity);
						
					MeshInstance& inst = mesh_instances.emplace_back();
					inst.world_matrix = get_entity_world_matrix(entity);
					inst.mesh = m;
					inst.material = mesh.material.get();
				}
			}

			graphics_state_unbind(cmd);

			// SHADOW MAPPING
			if (light_instances.size()) {

				LightInstance* it = light_instances.data();
				LightInstance* end = it + light_instances.size();

				while (it != end) {
					if (it->comp->shadow_mapping_enabled)
						break;
					++it;
				}

				while (it != end) {

					graphics_event_begin("Shadow Mapping", cmd);
						
					LightInstance& light = light_instances.back();

					if (light.comp->light_type == LightType_Direction) {

						auto& l = light.direction;

						f32 width = camera_data.width * 0.5f;
						f32 height = camera_data.height * 0.5f;

						f32 near = camera_data.near;
						f32 far = 0.f;

						// TODO: WTF
						f32 tan_xfov = tanf(atan2f(width, near));
						f32 tan_yfov = tanf(atan2f(height, near));

						GPUImage* const* shadow_maps = get_shadow_map(light.entity, light.comp);

						XMMATRIX light_view = mat_view_from_quaternion(camera_data.position, l.world_rotation);

						foreach(cascade_index, 4u) {

							if (far >= camera_data.far)
								break;
								
							GPUImage* shadow_map = shadow_maps[cascade_index];

							// Compute frustum
							near = SV_MAX(far, camera_data.near);

							if (cascade_index == 3u) {

								far = camera_data.far;
							}
							else far += l.cascade_distance[cascade_index];
									
							f32 x0 = tan_xfov * near;
							f32 x1 = tan_xfov * far;
							f32 y0 = tan_yfov * near;
							f32 y1 = tan_yfov * far;

							v3_f32 p[8u];
							p[0] = { -x0,  y0, near };
							p[1] = {  x0,  y0, near };
							p[2] = { -x0, -y0, near };
							p[3] = {  x0, -y0, near };
									
							p[4] = { -x1,  y1, far };
							p[5] = {  x1,  y1, far };
							p[6] = { -x1, -y1, far };
							p[7] = {  x1, -y1, far };

							// View space -> world space -> light view space

							XMMATRIX matrix = camera_data.ivm * light_view;

							foreach(i, 8)
								p[i] = XMVector4Transform(vec3_to_dx(p[i], 1.f), matrix);

							f32 min_x = f32_max;
							f32 max_x = -f32_max;
							f32 min_y = f32_max;
							f32 max_y = -f32_max;
							f32 min_z = f32_max;
							f32 max_z = -f32_max;

							foreach(i, 8) {
								min_x = SV_MIN(min_x, p[i].x);
								max_x = SV_MAX(max_x, p[i].x);
								min_y = SV_MIN(min_y, p[i].y);
								max_y = SV_MAX(max_y, p[i].y);
								min_z = SV_MIN(min_z, p[i].z);
								max_z = SV_MAX(max_z, p[i].z);
							}

							f32 z_center = min_z + (max_z - min_z) * 0.5f;
							min_z = SV_MIN(min_z, z_center - 1000.f);
							max_z = SV_MAX(max_z, z_center + 1000.f);
								
							XMMATRIX projection = XMMatrixOrthographicOffCenterLH(min_x, max_x, min_y, max_y, min_z, max_z);

							XMMATRIX vpm = light_view * projection;

							if (cascade_index != 3u)
								l.cascade_far[cascade_index] = far;
								
							l.light_matrix[cascade_index] = camera_data.ivm * vpm * XMMatrixScaling(0.5f, 0.5f, 1.f) * XMMatrixTranslation(0.5f, 0.5f, 0.f);

							graphics_constant_buffer_bind(gfx.cbuffer_shadow_mapping, 0u, ShaderType_Vertex, cmd);
							graphics_shader_unbind(ShaderType_Pixel, cmd);
							graphics_shader_bind(gfx.vs_shadow, cmd);
							graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
							graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
							graphics_rasterizerstate_unbind(cmd);
							graphics_blendstate_unbind(cmd);

							graphics_viewport_set(shadow_map, 0u, cmd);
							graphics_scissor_set(shadow_map, 0u, cmd);
						
							GPUImage* att[1u];
							att[0u] = shadow_map;
						
							// TODO: Use renderpass
							graphics_image_clear(shadow_map, GPUImageLayout_DepthStencilReadOnly, GPUImageLayout_DepthStencil, Color::Black(), 1.f, 0u, cmd);

							graphics_renderpass_begin(gfx.renderpass_shadow_mapping, att, cmd);

							for (const MeshInstance& mesh : mesh_instances) {

								GPU_ShadowMappingData data;
								data.tm = mesh.world_matrix * vpm;

								graphics_buffer_update(gfx.cbuffer_shadow_mapping, GPUBufferState_Constant, &data, sizeof(GPU_ShadowMappingData), 0u, cmd);
								graphics_vertex_buffer_bind(mesh.mesh->vbuffer, 0u, 0u, cmd);
								graphics_index_buffer_bind(mesh.mesh->ibuffer, 0u, cmd);

								graphics_draw_indexed(u32(mesh.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);
							}
						
							graphics_renderpass_end(cmd);

							GPUBarrier barrier = GPUBarrier::Image(shadow_map, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencilReadOnly);
							graphics_barrier(&barrier, 1u, cmd);
						}
					}

					graphics_event_end(cmd);

					++it;
				}
			}

			// DRAW SCENE
			{		    
				graphics_event_begin("Scene Rendering", cmd);

				graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
				graphics_blendstate_bind(gfx.bs_mesh, cmd);
				graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);
							
				graphics_viewport_set(gfx.offscreen, 0u, cmd);
				graphics_scissor_set(gfx.offscreen, 0u, cmd);

				// Begin renderpass
				GPUImage* att[] = { gfx.offscreen, gfx.gbuffer_normal, gfx.gbuffer_emission, gfx.gbuffer_depthstencil };
				graphics_renderpass_begin(gfx.renderpass_gbuffer, att, cmd);

				for (const LightInstance& light : light_instances) {

					// Send light data
					{			    
						GPU_LightData light_data = {};
						GPU_ShadowData shadow_data;

						graphics_shader_resource_bind(gfx.image_white, 4u, ShaderType_Pixel, cmd);
						graphics_shader_resource_bind(gfx.image_white, 5u, ShaderType_Pixel, cmd);
						graphics_shader_resource_bind(gfx.image_white, 6u, ShaderType_Pixel, cmd);
						graphics_shader_resource_bind(gfx.image_white, 7u, ShaderType_Pixel, cmd);

						GPU_LightData& l0 = light_data;
						const LightInstance& l1 = light;

						l0.type = l1.comp->light_type;
						l0.color = color_to_vec3(l1.comp->color);
						l0.intensity = l1.comp->intensity;
						l0.has_shadows = 0u;

						switch (l1.comp->light_type)
						{
						case LightType_Point:
							l0.position = l1.point.position;
							l0.range = l1.comp->range;
							l0.smoothness = l1.comp->smoothness;
							break;

						case LightType_Direction:
							l0.position = l1.direction.view_direction;

							if (l1.comp->shadow_mapping_enabled) {
									
								GPUImage* const* shadow_maps = get_shadow_map(l1.entity, l1.comp);

								foreach(i, 4u)
									graphics_shader_resource_bind(shadow_maps[i], 4u + i, ShaderType_Pixel, cmd);
									
								shadow_data.light_matrix0 = l1.direction.light_matrix[0];
								shadow_data.light_matrix1 = l1.direction.light_matrix[1];
								shadow_data.light_matrix2 = l1.direction.light_matrix[2];
								shadow_data.light_matrix3 = l1.direction.light_matrix[3];
									
								shadow_data.cascade_far0 = l1.direction.cascade_far[0];
								shadow_data.cascade_far1 = l1.direction.cascade_far[1];
								shadow_data.cascade_far2 = l1.direction.cascade_far[2];

								f32 resolution = (f32)graphics_image_info(shadow_maps[0]).width;
									
								shadow_data.bias = l1.direction.shadow_bias / resolution;
									
								l0.has_shadows = 1u;
							}
							break;
						}

						graphics_buffer_update(gfx.cbuffer_light_instances, GPUBufferState_Constant, &light_data, sizeof(GPU_LightData), 0u, cmd);
						graphics_buffer_update(gfx.cbuffer_shadow_data, GPUBufferState_Constant, &shadow_data, sizeof(GPU_ShadowData), 0u, cmd);
					}

					graphics_constant_buffer_bind(gfx.cbuffer_light_instances, 1u, ShaderType_Pixel, cmd);
					graphics_constant_buffer_bind(gfx.cbuffer_shadow_data, 2u, ShaderType_Pixel, cmd);

					if (mesh_instances.size()) {

						graphics_event_begin("Mesh Rendering", cmd);
								
						// Prepare state
						graphics_shader_bind(gfx.vs_mesh_default, cmd);
						graphics_shader_bind(gfx.ps_mesh_default, cmd);
						graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
							
						// Bind resources
						GPUBuffer* instance_buffer = gfx.cbuffer_mesh_instance;
						GPUBuffer* material_buffer = gfx.cbuffer_material;
						
						graphics_constant_buffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
						
						graphics_constant_buffer_bind(material_buffer, 0u, ShaderType_Pixel, cmd);
						graphics_constant_buffer_bind(gfx.cbuffer_environment, 3u, ShaderType_Pixel, cmd);

						foreach(i, mesh_instances.size()) {

							const MeshInstance& inst = mesh_instances[i];
								
							graphics_vertex_buffer_bind(inst.mesh->vbuffer, 0u, 0u, cmd);
							graphics_index_buffer_bind(inst.mesh->ibuffer, 0u, cmd);

							bind_material(inst.material, cmd);

							// Update instance data
							{
								GPU_MeshInstanceData data;
								data.model_view_matrix = inst.world_matrix * camera_data.vm;
								data.inv_model_view_matrix = XMMatrixInverse(nullptr, data.model_view_matrix);
								graphics_buffer_update(instance_buffer, GPUBufferState_Constant, &data, sizeof(GPU_MeshInstanceData), 0u, cmd);
							}

							graphics_draw_indexed(u32(inst.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);
						}

						graphics_event_end(cmd);
					}

					if (terrain_instances.size()) {

						graphics_event_begin("Terrain Rendering", cmd);
			
						// Prepare state
						graphics_shader_bind(gfx.vs_terrain, cmd);
						graphics_shader_bind(gfx.ps_terrain, cmd);
						graphics_inputlayoutstate_bind(gfx.ils_terrain, cmd);

						graphics_constant_buffer_bind(gfx.cbuffer_terrain_instance, 1u, ShaderType_Vertex, cmd);
						graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);

						for (const TerrainInstance& inst : terrain_instances) {

							graphics_vertex_buffer_bind(inst.terrain->vbuffer, 0u, 0u, cmd);
							graphics_index_buffer_bind(inst.terrain->ibuffer, 0u, cmd);

							bind_material(inst.material, cmd);

							// Update instance data
							{
								GPU_TerrainInstanceData data;
								data.model_view_matrix = inst.world_matrix * camera_data.vm;
								data.inv_model_view_matrix = XMMatrixInverse(nullptr, data.model_view_matrix);
								data.size_x = inst.terrain->resolution.x;
								data.size_z = inst.terrain->resolution.y;
								graphics_buffer_update(gfx.cbuffer_terrain_instance, GPUBufferState_Constant, &data, sizeof(GPU_TerrainInstanceData), 0u, cmd);
							}

							graphics_draw_indexed(u32(inst.terrain->indices.size()), 1u, 0u, 0u, 0u, cmd);
						}

						graphics_event_end(cmd);
					
					}

				}

				graphics_renderpass_end(cmd);

				graphics_event_end(cmd);

				// TODO: Optimize

				u32 sprite_offset = 0u;
				
				foreach(i, RENDER_LAYER_COUNT) {

					u32 sprite_end = (u32)sprite_instances.size();
					for (u32 j = sprite_offset; j < sprite_instances.size(); ++j) {
						if (sprite_instances[j].layer != i) {
							sprite_end = j;
							break;
						}
					}

					if (sprite_end != sprite_offset) {
						draw_sprites(camera_data, sprite_offset, sprite_end - sprite_offset, cmd);
						sprite_offset = sprite_end;
					}

					for (const ParticlesInstance& p : particles_instances) {
						if (p.layer == i)
							draw_particles(*p.particles, *p.model, p.position, camera_data.ivm, camera_data.vm, camera_data.pm, cmd);
					}
				}
			}

			// Postprocessing

			GPUBarrier barriers[3];
			barriers[0] = GPUBarrier::Image(gfx.gbuffer_normal, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
			barriers[1] = GPUBarrier::Image(gfx.gbuffer_depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencilReadOnly);

			graphics_barrier(barriers, 2u, cmd);

			if (camera.ssao.active) {
				screenspace_ambient_occlusion(camera.ssao.samples, camera.ssao.radius, camera.ssao.bias, cmd);
			}
		
			if (camera.bloom.active) {
		    
				postprocess_bloom(
						gfx.offscreen,
						GPUImageLayout_RenderTarget,
						GPUImageLayout_RenderTarget,
						gfx.image_aux0,
						GPUImageLayout_ShaderResource,
						GPUImageLayout_ShaderResource,
						gfx.image_aux1,
						GPUImageLayout_ShaderResource,
						GPUImageLayout_ShaderResource,
						gfx.gbuffer_emission,
						GPUImageLayout_RenderTarget,
						GPUImageLayout_RenderTarget,
						camera.bloom.threshold, camera.bloom.intensity, camera.bloom.strength, camera.bloom.iterations, os_window_aspect(), cmd);

			}

			barriers[0] = GPUBarrier::Image(gfx.gbuffer_normal, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
			barriers[1] = GPUBarrier::Image(gfx.gbuffer_depthstencil, GPUImageLayout_DepthStencilReadOnly, GPUImageLayout_DepthStencil);

			graphics_barrier(barriers, 2u, cmd);			
		}
	}

    void _draw_scene(CameraComponent& camera, v3_f32 camera_position, v4_f32 camera_rotation)
    {
		if (!there_is_scene()) return;

		event_dispatch("pre_draw_scene", NULL);
		draw_scene(camera, camera_position, camera_rotation);
		event_dispatch("post_draw_scene", NULL);
    }

	void _draw_scene()
	{
		event_dispatch("pre_draw_scene", NULL);
		
		CameraComponent* camera = get_main_camera();

		if (camera) {
			Entity entity = camera->id;
			
			v3_f32 position = get_entity_world_position(entity);
			v4_f32 rotation = get_entity_world_rotation(entity);
			draw_scene(*camera, position, rotation);
		}
		
		event_dispatch("post_draw_scene", NULL);
	}

    void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd)
    {
		auto& gfx = renderer->gfx;

		graphics_viewport_set(gfx.offscreen, 0u, cmd);
		graphics_scissor_set(gfx.offscreen, 0u, cmd);
	
		graphics_depthstencilstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_inputlayoutstate_bind(gfx.ils_sky, cmd);
		graphics_rasterizerstate_unbind(cmd);
		graphics_topology_set(GraphicsTopology_Triangles, cmd);

		graphics_vertex_buffer_bind(gfx.vbuffer_skybox, 0u, 0u, cmd);
		graphics_constant_buffer_bind(gfx.cbuffer_skybox, 0u, ShaderType_Vertex, cmd);
		graphics_shader_resource_bind(skymap, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		XMFLOAT4X4 vm;
		XMStoreFloat4x4(&vm, view_matrix);
		vm._41 = 0.f;
		vm._42 = 0.f;
		vm._43 = 0.f;
		vm._44 = 1.f;

		view_matrix = XMLoadFloat4x4(&vm);
		view_matrix = view_matrix * projection_matrix;

		graphics_buffer_update(gfx.cbuffer_skybox, GPUBufferState_Constant, &view_matrix, sizeof(XMMATRIX), 0u, cmd);

		graphics_shader_bind(gfx.vs_sky, cmd);
		graphics_shader_bind(gfx.ps_sky, cmd);

		GPUImage* att[] = { gfx.offscreen };
		graphics_renderpass_begin(gfx.renderpass_off, att, 0, 1.f, 0u, cmd);
		graphics_draw(36u, 1u, 0u, 0u, cmd);
		graphics_renderpass_end(cmd);
    }

    void draw_text(const DrawTextDesc& desc, CommandList cmd)
    {
		if (desc.text == nullptr) return;
		
		size_t text_size = (desc.text_size == size_t_max) ? string_size(desc.text) : desc.text_size;
		
		if (text_size == 0u) return;

		auto& gfx = renderer->gfx;
	
		// Select font
		Font& font = desc.font ? *desc.font : renderer->font_opensans;

		f32 aspect = (desc.aspect == f32_max) ? os_window_aspect() : desc.aspect;

		// Text space transformation
		f32 xmult = desc.font_size / aspect;
		f32 ymult = desc.font_size;

		List<TextVertex>& vertices = renderer->text_vertices[cmd];
		vertices.reset();
		
		f32 line_height = ymult;

		const char* it = desc.text;
		const char* end = it + text_size;

		f32 xoff;
		f32 yoff = 0.f;

		u32 max_lines = desc.max_lines;

		while (it != end && max_lines--) {

			xoff = 0.f;
			yoff -= line_height;
			u32 line_begin_index = (u32)vertices.size();

			// TODO: add first char offset to xoff
			const char* line_end = process_text_line(it, u32(text_size - (it - desc.text)), desc.max_width / xmult, font);
			SV_ASSERT(line_end != it);

			// Fill batch
			while (it != line_end) {

				Glyph* g_ = font.get(*it);
				
				if (g_) {

					Glyph& g = *g_;

					f32 advance = g.advance * xmult;

					if (*it != ' ') {
						f32 xpos = xoff + g.xoff * xmult;
						f32 ypos = yoff + g.yoff * ymult;
						f32 width = g.w * xmult;
						f32 height = g.h * ymult;
			
						vertices.push_back({ v4_f32{ xpos	     , ypos + height, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.w }, desc.color });
						vertices.push_back({ v4_f32{ xpos + width, ypos + height, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.w }, desc.color });
						vertices.push_back({ v4_f32{ xpos		 , ypos		    , 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.y }, desc.color });
						vertices.push_back({ v4_f32{ xpos + width, ypos		    , 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.y }, desc.color });
					}

					xoff += advance;
				}

				++it;
			}

			// Alignment
			if (vertices.size() != line_begin_index) {

				XMMATRIX matrix = XMMatrixIdentity();

				f32 line_begin = (vertices[line_begin_index].position.x);
				f32 line_end = (vertices.back().position.x);

				switch (desc.alignment)
				{
					
				case TextAlignment_Center:
					matrix = XMMatrixTranslation((line_begin - line_end) * 0.5f, 0.f, 0.f);
					break;

				case TextAlignment_Right:
					break;
				case TextAlignment_Justified:
					SV_LOG_ERROR("TODO-> Justified text rendering");
					break;
				}

				XMVECTOR vec;
				for (u32 i = line_begin_index; i < vertices.size(); ++i) {

					vec = vec4_to_dx(vertices[i].position);
					vec = XMVector4Transform(vec, matrix);
					vertices[i].position = vec;
				}
			}
		}

		// Transformation
		if (vertices.size() >= 4u) {
			
			XMMATRIX matrix = desc.transform_matrix;

			f32 y = desc.centred ? (-yoff * 0.5f) : 0.f;

			switch (desc.alignment) {

			case TextAlignment_Left:
				matrix = XMMatrixTranslation(-desc.max_width * 0.5f, y, 0.f) * matrix;
				break;

			default:
				matrix = XMMatrixTranslation(0.f, y, 0.f) * matrix;
				
			};

			XMVECTOR vec;
			foreach(i, vertices.size()) {
				
				vec = vec4_to_dx(vertices[i].position);
				vec = XMVector4Transform(vec, matrix);
				vertices[i].position = vec;
			}
		}

		// Prepare
		graphics_rasterizerstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_topology_set(GraphicsTopology_Triangles, cmd);

		graphics_shader_bind(gfx.vs_text, cmd);
		graphics_shader_bind(gfx.ps_text, cmd);

		GPUBuffer* buffer = get_batch_buffer(sizeof(GPU_TextData), cmd);
		graphics_vertex_buffer_bind(buffer, 0u, 0u, cmd);
		graphics_index_buffer_bind(gfx.ibuffer_text, 0u, cmd);
		graphics_shader_resource_bind(font.image, 0u, ShaderType_Pixel, cmd);
		graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

		graphics_inputlayoutstate_bind(gfx.ils_text, cmd);

		if (vertices.size()) {

			u32 vertex_offset = 0u;

			while (vertex_offset < vertices.size()) {

				u32 vertex_count = SV_MIN((u32)vertices.size() - vertex_offset, TEXT_BATCH_COUNT * 4u);
				graphics_buffer_update(buffer, GPUBufferState_Vertex, vertices.data() + vertex_offset, vertex_count * sizeof(TextVertex), 0u, cmd);
				
				GPUImage* att[1];
				att[0] = gfx.offscreen;
				
				graphics_renderpass_begin(gfx.renderpass_off, att, nullptr, 1.f, 0u, cmd);
				graphics_draw_indexed(vertex_count / 4u * 6u, 1u, 0u, 0u, 0u, cmd);
				graphics_renderpass_end(cmd);

				vertex_offset += vertex_count;
			}
		}
    }

    void draw_text_area(const DrawTextAreaDesc& desc, CommandList cmd)
    {
		if (desc.text == nullptr) return;
		
		size_t text_size = (desc.text_size == size_t_max) ? string_size(desc.text) : desc.text_size;
		
		if (text_size == 0u) return;
	
		// Select font
		Font& font = desc.font ? *desc.font : renderer->font_opensans;

		f32 aspect = (desc.aspect == f32_max) ? os_window_aspect() : desc.aspect;

		f32 transformed_max_width = desc.max_width / (desc.font_size / aspect);

		const char* text = desc.text;

		// Select text to render
		if (desc.bottom_top) {
			
			// Count lines
			u32 lines = 0u;

			const char* it = text;
			const char* end = text + text_size;

			while (it != end) {

				it = process_text_line(it, u32(text_size - (it - text)), transformed_max_width, font);
				++lines;
			}

			u32 begin_line_index = lines - SV_MIN(desc.max_lines, lines);
			begin_line_index -= SV_MIN(begin_line_index, desc.line_offset);

			it = text;
			lines = 0u;

			while (it != end && lines != begin_line_index) {

				it = process_text_line(it, u32(text_size - (it - text)), transformed_max_width, font);
				++lines;
			}

			text_size = text_size - (it - text);
			text = it;
		}
		else if (desc.line_offset) {

			const char* it = text;
			const char* end = text + text_size;

			foreach (i, desc.line_offset) {

				it = process_text_line(it, u32(text_size - (it - text)), desc.max_width / desc.font_size / aspect, font);
				if (it == end) return;
			}

			text_size = text_size - (it - text);
			text = it;
		}

		DrawTextDesc desc0;
		desc0.text = text;
		desc0.text_size = text_size;
		desc0.transform_matrix = desc.transform_matrix;
		desc0.max_width = desc.max_width;
		desc0.max_lines = desc.max_lines;
		desc0.centred = false;
		desc0.font_size = desc.font_size;
		desc0.aspect = aspect;
		desc0.alignment = desc.alignment;
		desc0.font = &font;

		draw_text(desc0, cmd);
    }

    Font& renderer_default_font()
    {
		return renderer->font_opensans;
    }

    GPUImage* renderer_white_image()
    {
		return renderer->gfx.image_white;
    }

	GPUImage* renderer_offscreen()
    {
		return renderer->gfx.offscreen;
    }

    // POSTPROCESSING

    void postprocess_blur(
			BlurType blur_type,
			GPUImage* src,
			GPUImageLayout src_layout0,
			GPUImageLayout src_layout1,
			GPUImage* aux,
			GPUImageLayout aux_layout0,
			GPUImageLayout aux_layout1,
			f32 intensity,
			f32 aspect,
			CommandList cmd
		)
    {
		GPUBarrier barriers[2u];
		u32 barrier_count = 0u;
		GPU_GaussianBlurData data;

		auto& gfx = renderer->gfx;
	
		GPUImageInfo src_info = graphics_image_info(src);
		GPUImageInfo aux_info = graphics_image_info(aux);
		
		// Prepare image layouts
		{
			if (src_layout0 != GPUImageLayout_ShaderResource) {

				barriers[0u] = GPUBarrier::Image(src, src_layout0, GPUImageLayout_ShaderResource);
				++barrier_count;
			}
			if (aux_layout0 != GPUImageLayout_UnorderedAccessView) {

				barriers[barrier_count++] = GPUBarrier::Image(aux, aux_layout0, GPUImageLayout_UnorderedAccessView);
			}
			if (barrier_count) {
				graphics_barrier(barriers, barrier_count, cmd);
			}
		}

		if (blur_type == BlurType_GaussianFloat4)
			graphics_shader_bind(gfx.cs_gaussian_blur_float4, cmd);
		else if (blur_type == BlurType_GaussianFloat)
			graphics_shader_bind(gfx.cs_gaussian_blur_float, cmd);

		graphics_sampler_bind(gfx.sampler_linear_clamp, 0u, ShaderType_Compute, cmd);
		graphics_unordered_access_view_bind(aux, 0u, ShaderType_Compute, cmd);
		graphics_shader_resource_bind(src, 0u, ShaderType_Compute, cmd);
		graphics_constant_buffer_bind(gfx.cbuffer_gaussian_blur, 0u, ShaderType_Compute, cmd);
	
		// Horizontal blur
		{
			data.intensity = intensity * (f32(aux_info.width) / f32(src_info.width));
			if (aspect < 1.f) {
				data.intensity /= aspect;
			}

			data.horizontal = 1u;
			graphics_buffer_update(gfx.cbuffer_gaussian_blur, GPUBufferState_Constant, &data, sizeof(GPU_GaussianBlurData), 0u, cmd);
		
			graphics_dispatch_image(aux, 16, 16, cmd);
		}

		// Vertical blur
		{
			barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_ShaderResource, GPUImageLayout_UnorderedAccessView);
			barriers[1u] = GPUBarrier::Image(aux, GPUImageLayout_UnorderedAccessView, GPUImageLayout_ShaderResource);
			graphics_barrier(barriers, 2u, cmd);

			graphics_unordered_access_view_bind(src, 0u, ShaderType_Compute, cmd);
			graphics_shader_resource_bind(aux, 0u, ShaderType_Compute, cmd);

			data.intensity = intensity * (f32(aux_info.height) / f32(src_info.height));
			if (aspect > 1.f) {
				data.intensity *= aspect;
			}

			data.horizontal = 0u;
			graphics_buffer_update(gfx.cbuffer_gaussian_blur, GPUBufferState_Constant, &data, sizeof(GPU_GaussianBlurData), 0u, cmd);
			
			graphics_dispatch_image(src, 16, 16, cmd);
		}

		// Change images layout
		{
			barrier_count = 0u;

			if (src_layout1 != GPUImageLayout_UnorderedAccessView) {

				barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_UnorderedAccessView, src_layout1);
				++barrier_count;
			}
			if (aux_layout1 != GPUImageLayout_ShaderResource) {

				barriers[barrier_count++] = GPUBarrier::Image(aux, GPUImageLayout_ShaderResource, aux_layout1);
			}
			if (barrier_count) {
				graphics_barrier(barriers, barrier_count, cmd);
			}
		}
    }

    void postprocess_bloom(
			GPUImage* src, 
			GPUImageLayout src_layout0, 
			GPUImageLayout src_layout1, 
			GPUImage* aux0, // Used in threshold pass
			GPUImageLayout aux0_layout0, 
			GPUImageLayout aux0_layout1,
			GPUImage* aux1, // Used to blur the image
			GPUImageLayout aux1_layout0, 
			GPUImageLayout aux1_layout1,
			GPUImage* emission,
			GPUImageLayout emission_layout0, 
			GPUImageLayout emission_layout1,
			f32 threshold,
			f32 intensity,
			f32 strength,
			u32 iterations,
			f32 aspect,
			CommandList cmd
		)
    {
		GPUBarrier barriers[3];
		u32 barrier_count = 0u;

		auto& gfx = renderer->gfx;

		if (iterations == 0)
			return;

		graphics_event_begin("Bloom", cmd);

		// Threshold pass
		{
			// Prepare image layouts
			if (src_layout0 != GPUImageLayout_ShaderResource) {
				barriers[0u] = GPUBarrier::Image(src, src_layout0, GPUImageLayout_ShaderResource);
				++barrier_count;
			}
			if (aux0_layout0 != GPUImageLayout_UnorderedAccessView) {
				barriers[barrier_count++] = GPUBarrier::Image(aux0, aux0_layout0, GPUImageLayout_UnorderedAccessView);
			}
			if (emission_layout0 != GPUImageLayout_ShaderResource) {
				barriers[barrier_count++] = GPUBarrier::Image(emission, emission_layout0, GPUImageLayout_ShaderResource);
			}
			
			if (barrier_count) {
				graphics_barrier(barriers, barrier_count, cmd);
			}

			graphics_unordered_access_view_bind(aux0, 0u, ShaderType_Compute, cmd);
			graphics_shader_resource_bind(src, 0u, ShaderType_Compute, cmd);
			graphics_constant_buffer_bind(gfx.cbuffer_bloom_threshold, 0u, ShaderType_Compute, cmd);
			graphics_shader_bind(gfx.cs_bloom_threshold, cmd);
			graphics_sampler_bind(gfx.sampler_linear_clamp, 0u, ShaderType_Compute, cmd);

			GPU_BloomThresholdData data;
			data.threshold = threshold;
			data.strength = strength;

			graphics_buffer_update(gfx.cbuffer_bloom_threshold, GPUBufferState_Constant, &data, sizeof(GPU_BloomThresholdData), 0u, cmd);

			graphics_dispatch_image(aux0, 16, 16, cmd);
		}

		// Add emissive map
		if (emission) {

			graphics_shader_bind(gfx.cs_bloom_addition, cmd);
			graphics_unordered_access_view_bind(aux0, 0u, ShaderType_Compute, cmd);
			graphics_shader_resource_bind(emission, 0u, ShaderType_Compute, cmd);
		
			graphics_dispatch_image(aux0, 16, 16, cmd);
		}

		// Blur image		
		foreach(i, iterations - 1) {
			
			postprocess_blur(
					BlurType_GaussianFloat4,
					aux0,
					GPUImageLayout_UnorderedAccessView,
					GPUImageLayout_UnorderedAccessView,
					aux1,
					aux1_layout0,
					aux1_layout0,
					intensity,
					aspect,
					cmd);
		}

		postprocess_blur(
					BlurType_GaussianFloat4,
					aux0,
					GPUImageLayout_UnorderedAccessView,
					GPUImageLayout_ShaderResource,
					aux1,
					aux1_layout0,
					aux1_layout1,
					intensity,
					aspect,
					cmd);

		// Color addition
		{
			barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_ShaderResource, GPUImageLayout_UnorderedAccessView);
			graphics_barrier(barriers, 1u, cmd);
			
			graphics_shader_bind(gfx.cs_bloom_addition, cmd);
			graphics_unordered_access_view_bind(src, 0u, ShaderType_Compute, cmd);
			graphics_shader_resource_bind(aux0, 0u, ShaderType_Compute, cmd);
		
			graphics_dispatch_image(src, 16, 16, cmd);
		}

		// Last memory barriers
		barrier_count = 0u;

		if (src_layout1 != GPUImageLayout_UnorderedAccessView) {

			barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_UnorderedAccessView, src_layout1);
			++barrier_count;
		}
		if (aux0_layout1 != GPUImageLayout_ShaderResource) {
			barriers[barrier_count++] = GPUBarrier::Image(aux0, GPUImageLayout_ShaderResource, aux0_layout1);
		}
		if (emission_layout1 != GPUImageLayout_ShaderResource) {
			barriers[barrier_count++] = GPUBarrier::Image(emission, GPUImageLayout_ShaderResource, emission_layout1);
		}
		if (barrier_count) {
			graphics_barrier(barriers, barrier_count, cmd);
		}

		graphics_event_end(cmd);
    }

	// DEBUG

#if SV_EDITOR

	void display_debug_renderer()
	{
		if (gui_begin_window("Renderer Debug")) {
			
			// Shadow mapping info
			if (gui_collapse("Shadow maps")) {

				for (ShadowMapRef ref : renderer->shadow_maps) {

					const char* name = entity_exists(ref.entity) ? get_entity_name(ref.entity) : "Not exist";
					if (name == NULL || string_size(name) == 0u)
						name = "Unnamed";

					gui_text(get_entity_name(ref.entity));
					foreach(i, 4u) {

						gui_push_image(GuiImageType_Background, ref.image[i], { 0.f, 0.f, 1.f, 1.f }, GPUImageLayout_DepthStencilReadOnly);
						gui_image(200.f, ref.entity);
						gui_pop_image();
					}
				}
			}

			if (gui_collapse("SSAO")) {

				gui_push_image(GuiImageType_Background, renderer->gfx.gbuffer_ssao, { 0.f, 0.f, 1.f, 1.f }, GPUImageLayout_ShaderResource);
				gui_image(400.f, 93842);
				gui_pop_image();
			}
			gui_end_window();
		}
	}
	
#endif
}
