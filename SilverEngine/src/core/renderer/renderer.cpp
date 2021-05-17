#include "defines.h"

#include "core/renderer/renderer_internal.h"
#include "core/mesh.h"
#include "debug/console.h"

namespace sv {

    RendererState* renderer = nullptr;

    // SHADER COMPILATION

#define COMPILE_SHADER(type, shader, path, alwais_compile) SV_CHECK(graphics_shader_compile_fastbin_from_file(path, type, &shader, "$system/shaders/" path, alwais_compile));
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)

    static bool compile_shaders()
    {
	auto& gfx = renderer->gfx;
	
	COMPILE_VS(gfx.vs_im_primitive, "immediate_shader.hlsl");
	COMPILE_PS(gfx.ps_im_primitive, "immediate_shader.hlsl");
	COMPILE_VS_(gfx.vs_im_mesh_wireframe, "immediate_mesh_wireframe.hlsl");

	COMPILE_VS(gfx.vs_text, "text.hlsl");
	COMPILE_PS(gfx.ps_text, "text.hlsl");

	COMPILE_VS(gfx.vs_sprite, "sprite/default.hlsl");
	COMPILE_PS(gfx.ps_sprite, "sprite/default.hlsl");

	COMPILE_VS_(gfx.vs_mesh_default, "mesh_default.hlsl");
	COMPILE_PS_(gfx.ps_mesh_default, "mesh_default.hlsl");

	COMPILE_VS(gfx.vs_sky, "skymapping.hlsl");
	COMPILE_PS(gfx.ps_sky, "skymapping.hlsl");

	COMPILE_VS(gfx.vs_default_postprocess, "postprocessing/default.hlsl");
	COMPILE_PS(gfx.ps_default_postprocess, "postprocessing/default.hlsl");
	COMPILE_PS(gfx.ps_gaussian_blur, "postprocessing/gaussian_blur.hlsl");
	COMPILE_PS(gfx.ps_bloom_threshold, "postprocessing/bloom_threshold.hlsl");
	COMPILE_PS(gfx.ps_ssao, "postprocessing/ssao.hlsl");

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

	// SSAO pass
	att[0].loadOp = AttachmentOperation_Load;
	att[0].storeOp = AttachmentOperation_Store;
	att[0].stencilLoadOp = AttachmentOperation_DontCare;
	att[0].stencilStoreOp = AttachmentOperation_DontCare;
	att[0].format = GBUFFER_SSAO_FORMAT;
	att[0].initialLayout = GPUImageLayout_RenderTarget;
	att[0].layout = GPUImageLayout_RenderTarget;
	att[0].finalLayout = GPUImageLayout_RenderTarget;
	att[0].type = AttachmentType_RenderTarget;

	desc.attachmentCount = 1u;
	CREATE_RENDERPASS("SSAORenderpass", gfx.renderpass_ssao);

	return true;
    }

    // BUFFER CREATION

    static bool create_buffers()
    {
	auto& gfx = renderer->gfx;
	
	GPUBufferDesc desc;

	// Environment
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(EnvironmentData);
	    desc.pData = nullptr;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_environment));
	}

	// Immediate rendering
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Dynamic;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(ImRendVertex) * 4u;
	    desc.pData = nullptr;

	    foreach(i, GraphicsLimit_CommandList)
		SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_im_primitive[i]));

	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Dynamic;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(XMMATRIX) + sizeof(v4_f32);
	    desc.pData = nullptr;

	    foreach(i, GraphicsLimit_CommandList)
		SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_im_mesh[i]));
	}

	// Gaussian blur
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(GaussianBlurData);
	    desc.pData = nullptr;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_gaussian_blur));
	}

	// Bloom threshold
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(v4_f32);
	    desc.pData = nullptr;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_bloom_threshold));
	}

	// Camera buffer
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(GPU_CameraData);
	    desc.pData = nullptr;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_camera));
	}

	// set text index data
	{
	    constexpr u32 index_count = TEXT_BATCH_COUNT * 6u;
	    u16* index_data = new u16[index_count];

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
	    desc.indexType = IndexType_16;
	    desc.bufferType = GPUBufferType_Index;
	    desc.usage = ResourceUsage_Static;
	    desc.CPUAccess = CPUAccess_None;
	    desc.size = sizeof(u16) * index_count;
	    desc.pData = index_data;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.ibuffer_text));
	    graphics_name_set(gfx.ibuffer_text, "Text_IndexBuffer");

	    delete[] index_data;
	}

	// Sprite index buffer
	{
	    u32* index_data = new u32[SPRITE_BATCH_COUNT * 6u];
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

	    desc.bufferType = GPUBufferType_Index;
	    desc.CPUAccess = CPUAccess_None;
	    desc.usage = ResourceUsage_Static;
	    desc.pData = index_data;
	    desc.size = SPRITE_BATCH_COUNT * 6u * sizeof(u32);
	    desc.indexType = IndexType_32;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.ibuffer_sprite));
	    delete[] index_data;

	    graphics_name_set(gfx.ibuffer_sprite, "Sprite_IndexBuffer");
	}

	// Mesh
	{
	    desc.pData = nullptr;
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Dynamic;
	    desc.CPUAccess = CPUAccess_Write;

	    desc.size = sizeof(GPU_MeshInstanceData);
	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance));

	    desc.size = sizeof(Material);
	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_material));

	    desc.size = sizeof(GPU_LightData) * LIGHT_COUNT;
	    SV_CHECK(graphics_buffer_create(&desc, &gfx.cbuffer_light_instances));
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


	    desc.pData = box;
	    desc.bufferType = GPUBufferType_Vertex;
	    desc.size = sizeof(v3_f32) * 36u;
	    desc.usage = ResourceUsage_Static;
	    desc.CPUAccess = CPUAccess_None;

	    SV_CHECK(graphics_buffer_create(&desc, &gfx.vbuffer_skybox));

	    desc.pData = nullptr;
	    desc.bufferType = GPUBufferType_Constant;
	    desc.size = sizeof(XMMATRIX);
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;

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
	    desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
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
	    desc.layout = GPUImageLayout_RenderTarget;
	    desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
	    SV_CHECK(graphics_image_create(&desc, &gfx.gbuffer_ssao));
	}
	
	// Aux Image
	{
	    desc.pData = nullptr;
	    desc.size = 0u;
	    desc.format = OFFSCREEN_FORMAT;
	    desc.layout = GPUImageLayout_ShaderResource;
	    desc.type = GPUImageType_RenderTarget | GPUImageType_ShaderResource;
	    desc.usage = ResourceUsage_Static;
	    desc.CPUAccess = CPUAccess_None;
	    desc.width = 1920u / 2u;
	    desc.height = 1080u / 2u;

	    SV_CHECK(graphics_image_create(&desc, &gfx.image_aux0));
	    SV_CHECK(graphics_image_create(&desc, &gfx.image_aux1));
	}

	// White Image
	{
	    u8 bytes[4];
	    for (u32 i = 0; i < 4; ++i) bytes[i] = 255u;

	    desc.pData = bytes;
	    desc.size = sizeof(u8) * 4u;
	    desc.format = Format_R8G8B8A8_UNORM;
	    desc.layout = GPUImageLayout_ShaderResource;
	    desc.type = GPUImageType_ShaderResource;
	    desc.usage = ResourceUsage_Static;
	    desc.CPUAccess = CPUAccess_None;
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

	    desc.elementCount = 3u;
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
	    att[0].dstAlphaBlendFactor = BlendFactor_One;
	    att[0].alphaBlendOp = BlendOperation_Add;
	    att[0].colorWriteMask = ColorComponent_All;

	    desc.attachmentCount = 1u;
	    desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
	    SV_CHECK(graphics_blendstate_create(&desc, &gfx.bs_transparent));

	    // MESH
	    foreach(i, GraphicsLimit_AttachmentRT) {
		att[i].blendEnabled = false;
		att[i].colorWriteMask = ColorComponent_All;
	    }

	    desc.attachmentCount = GraphicsLimit_AttachmentRT;

	    SV_CHECK(graphics_blendstate_create(&desc, &gfx.bs_mesh));

	    // ADDITION
	    att[0].blendEnabled = true;
	    att[0].srcColorBlendFactor = BlendFactor_One;
	    att[0].dstColorBlendFactor = BlendFactor_One;
	    att[0].colorBlendOp = BlendOperation_Add;
	    att[0].srcAlphaBlendFactor = BlendFactor_One;
	    att[0].dstAlphaBlendFactor = BlendFactor_One;
	    att[0].alphaBlendOp = BlendOperation_Add;
	    att[0].colorWriteMask = ColorComponent_All;

	    desc.attachmentCount = 1u;
	    desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
	    SV_CHECK(graphics_blendstate_create(&desc, &gfx.bs_addition));
	}

	// Create DepthStencilStates
	{
	    DepthStencilStateDesc desc;
	    desc.depthTestEnabled = true;
	    desc.depthWriteEnabled = true;
	    desc.depthCompareOp = CompareOperation_Less;
	    desc.stencilTestEnabled = false;

	    graphics_depthstencilstate_create(&desc, &gfx.dss_default_depth);
	}

	// Create RasterizerStates
	{
	    RasterizerStateDesc desc;
	    desc.wireframe = false;
	    desc.cullMode = RasterizerCullMode_Back;
	    desc.clockwise = true;

	    graphics_rasterizerstate_create(&desc, &gfx.rs_back_culling);

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
	SV_CHECK(graphics_sampler_create(&desc, &gfx.sampler_blur));

	return true;
    }

    // RENDERER RUNTIME

    bool _renderer_initialize()
    {
	renderer = SV_ALLOCATE_STRUCT(RendererState);
	
	SV_CHECK(compile_shaders());
	SV_CHECK(create_renderpasses());
	SV_CHECK(create_samplers());
	SV_CHECK(create_buffers());
	SV_CHECK(create_images());
	SV_CHECK(create_states());

	// Create default fonts
	SV_CHECK(font_create(renderer->font_opensans, "$system/fonts/OpenSans/OpenSans-Regular.ttf", 128.f, 0u));
	SV_CHECK(font_create(renderer->font_console, "$system/fonts/Cousine/Cousine-Regular.ttf", 128.f, 0u));

	return true;
    }

    bool _renderer_close()
    {
	auto& gfx = renderer->gfx;
	
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

	    font_destroy(renderer->font_opensans);
	    font_destroy(renderer->font_console);

	    SV_FREE_STRUCT(renderer);
	}

	return true;
    }

    void _renderer_begin()
    {
	auto& gfx = renderer->gfx;
	
	CommandList cmd = graphics_commandlist_begin();

	graphics_image_clear(gfx.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Black(), 1.f, 0u, cmd);
	graphics_image_clear(gfx.gbuffer_normal, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Transparent(), 1.f, 0u, cmd);
	graphics_image_clear(gfx.gbuffer_emission, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Black(), 1.f, 0u, cmd);
	graphics_image_clear(gfx.gbuffer_ssao, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color::Red(), 1.f, 0u, cmd);
	graphics_image_clear(gfx.gbuffer_depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color::Black(), 1.f, 0u, cmd);
	
	graphics_viewport_set(gfx.offscreen, 0u, cmd);
	graphics_scissor_set(gfx.offscreen, 0u, cmd);
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
	Color* mem = (Color*)SV_ALLOCATE_MEMORY(image_width * image_height * 4u * 6u);

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
	desc.pData = images;
	desc.size = image_width * image_height * 4u;
	desc.format = Format_R8G8B8A8_UNORM;
	desc.layout = GPUImageLayout_ShaderResource;
	desc.type = GPUImageType_ShaderResource | GPUImageType_CubeMap;
	desc.usage = ResourceUsage_Static;
	desc.CPUAccess = CPUAccess_None;
	desc.width = image_width;
	desc.height = image_height;

	bool res = graphics_image_create(&desc, pimage);

	free(mem);
	delete[] data;

	return res;
    }

    struct SpriteInstance {
	XMMATRIX  tm;
	v4_f32	  texcoord;
	GPUImage* image;
	Color	  color;
	f32	  layer;

	SpriteInstance() = default;
	SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color, f32 layer)
	    : tm(m), texcoord(texcoord), image(image), color(color), layer(layer) {}
    };

    struct MeshInstance {

	XMMATRIX transform_matrix;
	Mesh* mesh;
	Material* material;

	MeshInstance(const XMMATRIX& transform_matrix, Mesh* mesh, Material* material)
	    : transform_matrix(transform_matrix), mesh(mesh), material(material) {}

    };

    struct LightInstance {

	Color	     color;
	LightType    type;
	v3_f32	     position;
	f32	     range;
	f32	     intensity;
	f32	     smoothness;

	// None
	LightInstance() = default;

	// Point
	LightInstance(Color color, const v3_f32& position, f32 range, f32 intensity, f32 smoothness)
	    : color(color), type(LightType_Point), position(position), range(range), intensity(intensity), smoothness(smoothness) {}

	// Direction
	LightInstance(Color color, const v3_f32& direction, f32 intensity)
	    : color(color), type(LightType_Direction), position(direction), intensity(intensity) {}
    };

    SV_AUX void postprocessing_draw_call(CommandList cmd)
    {
	graphics_draw(4u, 1u, 0u, 0u, cmd);
    }

    /*SV_INTERNAL void screenspace_ambient_occlusion(CommandList cmd)
    {
	auto& gfx = renderer->gfx;
	
	graphics_event_begin("SSAO", cmd);
	
	graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
	graphics_shader_bind(gfx.vs_default_postprocess, cmd);
	graphics_shader_bind(gfx.ps_ssao, cmd);
	graphics_inputlayoutstate_unbind(cmd);
	graphics_blendstate_unbind(cmd);

	graphics_image_bind(gfx.gbuffer_normal, 0u, ShaderType_Pixel, cmd);
	graphics_image_bind(gfx.gbuffer_depthstencil, 1u, ShaderType_Pixel, cmd);

	graphics_constantbuffer_bind(gfx.cbuffer_camera, 0u, ShaderType_Pixel, cmd);
	
	GPUImage* att[1];
	att[0] = gfx.gbuffer_ssao;

        graphics_renderpass_begin(gfx.renderpass_ssao, att, cmd);
	postprocessing_draw_call(cmd);
	graphics_renderpass_end(cmd);

	postprocess_gaussian_blur(
		gfx.gbuffer_ssao,
		GPUImageLayout_RenderTarget,
		GPUImageLayout_ShaderResource,
		gfx.image_aux0,
		GPUImageLayout_ShaderResource,
		GPUImageLayout_ShaderResource,
		0.03f,
		os_window_aspect(),
		cmd,
		gfx.renderpass_ssao
	    );

	graphics_event_end(cmd);
	}*/

    // TEMP
    static List<SpriteInstance> sprite_instances;
    static List<MeshInstance> mesh_instances;
    static List<LightInstance> light_instances;
    
    SV_INTERNAL void draw_sprites(GPU_CameraData& camera_data, CommandList cmd)
    {
	auto& gfx = renderer->gfx;
	
	{
	    ComponentIterator it;
	    CompView<SpriteComponent> view;
	    
	    if (comp_it_begin(it, view)) {

		do {

		    SpriteComponent& spr = *view.comp;
		    Entity entity = view.entity;

		    v3_f32 pos = get_entity_world_position(entity);

		    v4_f32 tc = spr.texcoord;

		    if (spr.flags & SpriteComponentFlag_XFlip) std::swap(tc.x, tc.z);
		    if (spr.flags & SpriteComponentFlag_YFlip) std::swap(tc.y, tc.w);
		    
		    sprite_instances.emplace_back(get_entity_world_matrix(entity), tc, spr.texture.get(), spr.color, pos.z);
		}
		while(comp_it_next(it, view));
	    }
	}

	if (sprite_instances.size()) {

	    // Sort sprites
	    std::sort(sprite_instances.data(), sprite_instances.data() + sprite_instances.size(), [](const SpriteInstance& s0, const SpriteInstance& s1)
		{
		    return s0.layer < s1.layer;
		});

	    GPUBuffer* batch_buffer = get_batch_buffer(sizeof(SpriteData), cmd);
	    SpriteData& data = *(SpriteData*)renderer->batch_data[cmd];

	    // Prepare
	    graphics_event_begin("Sprite_GeometryPass", cmd);

	    graphics_topology_set(GraphicsTopology_Triangles, cmd);
	    graphics_vertexbuffer_bind(batch_buffer, 0u, 0u, cmd);
	    graphics_indexbuffer_bind(gfx.ibuffer_sprite, 0u, cmd);
	    graphics_inputlayoutstate_bind(gfx.ils_sprite, cmd);
	    graphics_sampler_bind(gfx.sampler_def_nearest, 0u, ShaderType_Pixel, cmd);
	    graphics_shader_bind(gfx.vs_sprite, cmd);
	    graphics_shader_bind(gfx.ps_sprite, cmd);
	    graphics_blendstate_bind(gfx.bs_transparent, cmd);
	    graphics_depthstencilstate_unbind(cmd);

	    GPUImage* att[2];
	    att[0] = gfx.offscreen;
	    att[1] = gfx.gbuffer_depthstencil;

	    XMMATRIX matrix;
	    XMVECTOR pos0, pos1, pos2, pos3;

	    // Batch data, used to update the vertex buffer
	    SpriteVertex* batchIt = data.data;

	    // Used to draw
	    u32 instanceCount;

	    const SpriteInstance* it = sprite_instances.data();
	    const SpriteInstance* end = it + sprite_instances.size();

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

		    matrix = XMMatrixMultiply(spr.tm, camera_data.view_projection_matrix);

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

		graphics_renderpass_begin(gfx.renderpass_world, att, nullptr, 1.f, 0u, cmd);

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

		end = sprite_instances.data() + sprite_instances.size();
		batchIt = data.data;

		graphics_event_end(cmd);
	    }
	}
    }

    SV_AUX bool update_light_buffer(const GPU_CameraData& camera_data, u32 offset, CommandList cmd)
    {
	// Send light data
	u32 light_count = SV_MIN(LIGHT_COUNT, u32(light_instances.size()) - offset);
			    
	GPU_LightData light_data[LIGHT_COUNT] = {};

	XMMATRIX rotation_view_matrix;
	{
	    XMFLOAT4X4 vm;
	    XMStoreFloat4x4(&vm, camera_data.view_matrix);
	    vm._41 = 0.f;
	    vm._42 = 0.f;
	    vm._43 = 0.f;
	    vm._44 = 1.f;

	    rotation_view_matrix = XMLoadFloat4x4(&vm);
	}

	foreach(i, light_count) {

	    GPU_LightData& l0 = light_data[i];
	    const LightInstance& l1 = light_instances[i + offset];

	    l0.type = l1.type;
	    l0.color = l1.color.toVec3();
	    l0.intensity = l1.intensity;

	    switch (l1.type)
	    {
	    case LightType_Point:
		l0.position = v3_f32(XMVector4Transform(l1.position.getDX(1.f), camera_data.view_matrix));
		l0.range = l1.range;
		l0.smoothness = l1.smoothness;
		break;

	    case LightType_Direction:
		l0.position = v3_f32(XMVector3Transform(l1.position.getDX(1.f), rotation_view_matrix));
		break;
	    }
	}

	graphics_buffer_update(renderer->gfx.cbuffer_light_instances, light_data, sizeof(GPU_LightData) * LIGHT_COUNT, 0u, cmd);

	return light_count;
    }

    void _draw_scene()
    {
	if (!there_is_scene()) return;
	
	auto& gfx = renderer->gfx;
	    
	mesh_instances.reset();
	light_instances.reset();
	sprite_instances.reset();

	CommandList cmd = graphics_commandlist_get();

	SceneData* scene = get_scene_data();

	// Create environment buffer
	{
	    EnvironmentData data;
	    data.ambient_light = scene->ambient_light.toVec3();
	    graphics_buffer_update(gfx.cbuffer_environment, &data, sizeof(EnvironmentData), 0u, cmd);
	}

	// Get lights
	{
	    ComponentIterator it;
	    CompView<LightComponent> v;

	    if (comp_it_begin(it, v)) {

		do {

		    Entity entity = v.entity;
		    LightComponent& l = *v.comp;

		    switch (l.light_type)
		    {
		    case LightType_Point:
			light_instances.emplace_back(l.color, get_entity_world_position(entity), l.range, l.intensity, l.smoothness);
			break;

		    case LightType_Direction:
		    {
			XMVECTOR direction = XMVectorSet(0.f, 0.f, 1.f, 1.f);

			direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(get_entity_world_rotation(entity).get_dx()));

			light_instances.emplace_back(l.color, v3_f32(direction), l.intensity);
		    }
		    break;

		    }

		}
		while(comp_it_next(it, v));
	    }
	}

	// Draw cameras
	{

#if SV_DEV
	    CameraComponent* camera_ = nullptr;
	    GPU_CameraData camera_data;

	    {
		v3_f32 cam_pos;
		v4_f32 cam_rot;
				
		if (dev.debug_draw) {

		    camera_ = &dev.camera;
		    cam_pos = dev.camera.position;
		    cam_rot = dev.camera.rotation;
		}
		else {
					
		    camera_ = get_main_camera();
		    
		    if (camera_) {

			cam_pos = get_entity_world_position(scene->main_camera);
			cam_rot = get_entity_world_rotation(scene->main_camera);
		    }
		}

		if (camera_) {
						
		    camera_data.projection_matrix = camera_->projection_matrix;
		    camera_data.position = cam_pos.getVec4(0.f);
		    camera_data.rotation = cam_rot;
		    camera_data.view_matrix = camera_->view_matrix;
		    camera_data.view_projection_matrix = camera_->view_projection_matrix;
		    camera_data.inverse_view_matrix = camera_->inverse_view_matrix;
		    camera_data.inverse_projection_matrix = camera_->inverse_projection_matrix;
		    camera_data.inverse_view_projection_matrix = camera_->inverse_view_projection_matrix;
		    camera_data.screen_width = 1920.f;
		    camera_data.screen_height = 1080.f;
		    camera_data.near = camera_->near;
		    camera_data.far = camera_->far;
		    
		    graphics_buffer_update(gfx.cbuffer_camera, &camera_data, sizeof(GPU_CameraData), 0u, cmd);
		}
	    }
#else
			
	    CameraComponent* camera_ = get_main_camera();
	    CameraBuffer_GPU camera_data;

	    if (camera_) {

		Entity cam = get_scene_data()->main_camera;
		camera_data.projection_matrix = camera_->projection_matrix;
		camera_data.position = get_entity_world_position(cam).getVec4();
		camera_data.rotation = get_entity_world_rotation(cam);
		camera_data.view_matrix = camera_->view_matrix;
		camera_data.view_projection_matrix = camera_->view_projection_matrix;
		camera_data.inverse_view_matrix = camera_->inverse_view_matrix;
		camera_data.inverse_projection_matrix = camera_->inverse_projection_matrix;
		camera_data.inverse_view_projection_matrix = camera_->inverse_view_projection_matrix;
		camera_data.screen_width = 1920.f;
		camera_data.screen_height = 1080.f;
		camera_data.near = camera_->near;
		camera_data.far = camera_->far;
		
		graphics_buffer_update(gfx.cbuffer_camera, &camera_data, sizeof(GPU_CameraData), 0u, cmd);
	    }

#endif
		
	    if (camera_) {

		CameraComponent& camera = *camera_;

		if (scene->skybox && camera.projection_type == ProjectionType_Perspective)
		    draw_sky(scene->skybox, camera_data.view_matrix, camera_data.projection_matrix, cmd);

		// DRAW MESHES
		{
		    {
			ComponentIterator it;
			CompView<MeshComponent> view;

			if (comp_it_begin(it, view)) {

			    do {

				MeshComponent& mesh = *view.comp;
				Entity entity = view.entity;

				Mesh* m = mesh.mesh.get();
				if (m == nullptr) continue;

				mesh_instances.emplace_back(get_entity_world_matrix(entity), m, mesh.material.get());
			    }
			    while (comp_it_next(it, view));
			}
		    }
		    
		    if (mesh_instances.size()) {

			graphics_event_begin("MeshRendering", cmd);
			
			// Prepare state
			graphics_shader_bind(gfx.vs_mesh_default, cmd);
			graphics_shader_bind(gfx.ps_mesh_default, cmd);
			graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
			graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
			graphics_blendstate_bind(gfx.bs_mesh, cmd);
			graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

			// Bind resources
			GPUBuffer* instance_buffer = gfx.cbuffer_mesh_instance;
			GPUBuffer* material_buffer = gfx.cbuffer_material;

			graphics_constantbuffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_camera, 1u, ShaderType_Vertex, cmd);
						
			graphics_constantbuffer_bind(material_buffer, 0u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_light_instances, 1u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_environment, 2u, ShaderType_Pixel, cmd);

			// Begin renderpass
			GPUImage* att[] = { gfx.offscreen, gfx.gbuffer_normal, gfx.gbuffer_emission, gfx.gbuffer_depthstencil };
			graphics_renderpass_begin(gfx.renderpass_gbuffer, att, cmd);

			// TODO: Multiple lights
			update_light_buffer(camera_data, 0u, cmd);

			foreach(i, mesh_instances.size()) {

			    const MeshInstance& inst = mesh_instances[i];

			    graphics_vertexbuffer_bind(inst.mesh->vbuffer, 0u, 0u, cmd);
			    graphics_indexbuffer_bind(inst.mesh->ibuffer, 0u, cmd);

			    // Update material data
			    {
				GPU_MaterialData material_data;
				material_data.flags = 0u;

				if (inst.material) {

				    GPUImage* diffuse_map = inst.material->diffuse_map.get();
				    GPUImage* normal_map = inst.material->normal_map.get();
				    GPUImage* specular_map = inst.material->specular_map.get();
				    GPUImage* emissive_map = inst.material->emissive_map.get();

				    graphics_image_bind(diffuse_map ? diffuse_map : gfx.image_white, 0u, ShaderType_Pixel, cmd);
				    if (normal_map) { graphics_image_bind(normal_map, 1u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_NORMAL_MAPPING; }
				    else graphics_image_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);

				    if (specular_map) { graphics_image_bind(specular_map, 2u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_SPECULAR_MAPPING; }
				    else graphics_image_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);

				    if (emissive_map) { graphics_image_bind(emissive_map, 3u, ShaderType_Pixel, cmd); material_data.flags |= MAT_FLAG_EMISSIVE_MAPPING; }
				    else graphics_image_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

				    material_data.diffuse_color = inst.material->diffuse_color.toVec3();
				    material_data.specular_color = inst.material->specular_color.toVec3();
				    material_data.emissive_color = inst.material->emissive_color.toVec3();
				    material_data.shininess = inst.material->shininess;

				    switch (inst.material->culling) {

				    case RasterizerCullMode_Back:
					graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
					break;
					
				    case RasterizerCullMode_Front:
					// TODO: graphics_rasterizerstate_bind(gfx.rs_front_culling, cmd);
					break;

				    case RasterizerCullMode_None:
					graphics_rasterizerstate_unbind(cmd);
					break;
					
				    }
				}
				else {

				    graphics_image_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

				    material_data.diffuse_color = Color::Gray(160u).toVec3();
				    material_data.specular_color = Color::Gray(160u).toVec3();
				    material_data.emissive_color = Color::Black().toVec3();
				    material_data.shininess = 0.5f;

				    graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
				}

				graphics_buffer_update(material_buffer, &material_data, sizeof(GPU_MaterialData), 0u, cmd);
			    }

			    // Update instance data
			    {
				GPU_MeshInstanceData mesh_data;
				mesh_data.model_view_matrix = inst.transform_matrix * camera_data.view_matrix;
				mesh_data.inv_model_view_matrix = XMMatrixInverse(nullptr, mesh_data.model_view_matrix);

				graphics_buffer_update(instance_buffer, &mesh_data, sizeof(GPU_MeshInstanceData), 0u, cmd);
			    }

			    graphics_draw_indexed(u32(inst.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);
			}

			graphics_renderpass_end(cmd);

			graphics_event_end(cmd);
		    }

		    draw_sprites(camera_data, cmd);
		}

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
			0.8f, 0.03f, os_window_aspect(), cmd);
	    }
	    else {
				
		constexpr f32 SIZE = 0.4f;
		constexpr const char* TEXT = "No Main Camera";
		draw_text(TEXT, strlen(TEXT), -1.f, +SIZE * 0.5f, 2.f, 1u, SIZE, os_window_aspect(), TextAlignment_Center, nullptr, cmd);
	    }

	    event_dispatch("draw_scene", nullptr);
	}
    }

    void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd)
    {
	auto& gfx = renderer->gfx;
	
	graphics_depthstencilstate_unbind(cmd);
	graphics_blendstate_unbind(cmd);
	graphics_inputlayoutstate_bind(gfx.ils_sky, cmd);
	graphics_rasterizerstate_unbind(cmd);
	graphics_topology_set(GraphicsTopology_Triangles, cmd);

	graphics_vertexbuffer_bind(gfx.vbuffer_skybox, 0u, 0u, cmd);
	graphics_constantbuffer_bind(gfx.cbuffer_skybox, 0u, ShaderType_Vertex, cmd);
	graphics_image_bind(skymap, 0u, ShaderType_Pixel, cmd);
	graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

	XMFLOAT4X4 vm;
	XMStoreFloat4x4(&vm, view_matrix);
	vm._41 = 0.f;
	vm._42 = 0.f;
	vm._43 = 0.f;
	vm._44 = 1.f;

	view_matrix = XMLoadFloat4x4(&vm);
	view_matrix = view_matrix * projection_matrix;

	graphics_buffer_update(gfx.cbuffer_skybox, &view_matrix, sizeof(XMMATRIX), 0u, cmd);

	graphics_shader_bind(gfx.vs_sky, cmd);
	graphics_shader_bind(gfx.ps_sky, cmd);

	GPUImage* att[] = { gfx.offscreen };
	graphics_renderpass_begin(gfx.renderpass_off, att, 0, 1.f, 0u, cmd);
	graphics_draw(36u, 1u, 0u, 0u, cmd);
	graphics_renderpass_end(cmd);
    }

    SV_INLINE static void text_draw_call(GPUImage* offscreen, GPUBuffer* buffer, TextData& data, u32 vertex_count, CommandList cmd)
    {
	if (vertex_count == 0u) return;

	auto& gfx = renderer->gfx;
	
	graphics_buffer_update(buffer, data.vertices, vertex_count * sizeof(TextVertex), 0u, cmd);

	GPUImage* att[1];
	att[0] = offscreen;

	graphics_renderpass_begin(gfx.renderpass_off, att, nullptr, 1.f, 0u, cmd);
	graphics_draw_indexed(vertex_count / 4u * 6u, 1u, 0u, 0u, 0u, cmd);
	graphics_renderpass_end(cmd);
    }

    u32 draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pFont, Color color, CommandList cmd)
    {
	if (text == nullptr) return 0u;
	if (text_size == 0u) return 0u;

	auto& gfx = renderer->gfx;
	
	// Select font
	Font& font = pFont ? *pFont : renderer->font_opensans;
		
	// Prepare
	graphics_rasterizerstate_unbind(cmd);
	graphics_blendstate_unbind(cmd);
	graphics_depthstencilstate_unbind(cmd);
	graphics_topology_set(GraphicsTopology_Triangles, cmd);

	graphics_shader_bind(gfx.vs_text, cmd);
	graphics_shader_bind(gfx.ps_text, cmd);

	GPUBuffer* buffer = get_batch_buffer(sizeof(TextData), cmd);
	graphics_vertexbuffer_bind(buffer, 0u, 0u, cmd);
	graphics_indexbuffer_bind(gfx.ibuffer_text, 0u, cmd);
	graphics_image_bind(font.image, 0u, ShaderType_Pixel, cmd);
	graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

	graphics_inputlayoutstate_bind(gfx.ils_text, cmd);

	TextData& data = *reinterpret_cast<TextData*>(renderer->batch_data[cmd]);

	// Text space transformation
	f32 xmult = font_size / aspect;
	f32 ymult = font_size;

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

	    // TODO: add first char offset to xoff
	    const char* line_end = process_text_line(it, u32(text_size - (it - text)), max_line_width / xmult, font);
	    SV_ASSERT(line_end != it);

	    if (vertex_count + (line_end - it) > TEXT_BATCH_COUNT) {

		text_draw_call(gfx.offscreen, buffer, data, vertex_count, cmd);
		vertex_count = 0u;
	    }

	    if (yoff + y > 1.f || yoff + line_height + y < -1.f) {
		it = line_end;
		continue;
	    }

	    // Fill batch
	    while (it != line_end) {

		Glyph* g_ = font.get(*it);

		if (g_) {

		    Glyph& g = *g_;

		    f32 advance = g.advance * xmult;

		    if (*it != ' ') {
			f32 xpos = xoff + g.xoff * xmult + x;
			f32 ypos = yoff + g.yoff * ymult + y;
			f32 width = g.w * xmult;
			f32 height = g.h * ymult;
			
			data.vertices[vertex_count++] = { v4_f32{ xpos	        , ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.w }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.w }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos		, ypos		, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.y }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos		, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.y }, color };
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

	number_of_chars = u32(it - text);

	text_draw_call(gfx.offscreen, buffer, data, vertex_count, cmd);

	return number_of_chars;
    }

    void draw_text_area(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pFont, Color color, CommandList cmd)
    {
	if (text == nullptr) return;
	if (text_size == 0u) return;

	// Select font
	Font& font = pFont ? *pFont : renderer->font_opensans;

	f32 transformed_max_width = max_line_width / (font_size / aspect);

	// Select text to render
	if (bottom_top) {
			
	    // Count lines
	    u32 lines = 0u;

	    const char* it = text;
	    const char* end = text + text_size;

	    while (it != end) {

		it = process_text_line(it, u32(text_size - (it - text)), transformed_max_width, font);
		++lines;
	    }

	    u32 begin_line_index = lines - SV_MIN(max_lines, lines);
	    begin_line_index -= SV_MIN(begin_line_index, line_offset);

	    it = text;
	    lines = 0u;

	    while (it != end && lines != begin_line_index) {

		it = process_text_line(it, u32(text_size - (it - text)), transformed_max_width, font);
		++lines;
	    }

	    text_size = text_size - (it - text);
	    text = it;
	}
	else if (line_offset) {

	    const char* it = text;
	    const char* end = text + text_size;

	    foreach (i, line_offset) {

		it = process_text_line(it, u32(text_size - (it - text)), max_line_width / font_size / aspect, font);
		if (it == end) return;
	    }

	    text_size = text_size - (it - text);
	    text = it;
	}

	draw_text(text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, &font, cmd);
    }

    Font& renderer_default_font()
    {
	return renderer->font_opensans;
    }

    GPUImage* renderer_white_image()
    {
	return renderer->gfx.image_white;
    }

    // POSTPROCESSING

    void postprocess_gaussian_blur(
	    GPUImage* src,
	    GPUImageLayout src_layout0,
	    GPUImageLayout src_layout1,
	    GPUImage* aux,
	    GPUImageLayout aux_layout0,
	    GPUImageLayout aux_layout1,
	    f32 intensity,
	    f32 aspect,
	    CommandList cmd,
	    RenderPass* renderpass
	)
    {
	GPUBarrier barriers[2u];
	u32 barrier_count = 0u;
	GPUImage* att[1u];
	GaussianBlurData data;

	auto& gfx = renderer->gfx;

	if (renderpass == nullptr)
	    renderpass = gfx.renderpass_off;
	
	GPUImageInfo src_info = graphics_image_info(src);
	GPUImageInfo aux_info = graphics_image_info(aux);
		
	// Prepare image layouts
	{
	    if (src_layout0 != GPUImageLayout_ShaderResource) {

		barriers[0u] = GPUBarrier::Image(src, src_layout0, GPUImageLayout_ShaderResource);
		++barrier_count;
	    }
	    if (aux_layout0 != GPUImageLayout_RenderTarget) {

		barriers[barrier_count++] = GPUBarrier::Image(aux, aux_layout0, GPUImageLayout_RenderTarget);
	    }
	    if (barrier_count) {
		graphics_barrier(barriers, barrier_count, cmd);
	    }
	}

	// Prepare graphics state
	{
	    graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
	    graphics_rasterizerstate_unbind(cmd);
	    graphics_blendstate_unbind(cmd);
	    graphics_depthstencilstate_unbind(cmd);
	    graphics_inputlayoutstate_unbind(cmd);

	    graphics_shader_bind(gfx.vs_default_postprocess, cmd);
	    graphics_shader_bind(gfx.ps_gaussian_blur, cmd);

	    graphics_constantbuffer_bind(gfx.cbuffer_gaussian_blur, 0u, ShaderType_Pixel, cmd);
	    graphics_sampler_bind(gfx.sampler_blur, 0u, ShaderType_Pixel, cmd);
	}
	
	// Horizontal blur
	{
	    graphics_image_bind(src, 0u, ShaderType_Pixel, cmd);
	    graphics_viewport_set(aux, 0u, cmd);
	    graphics_scissor_set(aux, 0u, cmd);

	    att[0u] = aux;

	    data.intensity = intensity * (f32(aux_info.width) / f32(src_info.width));
	    if (aspect < 1.f) {
		data.intensity /= aspect;
	    }

	    data.horizontal = 1u;
	    graphics_buffer_update(gfx.cbuffer_gaussian_blur, &data, sizeof(GaussianBlurData), 0u, cmd);

	    graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
	    postprocessing_draw_call(cmd);
	    graphics_renderpass_end(cmd);
	}

	// Vertical blur
	{
	    barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
	    barriers[1u] = GPUBarrier::Image(aux, GPUImageLayout_RenderTarget, GPUImageLayout_ShaderResource);
	    graphics_barrier(barriers, 2u, cmd);

	    graphics_image_bind(aux, 0u, ShaderType_Pixel, cmd);
	    graphics_viewport_set(src, 0u, cmd);
	    graphics_scissor_set(src, 0u, cmd);

	    att[0u] = src;

	    data.intensity = intensity * (f32(aux_info.height) / f32(src_info.height));
	    if (aspect > 1.f) {
		data.intensity *= aspect;
	    }

	    data.horizontal = 0u;
	    graphics_buffer_update(gfx.cbuffer_gaussian_blur, &data, sizeof(GaussianBlurData), 0u, cmd);
			
	    graphics_renderpass_begin(renderpass, att, cmd);
	    postprocessing_draw_call(cmd);
	    graphics_renderpass_end(cmd);
	}

	// Change images layout
	{
	    barrier_count = 0u;

	    if (src_layout1 != GPUImageLayout_RenderTarget) {

		barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_RenderTarget, src_layout1);
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

    void postprocess_addition(
	    GPUImage* dst,
	    GPUImage* src,
	    CommandList cmd
	)
    {
	auto& gfx = renderer->gfx;
	
	// Prepare graphics state
	graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
	graphics_rasterizerstate_unbind(cmd);
	graphics_depthstencilstate_unbind(cmd);
	graphics_inputlayoutstate_unbind(cmd);
	graphics_shader_bind(gfx.vs_default_postprocess, cmd);
	graphics_shader_bind(gfx.ps_default_postprocess, cmd);
	graphics_blendstate_bind(gfx.bs_addition, cmd);
	graphics_viewport_set(dst, 0u, cmd);
	graphics_scissor_set(dst, 0u, cmd);
		
	GPUImage* att[1u];
	att[0u] = dst;

	graphics_image_bind(src, 0u, ShaderType_Pixel, cmd);
	graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);
		
	graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
	postprocessing_draw_call(cmd);
	graphics_renderpass_end(cmd);
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
	    f32 aspect,
	    CommandList cmd
	)
    {
	GPUBarrier barriers[3];
	u32 barrier_count = 0u;
	GPUImage* att[1u];

	auto& gfx = renderer->gfx;

	// Prepare graphics state
	{
	    graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
	    graphics_rasterizerstate_unbind(cmd);
	    graphics_blendstate_unbind(cmd);
	    graphics_depthstencilstate_unbind(cmd);
	    graphics_inputlayoutstate_unbind(cmd);
	    graphics_shader_bind(gfx.vs_default_postprocess, cmd);
	}

	// Threshold pass
	{
	    // Prepare image layouts
	    if (src_layout0 != GPUImageLayout_ShaderResource) {
		barriers[0u] = GPUBarrier::Image(src, src_layout0, GPUImageLayout_ShaderResource);
		++barrier_count;
	    }
	    if (aux0_layout0 != GPUImageLayout_RenderTarget) {
		barriers[barrier_count++] = GPUBarrier::Image(aux0, aux0_layout0, GPUImageLayout_RenderTarget);
	    }
	    if (emission_layout0 != GPUImageLayout_ShaderResource) {
		barriers[barrier_count++] = GPUBarrier::Image(emission, emission_layout0, GPUImageLayout_ShaderResource);
	    }
			
	    if (barrier_count) {
		graphics_barrier(barriers, barrier_count, cmd);
	    }
			
	    graphics_image_bind(src, 0u, ShaderType_Pixel, cmd);
	    graphics_sampler_bind(gfx.sampler_blur, 0u, ShaderType_Pixel, cmd);
	    graphics_constantbuffer_bind(gfx.cbuffer_bloom_threshold, 0u, ShaderType_Pixel, cmd);
	    graphics_shader_bind(gfx.ps_bloom_threshold, cmd);
	    graphics_viewport_set(aux0, 0u, cmd);
	    graphics_scissor_set(aux0, 0u, cmd);

	    graphics_buffer_update(gfx.cbuffer_bloom_threshold, &threshold, sizeof(f32), 0u, cmd);
			
	    att[0u] = aux0;
			
	    graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
	    postprocessing_draw_call(cmd);
	    graphics_renderpass_end(cmd);
	}

	// Add emissive map
	if (emission) {

	    postprocess_addition(aux0, emission, cmd);
	}

	// Blur image
	postprocess_gaussian_blur(
		aux0,
		GPUImageLayout_RenderTarget,
		GPUImageLayout_ShaderResource,
		aux1,
		aux1_layout0,
		aux1_layout1,
		intensity,
		aspect,
		cmd);

	// Color addition
	{
	    barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_ShaderResource, GPUImageLayout_RenderTarget);
	    graphics_barrier(barriers, 1u, cmd);
			
	    postprocess_addition(src, aux0, cmd);
	}

	// Last memory barriers
	barrier_count = 0u;

	if (src_layout1 != GPUImageLayout_RenderTarget) {

	    barriers[0u] = GPUBarrier::Image(src, GPUImageLayout_RenderTarget, src_layout1);
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
    }

    ///////////////////////////////////////////////// IMMEDIATE MODE RENDERER ///////////////////////////////////////////////////////////////////////

#define SV_IMREND() auto& state = sv::renderer->immediate_mode_state[cmd]

    enum ImRendHeader : u32 {
	ImRendHeader_PushMatrix,
	ImRendHeader_PopMatrix,
	ImRendHeader_PushScissor,
	ImRendHeader_PopScissor,

	ImRendHeader_Camera,

	ImRendHeader_DrawCall,
    };

    enum ImRendDrawCall : u32 {
	ImRendDrawCall_Quad,
	ImRendDrawCall_Sprite,
	ImRendDrawCall_Triangle,
	ImRendDrawCall_Line,
	
	ImRendDrawCall_MeshWireframe,
	
	ImRendDrawCall_Text,
	ImRendDrawCall_TextArea,
    };

    /* TODO 
       - While is updating matrices or cameras, not update every time
       - With scissors to
     */

    template<typename T>
    SV_AUX void imrend_write(ImmediateModeState& state, const T& t)
    {
	state.buffer.write_back(&t, sizeof(T));
    }

    template<typename T>
    SV_AUX T imrend_read(u8*& it)
    {
	T t;
	memcpy(&t, it, sizeof(T));
	it += sizeof(T);
	return t;
    }

    SV_AUX void update_current_matrix(ImmediateModeState& state)
    {
	XMMATRIX matrix = XMMatrixIdentity();

	for (const XMMATRIX& m : state.matrix_stack) {

	    matrix = XMMatrixMultiply(matrix, m);
	}

	XMMATRIX vpm;
	
	switch (state.current_camera)
	{
	case ImRendCamera_Normal:
	    vpm = XMMatrixScaling(2.f, 2.f, 1.f) * XMMatrixTranslation(-1.f, -1.f, 0.f);
	    break;

	case ImRendCamera_Editor:
	    vpm = dev.camera.view_projection_matrix;
	    break;

	case ImRendCamera_Clip:
	default:
	    vpm = XMMatrixIdentity();
	    
	}

	state.current_matrix = matrix * vpm;
    }

    SV_AUX void update_current_scissor(ImmediateModeState& state, CommandList cmd)
    {
	v4_f32 s0 = { 0.5f, 0.5f, 1.f, 1.f };

	u32 begin_index = 0u;

	if (state.scissor_stack.size()) {
	    
	    for (i32 i = (i32)state.scissor_stack.size() - 1u; i >= 0; --i) {

		if (!state.scissor_stack[i].additive) {
		    begin_index = i;
		    break;
		}
	    }

	    for (u32 i = begin_index; i < (u32)state.scissor_stack.size(); ++i) {

		const v4_f32& s1 = state.scissor_stack[i].bounds;

		f32 min0 = s0.x - s0.z * 0.5f;
		f32 max0 = s0.x + s0.z * 0.5f;
	    
		f32 min1 = s1.x - s1.z * 0.5f;
		f32 max1 = s1.x + s1.z * 0.5f;
	    
		f32 min = SV_MAX(min0, min1);
		f32 max = SV_MIN(max0, max1);
		
		if (min >= max) {
		    s0 = {};
		    break;
		}
	    
		s0.z = max - min;
		s0.x = min + s0.z * 0.5f;
	    
		min0 = s0.y - s0.w * 0.5f;
		max0 = s0.y + s0.w * 0.5f;
	    
		min1 = s1.y - s1.w * 0.5f;
		max1 = s1.y + s1.w * 0.5f;

		min = SV_MAX(min0, min1);
		max = SV_MIN(max0, max1);

		if (min >= max) {
		    s0 = {};
		    break;
		}

		s0.w = max - min;
		s0.y = min + s0.w * 0.5f;
	    }
	}

	const GPUImageInfo& info = graphics_image_info(renderer->gfx.offscreen);
	
	Scissor s;
	s.width = u32(s0.z * f32(info.width));
	s.height = u32(s0.w * f32(info.height));
	s.x = u32(s0.x * f32(info.width) - s0.z * f32(info.width) * 0.5f);
	s.y = u32(s0.y * f32(info.height) - s0.w * f32(info.height) * 0.5f);

	graphics_scissor_set(s, 0u, cmd);
    }
    
    void imrend_begin_batch(CommandList cmd)
    {
	SV_IMREND();

	state.buffer.reset();
    }
    
    void imrend_flush(CommandList cmd)
    {
	SV_IMREND();

	graphics_event_begin("Immediate Rendering", cmd);

	auto& gfx = renderer->gfx;
	
	GPUImage* att[1];
	att[0] = gfx.offscreen;

	graphics_renderpass_begin(gfx.renderpass_off, att, cmd);

	state.current_matrix = XMMatrixIdentity();
	state.matrix_stack.reset();
	state.scissor_stack.reset();
	state.current_camera = ImRendCamera_Clip;

	u8* it = (u8*)state.buffer.data();
	u8* end = (u8*)state.buffer.data() + state.buffer.size();

	while (it != end)
	{
	    ImRendHeader header = imrend_read<ImRendHeader>(it);
	    
	    switch (header) {

	    case ImRendHeader_PushMatrix:
	    {
		XMMATRIX m = imrend_read<XMMATRIX>(it);
		state.matrix_stack.push_back(m);
		update_current_matrix(state);
	    }
	    break;
		
	    case ImRendHeader_PopMatrix:
	    {
		state.matrix_stack.pop_back();
		update_current_matrix(state);
	    }
	    break;

	    case ImRendHeader_PushScissor:
	    {
		ImRendScissor s;
		s.bounds = imrend_read<v4_f32>(it);
		s.additive = imrend_read<bool>(it);
		
		state.scissor_stack.push_back(s);
		update_current_scissor(state, cmd);
	    }
	    break;
	    
	    case ImRendHeader_PopScissor:
	    {
		state.scissor_stack.pop_back();
		update_current_scissor(state, cmd);
	    }
	    break;

	    case ImRendHeader_Camera:
	    {
		state.current_camera = imrend_read<ImRendCamera>(it);
		update_current_matrix(state);
	    }
	    break;
		
	    case ImRendHeader_DrawCall:
	    {
		ImRendDrawCall draw_call = imrend_read<ImRendDrawCall>(it);

		switch (draw_call) {

		case ImRendDrawCall_Quad:
		case ImRendDrawCall_Sprite:
		case ImRendDrawCall_Triangle:
		case ImRendDrawCall_Line:
		{
		    graphics_constantbuffer_bind(gfx.cbuffer_im_primitive[cmd], 0u, ShaderType_Vertex, cmd);

		    graphics_blendstate_bind(gfx.bs_transparent, cmd);
		    graphics_depthstencilstate_unbind(cmd);
		    graphics_inputlayoutstate_unbind(cmd);
		    graphics_rasterizerstate_unbind(cmd);

		    graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);
		    
		    graphics_shader_bind(gfx.vs_im_primitive, cmd);
		    graphics_shader_bind(gfx.ps_im_primitive, cmd);

		    if (draw_call == ImRendDrawCall_Quad || draw_call == ImRendDrawCall_Sprite) {

			graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);
			
			v3_f32 position = imrend_read<v3_f32>(it);
			v2_f32 size = imrend_read<v2_f32>(it);
			Color color = imrend_read<Color>(it);
			GPUImage* image = gfx.image_white;
			v4_f32 tc = { 0.f, 0.f, 1.f, 1.f };

			if (draw_call == ImRendDrawCall_Sprite) {
			    image = imrend_read<GPUImage*>(it);
			    tc = imrend_read<v4_f32>(it);
			}

			XMMATRIX m = XMMatrixScaling(size.x, size.y, 1.f) * XMMatrixTranslation(position.x, position.y, position.z);

			m *= state.current_matrix;

			XMVECTOR v0 = XMVector4Transform(XMVectorSet(-0.5f, 0.5f, 0.f, 1.f), m);
			XMVECTOR v1 = XMVector4Transform(XMVectorSet(0.5f, 0.5f, 0.f, 1.f), m);
			XMVECTOR v2 = XMVector4Transform(XMVectorSet(-0.5f, -0.5f, 0.f, 1.f), m);
			XMVECTOR v3 = XMVector4Transform(XMVectorSet(0.5f, -0.5f, 0.f, 1.f), m);
		    
			ImRendVertex vertices[4u];
			vertices[0u] = { v4_f32(v0), v2_f32{tc.x, tc.y}, color };
			vertices[1u] = { v4_f32(v1), v2_f32{tc.z, tc.y}, color };
			vertices[2u] = { v4_f32(v2), v2_f32{tc.x, tc.w}, color };
			vertices[3u] = { v4_f32(v3), v2_f32{tc.z, tc.w}, color };

			graphics_image_bind(image, 0u, ShaderType_Pixel, cmd);

			graphics_buffer_update(gfx.cbuffer_im_primitive[cmd], vertices, sizeof(ImRendVertex) * 4u, 0u, cmd);

			graphics_draw(4u, 1u, 0u, 0u, cmd);
		    }
		    else if (draw_call == ImRendDrawCall_Triangle) {

			graphics_topology_set(GraphicsTopology_TriangleStrip, cmd);

			v3_f32 p0 = imrend_read<v3_f32>(it);
			v3_f32 p1 = imrend_read<v3_f32>(it);
			v3_f32 p2 = imrend_read<v3_f32>(it);
			Color color = imrend_read<Color>(it);

			XMMATRIX m = state.current_matrix;

			XMVECTOR v0 = XMVector4Transform(p0.getDX(1.f), m);
			XMVECTOR v1 = XMVector4Transform(p1.getDX(1.f), m);
			XMVECTOR v2 = XMVector4Transform(p2.getDX(1.f), m);
		    
			ImRendVertex vertices[3u];
			vertices[0u] = { v4_f32(v0), v2_f32{}, color };
			vertices[1u] = { v4_f32(v1), v2_f32{}, color };
			vertices[2u] = { v4_f32(v2), v2_f32{}, color };

			graphics_image_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);

			graphics_buffer_update(gfx.cbuffer_im_primitive[cmd], vertices, sizeof(ImRendVertex) * 3u, 0u, cmd);

			graphics_draw(3u, 1u, 0u, 0u, cmd);
		    }
		    else if (draw_call == ImRendDrawCall_Line) {

			graphics_topology_set(GraphicsTopology_Lines, cmd);

			v3_f32 p0 = imrend_read<v3_f32>(it);
			v3_f32 p1 = imrend_read<v3_f32>(it);
			Color color = imrend_read<Color>(it);

			XMMATRIX m = state.current_matrix;

			XMVECTOR v0 = XMVector4Transform(p0.getDX(1.f), m);
			XMVECTOR v1 = XMVector4Transform(p1.getDX(1.f), m);
		    
			ImRendVertex vertices[2u];
			vertices[0u] = { v4_f32(v0), v2_f32{}, color };
			vertices[1u] = { v4_f32(v1), v2_f32{}, color };

			graphics_image_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);

			graphics_buffer_update(gfx.cbuffer_im_primitive[cmd], vertices, sizeof(ImRendVertex) * 2u, 0u, cmd);

			graphics_draw(3u, 1u, 0u, 0u, cmd);
		    }
		    
		}break;

		case ImRendDrawCall_MeshWireframe:
		{
		    graphics_image_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);
		    graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
		    graphics_shader_bind(gfx.vs_im_mesh_wireframe, cmd);
		    graphics_shader_bind(gfx.ps_im_primitive, cmd);
		    graphics_topology_set(GraphicsTopology_Triangles, cmd);
		    graphics_constantbuffer_bind(gfx.cbuffer_im_mesh[cmd], 0u, ShaderType_Vertex, cmd);
		    graphics_rasterizerstate_bind(gfx.rs_wireframe, cmd);
		    graphics_blendstate_bind(gfx.bs_transparent, cmd);

		    Mesh* mesh = imrend_read<Mesh*>(it);
		    Color color = imrend_read<Color>(it);

		    graphics_vertexbuffer_bind(mesh->vbuffer, 0u, 0u, cmd);
		    graphics_indexbuffer_bind(mesh->ibuffer, 0u, cmd);

		    struct {
			XMMATRIX matrix;
			v4_f32 color;
		    } data;

		    data.matrix = state.current_matrix;
		    data.color = color.toVec4();

		    graphics_buffer_update(gfx.cbuffer_im_mesh[cmd], &data, sizeof(data), 0u, cmd);
		    graphics_draw_indexed((u32)mesh->indices.size(), 1u, 0u, 0u, 0u, cmd);
		}
		break;
		
		case ImRendDrawCall_TextArea:
		{
		    const char* text = (const char*)it;
		    it += strlen(text) + 1u;

		    size_t text_size = imrend_read<size_t>(it);
		    f32 x = imrend_read<f32>(it);
		    f32 y = imrend_read<f32>(it);
		    f32 max_line_width = imrend_read<f32>(it);
		    u32 max_lines = imrend_read<u32>(it);
		    f32 font_size = imrend_read<f32>(it);
		    f32 aspect = imrend_read<f32>(it);
		    TextAlignment alignment = imrend_read<TextAlignment>(it);
		    u32 line_offset = imrend_read<u32>(it);
		    bool bottom_top = imrend_read<bool>(it);
		    Font* pfont = imrend_read<Font*>(it);
		    Color color = imrend_read<Color>(it);
		    
		    XMVECTOR pos = v2_f32(x, y).getDX();
		    pos = XMVector3Transform(pos, state.current_matrix);
		    x = XMVectorGetX(pos);
		    y = XMVectorGetY(pos);

		    XMVECTOR rot, scale;
		    
		    XMMatrixDecompose(&scale, &rot, &pos, state.current_matrix);
		    
		    max_line_width *= XMVectorGetX(scale);
		    font_size *= XMVectorGetY(scale);

		    graphics_renderpass_end(cmd);
		    draw_text_area(text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, line_offset, bottom_top, pfont, color, cmd);
		    graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
		    
		}break;

		case ImRendDrawCall_Text:
		{
		    const char* text = (const char*)it;
		    it += strlen(text) + 1u;

		    size_t text_size = imrend_read<size_t>(it);
		    f32 x = imrend_read<f32>(it);
		    f32 y = imrend_read<f32>(it);
		    f32 max_line_width = imrend_read<f32>(it);
		    u32 max_lines = imrend_read<u32>(it);
		    f32 font_size = imrend_read<f32>(it);
		    f32 aspect = imrend_read<f32>(it);
		    TextAlignment alignment = imrend_read<TextAlignment>(it);
		    Font* pfont = imrend_read<Font*>(it);
		    Color color = imrend_read<Color>(it);
		    
		    XMVECTOR pos = v2_f32(x, y).getDX();
		    pos = XMVector3Transform(pos, state.current_matrix);
		    x = XMVectorGetX(pos);
		    y = XMVectorGetY(pos);

		    XMVECTOR rot, scale;
		    
		    XMMatrixDecompose(&scale, &rot, &pos, state.current_matrix);
		    
		    max_line_width *= XMVectorGetX(scale);
		    font_size *= XMVectorGetY(scale);

		    graphics_renderpass_end(cmd);
		    draw_text(text, text_size, x, y, max_line_width, max_lines, font_size, aspect, alignment, pfont, color, cmd);
		    graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
		    
		}break;
		    
		}
	    }
	    break;
		
		
		
	    }
	}

	SV_ASSERT(state.matrix_stack.empty());
	SV_ASSERT(state.scissor_stack.empty());

	graphics_renderpass_end(cmd);

	graphics_event_end(cmd);
    }

    void imrend_push_matrix(const XMMATRIX& matrix, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_PushMatrix);
	imrend_write(state, matrix);
    }

    void imrend_pop_matrix(CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_PopMatrix);
    }

    void imrend_push_scissor(f32 x, f32 y, f32 width, f32 height, bool additive, CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_PushScissor);
	imrend_write(state, v4_f32(x, y, width, height));
	imrend_write(state, additive);
    }
    
    void imrend_pop_scissor(CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_PopScissor);
    }

    void imrend_camera(ImRendCamera camera, CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_Camera);
	imrend_write(state, camera);
    }

    void imrend_draw_quad(const v3_f32& position, const v2_f32& size, Color color, CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_Quad);

	imrend_write(state, position);
	imrend_write(state, size);
	imrend_write(state, color);
    }

    void imrend_draw_line(const v3_f32& p0, const v3_f32& p1, Color color, CommandList cmd)
    {
	SV_IMREND();
	
	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_Line);

	imrend_write(state, p0);
	imrend_write(state, p1);
	imrend_write(state, color);
    }

    void imrend_draw_triangle(const v3_f32& p0, const v3_f32& p1, const v3_f32& p2, Color color, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_Triangle);
	
	imrend_write(state, p0);
	imrend_write(state, p1);
	imrend_write(state, p2);
	imrend_write(state, color);
    }

    void imrend_draw_sprite(const v3_f32& position, const v2_f32& size, Color color, GPUImage* image, GPUImageLayout layout, const v4_f32& texcoord, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_Sprite);

	imrend_write(state, position);
	imrend_write(state, size);
	imrend_write(state, color);
	imrend_write(state, image);
	imrend_write(state, texcoord);
	// TODO: Image layout
    }

    void imrend_draw_mesh_wireframe(Mesh* mesh, Color color, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_MeshWireframe);
	
	imrend_write(state, mesh);
	imrend_write(state, color);
    }

    void imrend_draw_text(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, Font* pfont, Color color, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_Text);

	state.buffer.write_back(text, text_size);
	imrend_write(state, '\0');

	imrend_write(state, text_size);
	imrend_write(state, x);
	imrend_write(state, y);
	imrend_write(state, max_line_width);
	imrend_write(state, max_lines);
	imrend_write(state, font_size);
	imrend_write(state, aspect);
	imrend_write(state, alignment);
	imrend_write(state, pfont);
	imrend_write(state, color);
    }

    void imrend_draw_text_area(const char* text, size_t text_size, f32 x, f32 y, f32 max_line_width, u32 max_lines, f32 font_size, f32 aspect, TextAlignment alignment, u32 line_offset, bool bottom_top, Font* pfont, Color color, CommandList cmd)
    {
	SV_IMREND();

	imrend_write(state, ImRendHeader_DrawCall);
	imrend_write(state, ImRendDrawCall_TextArea);

	state.buffer.write_back(text, text_size);
	imrend_write(state, '\0');

	imrend_write(state, text_size);
	imrend_write(state, x);
	imrend_write(state, y);
	imrend_write(state, max_line_width);
	imrend_write(state, max_lines);
	imrend_write(state, font_size);
	imrend_write(state, aspect);
	imrend_write(state, alignment);
	imrend_write(state, line_offset);
	imrend_write(state, bottom_top);
	imrend_write(state, pfont);
	imrend_write(state, color);
    }
    
    void imrend_draw_orthographic_grip(const v2_f32& position, const v2_f32& offset, const v2_f32& size, const v2_f32& gridSize, Color color, CommandList cmd)
    {
	v2_f32 begin = position - offset - size / 2.f;
	v2_f32 end = begin + size;

	for (f32 y = i32(begin.y / gridSize.y) * gridSize.y; y < end.y; y += gridSize.y) {
	    imrend_draw_line({ begin.x + offset.x, y + offset.y, 0.f }, { end.x + offset.x, y + offset.y, 0.f }, color, cmd);
	}
	for (f32 x = i32(begin.x / gridSize.x) * gridSize.x; x < end.x; x += gridSize.x) {
	    imrend_draw_line({ x + offset.x, begin.y + offset.y, 0.f }, { x + offset.x, end.y + offset.y, 0.f }, color, cmd);
	}
    }
}
