#include "core.h"

#include "renderer/renderer_internal.h"

namespace sv {

	// Common Buffers

	static GPUBuffer		g_VertexBuffer;

	// Default PP

	static Shader			g_DefVertexShader;
	static Shader			g_DefPixelShader;
	static InputLayoutState g_DefInputLayoutState;
	static Sampler			g_DefSampler;
	static GraphicsPipeline	g_DefPipeline;

	bool renderer_postprocessing_initialize(const InitializationRendererDesc& desc)
	{
		// Create Common Buffers
		{
			vec2 vertices[] = {
				{-1.f, -1.f},
				{ 1.f, -1.f},
				{-1.f,  1.f},
				{ 1.f,  1.f},
			};

			GPUBufferDesc desc;
			desc.bufferType = GPUBufferType_Vertex;
			desc.CPUAccess = CPUAccess_None;
			desc.pData = vertices;
			desc.size = 4 * sizeof(vec2);
			desc.usage = ResourceUsage_Static;

			svCheck(graphics_buffer_create(&desc, g_VertexBuffer));
		}

		svCheck(renderer_shader_create("DefPostProcess", ShaderType_Vertex, g_DefVertexShader));
		svCheck(renderer_shader_create("DefPostProcess", ShaderType_Pixel, g_DefPixelShader));

		// Create Default PP
		{
			SamplerDesc samDesc;
			samDesc.addressModeU = SamplerAddressMode_Mirror;
			samDesc.addressModeV = SamplerAddressMode_Mirror;
			samDesc.addressModeW = SamplerAddressMode_Mirror;
			samDesc.magFilter = SamplerFilter_Linear;
			samDesc.minFilter = SamplerFilter_Linear;

			svCheck(graphics_sampler_create(&samDesc, g_DefSampler));

			InputLayoutStateDesc inputLayout;
			inputLayout.elements.push_back({ "Position", 0u, 0u, 0u, Format_R32G32_FLOAT });
			inputLayout.slots.push_back({ 0u, sizeof(vec2), false });

			svCheck(graphics_inputlayoutstate_create(&inputLayout, g_DefInputLayoutState));

			g_DefPipeline.pVertexShader = &g_DefVertexShader;
			g_DefPipeline.pPixelShader = &g_DefPixelShader;
			g_DefPipeline.pGeometryShader = nullptr;
			g_DefPipeline.pInputLayoutState = &g_DefInputLayoutState;
			g_DefPipeline.pBlendState = nullptr;
			g_DefPipeline.pRasterizerState = nullptr;
			g_DefPipeline.pDepthStencilState = nullptr;
			g_DefPipeline.topology = GraphicsTopology_TriangleStrip;
		}

		return true;
	}

	bool renderer_postprocessing_close()
	{
		g_DefPipeline = {};
		svCheck(graphics_destroy(g_VertexBuffer));
		svCheck(graphics_destroy(g_DefVertexShader));
		svCheck(graphics_destroy(g_DefPixelShader));
		svCheck(graphics_destroy(g_DefInputLayoutState));
		return true;
	}

	// DEFAULT POSTPROCESSING

	bool renderer_postprocessing_default_create(Format dstFormat, GPUImageLayout initialLayout, GPUImageLayout finalLayout, PostProcessing_Default& pp)
	{
		AttachmentDesc att;
		att.loadOp = AttachmentOperation_DontCare;
		att.storeOp = AttachmentOperation_Store;
		att.format = dstFormat;
		att.initialLayout = initialLayout;
		att.layout = GPUImageLayout_RenderTarget;
		att.finalLayout = finalLayout;
		att.type = AttachmentType_RenderTarget;

		RenderPassDesc desc;
		desc.attachments.emplace_back(att);

		svCheck(graphics_renderpass_create(&desc, pp.renderPass));
		return true;
	}
	bool renderer_postprocessing_default_destroy(PostProcessing_Default& pp)
	{
		svCheck(graphics_destroy(pp.renderPass));
		return true;
	}
	void renderer_postprocessing_default_draw(PostProcessing_Default& pp, GPUImage& src, GPUImage& dst, CommandList cmd)
	{
		GPUImage* att[] = {
			&dst
		};
		graphics_renderpass_begin(pp.renderPass, att, nullptr, 1.f, 0u, cmd);

		Viewport viewport = graphics_image_get_viewport(dst);
		Scissor scissor = graphics_image_get_scissor(dst);
		graphics_set_viewports(&viewport, 1u, cmd);
		graphics_set_scissors(&scissor, 1u, cmd);

		graphics_pipeline_bind(g_DefPipeline, cmd);

		GPUBuffer* vBuffers[] = {
			&g_VertexBuffer
		};
		ui32 offset = 0u;
		ui32 stride = 0u;
		graphics_vertexbuffer_bind(vBuffers, &offset, &stride, 1u, cmd);

		Sampler* samplers[] = {
			&g_DefSampler
		};
		graphics_sampler_bind(samplers, 1u, ShaderType_Pixel, cmd);

		GPUImage* images[] = {
			&src
		};
		graphics_image_bind(images, 1u, ShaderType_Pixel, cmd);

		graphics_draw(4, 1u, 0u, 0u, cmd);

		graphics_renderpass_end(cmd);
	}

}