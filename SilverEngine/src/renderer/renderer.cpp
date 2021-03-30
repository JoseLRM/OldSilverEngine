#include "core.h"

#include "renderer/renderer_internal.h"
#include "mesh.h"
#include "dev.h"

namespace sv {

    GraphicsObjects		gfx = {};
    u8*					batch_data[GraphicsLimit_CommandList] = {};
    Font				font_opensans;
    Font				font_console;

    // SHADER COMPILATION

#define COMPILE_SHADER(type, shader, path, alwais_compile) svCheck(graphics_shader_compile_fastbin_from_file(path, type, &shader, "system/shaders/" path, alwais_compile));
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)

    static Result compile_shaders()
    {
	COMPILE_VS(gfx.vs_debug_solid_batch, "debug/solid_batch.hlsl");
	COMPILE_PS(gfx.ps_debug_solid, "debug/solid_batch.hlsl");
	COMPILE_VS(gfx.vs_debug_mesh_wireframe, "debug/mesh_wireframe.hlsl");

	COMPILE_VS(gfx.vs_text, "text.hlsl");
	COMPILE_PS(gfx.ps_text, "text.hlsl");

	COMPILE_VS(gfx.vs_sprite, "sprite/default.hlsl");
	COMPILE_PS(gfx.ps_sprite, "sprite/default.hlsl");

	COMPILE_VS(gfx.vs_mesh, "mesh/default.hlsl");
	COMPILE_PS(gfx.ps_mesh, "mesh/default.hlsl");

	COMPILE_VS(gfx.vs_sky, "skymapping.hlsl");
	COMPILE_PS(gfx.ps_sky, "skymapping.hlsl");

	COMPILE_VS(gfx.vs_default_postprocess, "postprocessing/default.hlsl");
	COMPILE_PS(gfx.ps_default_postprocess, "postprocessing/default.hlsl");
	COMPILE_PS(gfx.ps_gaussian_blur, "postprocessing/gaussian_blur.hlsl");
	COMPILE_PS(gfx.ps_bloom_threshold, "postprocessing/bloom_threshold.hlsl");

	return Result_Success;
    }

    // RENDERPASSES CREATION

