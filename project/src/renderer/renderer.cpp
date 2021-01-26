#include "SilverEngine/core.h"

#include "renderer/renderer_internal.h"

#include "SilverEngine/mesh.h"

namespace sv {

	GraphicsObjects		gfx = {};
	RenderingUtils		rend_utils[GraphicsLimit_CommandList] = {};
	RenderingContext	render_context[GraphicsLimit_CommandList] = {};

	// SHADER COMPILATION

#define COMPILE_SHADER(type, shader, path, alwais_compile) svCheck(graphics_shader_compile_fastbin_from_file(path, type, &shader, SV_SYS("system/shaders/") path, alwais_compile));
#define COMPILE_VS(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, false)
#define COMPILE_PS(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, false)

#define COMPILE_VS_(shader, path) COMPILE_SHADER(ShaderType_Vertex, shader, path, true)
#define COMPILE_PS_(shader, path) COMPILE_SHADER(ShaderType_Pixel, shader, path, true)

	static Result compile_shaders()
	{
		COMPILE_VS(gfx.vs_debug_quad, "debug/quad.hlsl");
		COMPILE_PS(gfx.ps_debug_quad, "debug/quad.hlsl");
		COMPILE_VS(gfx.vs_debug_ellipse, "debug/ellipse.hlsl");
		COMPILE_PS(gfx.ps_debug_ellipse, "debug/ellipse.hlsl");
		COMPILE_VS(gfx.vs_debug_sprite, "debug/sprite.hlsl");
		COMPILE_PS(gfx.ps_debug_sprite, "debug/sprite.hlsl");

		COMPILE_VS(gfx.vs_text, "text.hlsl");
		COMPILE_PS(gfx.ps_text, "text.hlsl");

		COMPILE_VS(gfx.vs_sprite, "sprite/default.hlsl");
		COMPILE_PS(gfx.ps_sprite, "sprite/default.hlsl");

		COMPILE_VS_(gfx.vs_mesh, "mesh/default.hlsl");
		COMPILE_PS_(gfx.ps_mesh, "mesh/default.hlsl");

		return Result_Success;
	}

	// RENDERPASSES CREATION

#define CREATE_RENDERPASS(name, renderpass) svCheck(graphics_renderpass_create(&desc, &renderpass));

	static Result create_renderpasses()
	{
		RenderPassDesc desc;
		AttachmentDesc att[GraphicsLimit_Attachment];
		desc.pAttachments = att;

		// Debug
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
		CREATE_RENDERPASS("DebugRenderpass", gfx.renderpass_debug);

		// Text
		att[0].loadOp = AttachmentOperation_Load;
		att[0].storeOp = AttachmentOperation_Store;
		att[0].format = OFFSCREEN_FORMAT;
		att[0].initialLayout = GPUImageLayout_RenderTarget;
		att[0].layout = GPUImageLayout_RenderTarget;
		att[0].finalLayout = GPUImageLayout_RenderTarget;
		att[0].type = AttachmentType_RenderTarget;

		desc.attachmentCount = 1u;
		CREATE_RENDERPASS("TextRenderpass", gfx.renderpass_text);

		// Sprite
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
		CREATE_RENDERPASS("SpriteRenderPass", gfx.renderpass_sprite);

		// Mesh
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
		CREATE_RENDERPASS("MeshRenderPass", gfx.renderpass_mesh);

		return Result_Success;
	}

	// BUFFER CREATION

	static Result create_buffers()
	{
		GPUBufferDesc desc;

		// set text index data
		{
			constexpr u32 index_count = TEXT_BATCH_COUNT * 6u;
			u16* index_data = new u16[index_count];

			for (u32 i = 0u; i < TEXT_BATCH_COUNT; ++i) {

				u32 currenti = i * 6u;
				u32 currentv = i * 4u;

				index_data[currenti + 0u] = 0 + currentv;
				index_data[currenti + 1u] = 1 + currentv;
				index_data[currenti + 2u] = 2 + currentv;

				index_data[currenti + 3u] = 1 + currentv;
				index_data[currenti + 4u] = 3 + currentv;
				index_data[currenti + 5u] = 2 + currentv;
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

		return Result_Success;
	}

	// IMAGE CREATION

	static Result create_images()
	{
		GPUImageDesc desc;

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
			slots[0].stride = sizeof(DebugVertex);

			elements[0] = { "Position", 0u, 0u, 0u, Format_R32G32B32A32_FLOAT };
			elements[1] = { "TexCoord", 0u, 0u, 4u * sizeof(float), Format_R32G32_FLOAT };
			elements[2] = { "Stroke", 0u, 0u, 6u * sizeof(float), Format_R32_FLOAT };
			elements[3] = { "Color", 0u, 0u, 7u * sizeof(float), Format_R8G8B8A8_UNORM };

			desc.elementCount = 4u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_debug));

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

			desc.elementCount = 2u;
			desc.slotCount = 1u;
			svCheck(graphics_inputlayoutstate_create(&desc, &gfx.ils_mesh));
		}

		// Create BlendStates
		{
			BlendAttachmentDesc att[GraphicsLimit_AttachmentRT];
			BlendStateDesc desc;
			desc.pAttachments = att;

			// DEBUG
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
			svCheck(graphics_blendstate_create(&desc, &gfx.bs_debug));

			// SPRITE
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

			svCheck(graphics_blendstate_create(&desc, &gfx.bs_sprite));

			// MESH
			att[0].blendEnabled = false;
			att[0].colorWriteMask = ColorComponent_All;

			desc.attachmentCount = 1u;

			svCheck(graphics_blendstate_create(&desc, &gfx.bs_mesh));
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
		}
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
		
		return Result_Success;
	}

	Result renderer_initialize()
	{
		svCheck(compile_shaders());
		svCheck(create_renderpasses());
		svCheck(create_samplers());
		svCheck(create_buffers());
		svCheck(create_images());
		svCheck(create_states());

		// Allocate batch memory
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {
				rend_utils[i].batch_data = new u8[MAX_BATCH_DATA];
			}
		}

		return Result_Success;
	}

	Result renderer_close()
	{
		// Free graphics objects
		// TODO
		svCheck(graphics_destroy_struct(&gfx, sizeof(gfx)));

		// Deallocte batch memory
		{
			for (u32 i = 0u; i < GraphicsLimit_CommandList; ++i) {
				delete[] rend_utils[i].batch_data;
				rend_utils[i].batch_data = nullptr;
			}
		}

		return Result_Success;
	}

	void renderer_begin_frame()
	{
		// Restart context
		foreach(i, GraphicsLimit_CommandList) {
			
			RenderingContext& ctx = render_context[i];
			ctx.offscreen = nullptr;
			ctx.zbuffer = nullptr;
			ctx.camera_buffer = nullptr;
		}
	}

}