#define CREATE_RENDERPASS(name, renderpass) svCheck(graphics_renderpass_create(&desc, &renderpass));

    static Result create_renderpasses()
    {
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
	att[1].format = ZBUFFER_FORMAT;
	att[1].initialLayout = GPUImageLayout_DepthStencil;
	att[1].layout = GPUImageLayout_DepthStencil;
	att[1].finalLayout = GPUImageLayout_DepthStencil;
	att[1].type = AttachmentType_DepthStencil;

	desc.attachmentCount = 2u;
	CREATE_RENDERPASS("WorldRenderPass", gfx.renderpass_world);

	return Result_Success;
    }

    // BUFFER CREATION

    static Result create_buffers()
    {
	GPUBufferDesc desc;

	// Environment
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(EnvironmentData);
	    desc.pData = nullptr;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_environment));
	}
		
	// Debug mesh
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(XMMATRIX) + sizeof(Color4f);
	    desc.pData = nullptr;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_debug_mesh));
	}

	// Gaussian blur
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(GaussianBlurData);
	    desc.pData = nullptr;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_gaussian_blur));
	}

	// Bloom threshold
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(v4_f32);
	    desc.pData = nullptr;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_bloom_threshold));
	}

	// Camera buffer
	{
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;
	    desc.size = sizeof(CameraBuffer_GPU);
	    desc.pData = nullptr;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_camera));
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

	    svCheck(graphics_buffer_create(&desc, &gfx.ibuffer_text));
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

	    svCheck(graphics_buffer_create(&desc, &gfx.ibuffer_sprite));
	    delete[] index_data;

	    graphics_name_set(gfx.ibuffer_sprite, "Sprite_IndexBuffer");
	}

	// Mesh
	{
	    desc.pData = nullptr;
	    desc.bufferType = GPUBufferType_Constant;
	    desc.usage = ResourceUsage_Dynamic;
	    desc.CPUAccess = CPUAccess_Write;

	    desc.size = sizeof(MeshData);
	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_mesh_instance));

	    desc.size = sizeof(Material);
	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_material));

	    desc.usage = ResourceUsage_Default;
	    desc.size = sizeof(LightData) * LIGHT_COUNT;
	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_light_instances));
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

	    svCheck(graphics_buffer_create(&desc, &gfx.vbuffer_skybox));

	    desc.pData = nullptr;
	    desc.bufferType = GPUBufferType_Constant;
	    desc.size = sizeof(XMMATRIX);
	    desc.usage = ResourceUsage_Default;
	    desc.CPUAccess = CPUAccess_Write;

	    svCheck(graphics_buffer_create(&desc, &gfx.cbuffer_skybox));
	}

	return Result_Success;
    }

    // IMAGE CREATION

    static Result create_images()
    {
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
	    svCheck(graphics_image_create(&desc, &gfx.offscreen));

	    desc.format = ZBUFFER_FORMAT;
	    desc.layout = GPUImageLayout_DepthStencil;
	    desc.type = GPUImageType_DepthStencil;
	    svCheck(graphics_image_create(&desc, &gfx.depthstencil));
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

	    svCheck(graphics_image_create(&desc, &gfx.image_aux0));
	    svCheck(graphics_image_create(&desc, &gfx.image_aux1));
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

	    svCheck(graphics_image_create(&desc, &gfx.image_white));
	}
		
	return Result_Success;
    }

    // STATE CREATION

    static Result create_states()
    {
	// Create InputlayoutStates
	{
	    InputSlotDesc slots[GraphicsLimit_InputSlot];
	    InputElementDesc elements[GraphicsLimit_InputElement];
	    InputLayoutStateDesc desc;
	    desc.pSlots = slots;
	    desc.pElements = elements;

	    // DEBUG
	    slots[0].instanced = false;
	    slots[0].slot = 0u;
	    slots[0].stride = sizeof(DebugVertex_Solid);

	    elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
	    elements[1] = { "Color", 0u, 0u, 4u * sizeof(f32), Format_R8G8B8A8_UNORM };

	    desc.elementCount = 2u;
	    desc.slotCount = 1u;
	    svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_debug_solid_batch));

	    // TEXT
	    slots[0] = { 0u, sizeof(TextVertex), false };

	    elements[0] = { "Position", 0u, 0u, sizeof(f32) * 0u, Format_R32G32B32A32_FLOAT };
	    elements[1] = { "TexCoord", 0u, 0u, sizeof(f32) * 4u, Format_R32G32_FLOAT };
	    elements[2] = { "Color", 0u, 0u, sizeof(f32) * 6u, Format_R8G8B8A8_UNORM };

	    desc.elementCount = 3u;
	    desc.slotCount = 1u;
	    svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_text));

	    // SPRITE
	    slots[0] = { 0u, sizeof(SpriteVertex), false };

	    elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
	    elements[1] = { "TexCoord", 0u, 0u, 4u * sizeof(f32), Format_R32G32_FLOAT };
	    elements[2] = { "Color", 0u, 0u, 6u * sizeof(f32), Format_R8G8B8A8_UNORM };

	    desc.elementCount = 3u;
	    desc.slotCount = 1u;
	    svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_sprite));

	    // MESH
	    slots[0] = { 0u, sizeof(MeshVertex), false };

	    elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };
	    elements[1] = { "Normal", 0u, 0u, 3u * sizeof(f32), Format_R32G32B32_FLOAT };
	    elements[2] = { "Tangent", 0u, 0u, 6u * sizeof(f32), Format_R32G32B32_FLOAT };
	    elements[3] = { "Bitangent", 0u, 0u, 9u * sizeof(f32), Format_R32G32B32_FLOAT };
	    elements[4] = { "Texcoord", 0u, 0u, 12u * sizeof(f32), Format_R32G32_FLOAT };

	    desc.elementCount = 5u;
	    desc.slotCount = 1u;
	    svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_mesh));

	    // SKY BOX
	    slots[0] = { 0u, sizeof(v3_f32), false };

	    elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32_FLOAT };

	    desc.elementCount = 1u;
	    desc.slotCount = 1u;
	    svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_sky));
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
	    svCheck(graphics_blendstate_create(&desc, &gfx.bs_transparent));

	    // MESH
	    att[0].blendEnabled = false;
	    att[0].colorWriteMask = ColorComponent_All;

	    desc.attachmentCount = 1u;

	    svCheck(graphics_blendstate_create(&desc, &gfx.bs_mesh));

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
	    svCheck(graphics_blendstate_create(&desc, &gfx.bs_addition));
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

	return Result_Success;
    }

    // SAMPLER CREATION

    static Result create_samplers()
    {
	SamplerDesc desc;

	desc.addressModeU = SamplerAddressMode_Wrap;
	desc.addressModeV = SamplerAddressMode_Wrap;
	desc.addressModeW = SamplerAddressMode_Wrap;
	desc.minFilter = SamplerFilter_Linear;
	desc.magFilter = SamplerFilter_Linear;
	svCheck(graphics_sampler_create(&desc, &gfx.sampler_def_linear));

	desc.addressModeU = SamplerAddressMode_Wrap;
	desc.addressModeV = SamplerAddressMode_Wrap;
	desc.addressModeW = SamplerAddressMode_Wrap;
	desc.minFilter = SamplerFilter_Nearest;
	desc.magFilter = SamplerFilter_Nearest;
	svCheck(graphics_sampler_create(&desc, &gfx.sampler_def_nearest));

	desc.addressModeU = SamplerAddressMode_Mirror;
	desc.addressModeV = SamplerAddressMode_Mirror;
	desc.addressModeW = SamplerAddressMode_Mirror;
	desc.minFilter = SamplerFilter_Linear;
	desc.magFilter = SamplerFilter_Linear;
	svCheck(graphics_sampler_create(&desc, &gfx.sampler_blur));

	return Result_Success;
    }

    // RENDERER RUNTIME

    Result renderer_initialize()
    {
	svCheck(compile_shaders());
	svCheck(create_renderpasses());
	svCheck(create_samplers());
	svCheck(create_buffers());
	svCheck(create_images());
	svCheck(create_states());

	// Create default fonts
	svCheck(font_create(font_opensans, "system/fonts/OpenSans/OpenSans-Regular.ttf", 128.f, 0u));
	svCheck(font_create(font_console, "system/fonts/Cousine/Cousine-Regular.ttf", 128.f, 0u));

	return Result_Success;
    }

    Result renderer_close()
    {
	// Free graphics objects
	graphics_destroy_struct(&gfx, sizeof(gfx));

	// Deallocte batch memory
	{
	    for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {

		if (batch_data[i]) {
		    free(batch_data[i]);
		    batch_data[i] = nullptr;
		}
	    }
	}

	font_destroy(font_opensans);
	font_destroy(font_console);

	return Result_Success;
    }

    void renderer_begin()
    {
	CommandList cmd = graphics_commandlist_begin();

	graphics_image_clear(gfx.offscreen, GPUImageLayout_RenderTarget, GPUImageLayout_RenderTarget, Color4f::Black(), 1.f, 0u, cmd);
	graphics_image_clear(gfx.depthstencil, GPUImageLayout_DepthStencil, GPUImageLayout_DepthStencil, Color4f::Black(), 1.f, 0u, cmd);
	
	graphics_viewport_set(gfx.offscreen, 0u, cmd);
	graphics_scissor_set(gfx.offscreen, 0u, cmd);
    }

    void renderer_end()
    {
	graphics_present_image(gfx.offscreen, GPUImageLayout_RenderTarget);
    }

    Result load_skymap_image(const char* filepath, GPUImage** pimage)
    {
	Color* data;
	u32 w, h;
	svCheck(load_image(filepath, (void**)& data, &w, &h));


	u32 image_width = w / 4u;
	u32 image_height = h / 3u;

	Color* images[6u] = {};
	Color* mem = (Color*)malloc(image_width * image_height * 4u * 6u);

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

	Result res = graphics_image_create(&desc, pimage);

	free(mem);
	delete[] data;

	return res;
    }

    struct SpriteInstance {
	XMMATRIX	tm;
	v4_f32		texcoord;
	GPUImage* image;
	Color		color;
	u32			layer;

	SpriteInstance() = default;
	SpriteInstance(const XMMATRIX& m, const v4_f32& texcoord, GPUImage* image, Color color, u32 layer)
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

	Color3f		color;
	LightType	type;
	v3_f32		position;
	f32			range;
	f32			intensity;
	f32			smoothness;

	// None
	LightInstance() = default;

	// Point
	LightInstance(const Color3f& color, const v3_f32& position, f32 range, f32 intensity, f32 smoothness)
	    : color(color), type(LightType_Point), position(position), range(range), intensity(intensity), smoothness(smoothness) {}

	// Direction
	LightInstance(const Color3f& color, const v3_f32& direction, f32 intensity)
	    : color(color), type(LightType_Direction), position(direction), intensity(intensity) {}
    };

    // Temp data
    static List<SpriteInstance> sprite_instances;
    static List<MeshInstance> mesh_instances;
    static List<LightInstance> light_instances;

    void draw_scene(Scene* scene)
    {
	mesh_instances.reset();
	light_instances.reset();
	sprite_instances.reset();

	CommandList cmd = graphics_commandlist_get();

	// Create environment buffer
	{
	    EnvironmentData data;
	    data.ambient_light = Color3f::Gray(0.1f);
	    graphics_buffer_update(gfx.cbuffer_environment, &data, sizeof(EnvironmentData), 0u, cmd);
	}

	// Get lights
	{
	    EntityView<LightComponent> lights(scene);

	    for (const ComponentView<LightComponent>& v : lights) {

		Entity entity = v.entity;
		LightComponent& l = *v.comp;

		Transform trans = get_entity_transform(scene, entity);

		switch (l.light_type)
		{
		case LightType_Point:
		    light_instances.emplace_back(l.color, trans.getWorldPosition(), l.range, l.intensity, l.smoothness);
		    break;

		case LightType_Direction:
		{
		    XMVECTOR direction = XMVectorSet(0.f, 0.f, 1.f, 1.f);

		    direction = XMVector3Transform(direction, XMMatrixRotationQuaternion(trans.getWorldRotation().get_dx()));

		    light_instances.emplace_back(l.color, v3_f32(direction), l.intensity);
		}
		break;

		}

	    }
	}

	// Draw cameras
	{

#ifdef SV_DEV
	    CameraComponent* camera_ = nullptr;
	    CameraBuffer_GPU camera_data;

	    {
		v3_f32 cam_pos;
		v4_f32 cam_rot;
				
		if (dev.debug_draw) {

		    camera_ = &dev.camera;
		    cam_pos = dev.camera.position;
		    cam_rot = dev.camera.rotation;
		}
		else {
					
		    camera_ = get_main_camera(scene);
		    if (camera_) {

			Transform trans = get_entity_transform(scene, scene->main_camera);
			cam_pos = trans.getWorldPosition();
			cam_rot = trans.getWorldRotation();
		    }
		}

		if (camera_) {
						
		    camera_data.projection_matrix = camera_->projection_matrix;
		    camera_data.position = cam_pos.getVec4(0.f);
		    camera_data.rotation = cam_rot;
		    camera_data.view_matrix = camera_->view_matrix;
		    camera_data.view_projection_matrix = camera_->view_projection_matrix;
					
		    graphics_buffer_update(gfx.cbuffer_camera, &camera_data, sizeof(CameraBuffer_GPU), 0u, cmd);
		}
	    }
#else
			
	    CameraComponent* camera_ = get_main_camera(scene);
	    CameraBuffer_GPU camera_data;

	    if (camera_) {
				
		Transform camera_trans = get_entity_transform(scene, camera_->entity);
		camera_data.projection_matrix = camera_->projection_matrix;
		camera_data.position = camera_trans.getWorldPosition().getVec4(0.f);
		camera_data.rotation = camera_trans.getWorldRotation();
		camera_data.view_matrix = camera_->view_matrix;
		camera_data.view_projection_matrix = camera_->view_projection_matrix;
				
		graphics_buffer_update(gfx.cbuffer_camera, &camera_data, sizeof(CameraBuffer_GPU), 0u, cmd);
	    }

#endif
		
	    if (camera_) {

		CameraComponent& camera = *camera_;

		if (scene->skybox && camera.projection_type == ProjectionType_Perspective)
		    draw_sky(scene->skybox, camera_data.view_matrix, camera_data.projection_matrix, cmd);

		// DRAW SPRITES
		{
		    {
			EntityView<SpriteComponent> sprites(scene);

			for (ComponentView<SpriteComponent> view : sprites) {

			    SpriteComponent& spr = *view.comp;
			    Entity entity = view.entity;

			    Transform trans = get_entity_transform(scene, entity);
			    sprite_instances.emplace_back(trans.getWorldMatrix(), spr.texcoord, spr.texture.get(), spr.color, spr.layer);
			}
		    }

		    if (sprite_instances.size()) {

			// Sort sprites
			std::sort(sprite_instances.data(), sprite_instances.data() + sprite_instances.size(), [](const SpriteInstance& s0, const SpriteInstance& s1)
			    {
				return s0.layer < s1.layer;
			    });

			GPUBuffer* batch_buffer = get_batch_buffer(sizeof(SpriteData), cmd);
			SpriteData& data = *(SpriteData*)batch_data[cmd];

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
			graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);

			GPUImage* att[2];
			att[0] = gfx.offscreen;
			att[1] = gfx.depthstencil;

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
			}
		    }
		}

		// DRAW MESHES
		{
		    {
			EntityView<MeshComponent> meshes(scene);

			for (ComponentView<MeshComponent> view : meshes) {

			    MeshComponent& mesh = *view.comp;
			    Entity entity = view.entity;

			    Mesh* m = mesh.mesh.get();
			    if (m == nullptr) continue;

			    Transform trans = get_entity_transform(scene, entity);
			    mesh_instances.emplace_back(trans.getWorldMatrix(), m, mesh.material.get());
			}
		    }

		    /* TODO LIST:
		       - Undefined lights
		    */
		    if (mesh_instances.size()) {
			// Prepare state
			graphics_shader_bind(gfx.vs_mesh, cmd);
			graphics_shader_bind(gfx.ps_mesh, cmd);
			graphics_inputlayoutstate_bind(gfx.ils_mesh, cmd);
			graphics_depthstencilstate_bind(gfx.dss_default_depth, cmd);
			graphics_rasterizerstate_bind(gfx.rs_back_culling, cmd);
			graphics_blendstate_bind(gfx.bs_mesh, cmd);
			graphics_sampler_bind(gfx.sampler_def_linear, 0u, ShaderType_Pixel, cmd);

			// Bind resources
			GPUBuffer* instance_buffer = gfx.cbuffer_mesh_instance;
			GPUBuffer* material_buffer = gfx.cbuffer_material;
			GPUBuffer* light_instances_buffer = gfx.cbuffer_light_instances;

			graphics_constantbuffer_bind(instance_buffer, 0u, ShaderType_Vertex, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_camera, 1u, ShaderType_Vertex, cmd);
						
			graphics_constantbuffer_bind(material_buffer, 0u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(light_instances_buffer, 1u, ShaderType_Pixel, cmd);
			graphics_constantbuffer_bind(gfx.cbuffer_environment, 2u, ShaderType_Pixel, cmd);

			u32 light_count = SV_MIN(LIGHT_COUNT, u32(light_instances.size()));

			// Send light data
			{
			    LightData light_data[LIGHT_COUNT];

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

				LightData& l0 = light_data[i];
				const LightInstance& l1 = light_instances[i];

				l0.type = l1.type;
				l0.color = l1.color;
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

			    graphics_buffer_update(light_instances_buffer, light_data, sizeof(LightData) * LIGHT_COUNT, 0u, cmd);

			}

			// Begin renderpass
			GPUImage* att[2u] = { gfx.offscreen, gfx.depthstencil };
			graphics_renderpass_begin(gfx.renderpass_world, att, cmd);

			foreach(i, mesh_instances.size()) {

			    const MeshInstance& inst = mesh_instances[i];

			    graphics_vertexbuffer_bind(inst.mesh->vbuffer, 0u, 0u, cmd);
			    graphics_indexbuffer_bind(inst.mesh->ibuffer, 0u, cmd);

			    // Update material data
			    {
				MaterialData material_data;
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

				    material_data.diffuse_color = inst.material->diffuse_color;
				    material_data.specular_color = inst.material->specular_color;
				    material_data.emissive_color = inst.material->emissive_color;
				    material_data.shininess = inst.material->shininess;
				}
				else {

				    graphics_image_bind(gfx.image_white, 0u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 1u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 2u, ShaderType_Pixel, cmd);
				    graphics_image_bind(gfx.image_white, 3u, ShaderType_Pixel, cmd);

				    material_data.diffuse_color = Color3f::Gray(0.45f);
				    material_data.specular_color = Color3f::Gray(0.5f);
				    material_data.emissive_color = Color3f::Black();
				    material_data.shininess = 0.5f;
				}

				graphics_buffer_update(material_buffer, &material_data, sizeof(MaterialData), 0u, cmd);
			    }

			    // Update instance data
			    {
				MeshData mesh_data;
				mesh_data.model_view_matrix = inst.transform_matrix * camera_data.view_matrix;
				mesh_data.inv_model_view_matrix = XMMatrixInverse(nullptr, mesh_data.model_view_matrix);

				graphics_buffer_update(instance_buffer, &mesh_data, sizeof(MeshData), 0u, cmd);
			    }

			    graphics_draw_indexed(u32(inst.mesh->indices.size()), 1u, 0u, 0u, 0u, cmd);
			}

			graphics_renderpass_end(cmd);
		    }
		}

		// TEMP bloom

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
			nullptr,
			0.8f, 0.03f, os_window_aspect(), cmd);

		gui_draw(scene->gui, cmd);
	    }
	    else {
				
		constexpr f32 SIZE = 0.4f;
		constexpr const char* TEXT = "No Main Camera";
		draw_text(TEXT, strlen(TEXT), -1.f, +SIZE * 0.5f, 2.f, 1u, SIZE, os_window_aspect(), TextAlignment_Center, &font_opensans, cmd);
	    }
	}
    }

    void draw_sky(GPUImage* skymap, XMMATRIX view_matrix, const XMMATRIX& projection_matrix, CommandList cmd)
    {
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

	// Select font
	Font& font = pFont ? *pFont : font_opensans;
		
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

	TextData& data = *reinterpret_cast<TextData*>(batch_data[cmd]);

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

			data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.w }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos + height	, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.w }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos			, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.x, g.texCoord.y }, color };
			data.vertices[vertex_count++] = { v4_f32{ xpos + width	, ypos			, 0.f, 1.f }, v2_f32{ g.texCoord.z, g.texCoord.y }, color };
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
	Font& font = pFont ? *pFont : font_opensans;

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

    // POSTPROCESSING

    SV_INLINE static void postprocessing_draw_call(CommandList cmd)
    {
	graphics_draw(4u, 1u, 0u, 0u, cmd);
    }

    void postprocess_gaussian_blur(
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
	GPUImage* att[1u];
	GaussianBlurData data;

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
			
	    graphics_renderpass_begin(gfx.renderpass_off, att, cmd);
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
	    GPUImage* src,
	    GPUImage* dst,
	    CommandList cmd
	)
    {
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
	    GPUImage* emissive,
	    f32 threshold,
	    f32 intensity,
	    f32 aspect,
	    CommandList cmd
	)
    {
	GPUBarrier barriers[2];
	u32 barrier_count = 0u;
	GPUImage* att[1u];

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
	if (emissive) {

	    postprocess_addition(aux0, emissive, cmd);
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
			
	    postprocess_addition(aux0, src, cmd);
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
	if (barrier_count) {
	    graphics_barrier(barriers, barrier_count, cmd);
	}
    }

}
