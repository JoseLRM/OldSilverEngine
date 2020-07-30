#include "core.h"

#include "renderer.h"

using namespace sv;

namespace _sv {

	// Common Buffers

	static Buffer			g_VertexBuffer;

	// Default PP

	static Shader			g_DefVertexShader;
	static Shader			g_DefPixelShader;
	static Sampler			g_DefSampler;
	static GraphicsPipeline	g_DefPipeline;

	bool renderer_postprocessing_initialize()
	{
		// Create Common Buffers
		{
			vec2 vertices[] = {
				{-1.f, -1.f},
				{ 1.f, -1.f},
				{-1.f,  1.f},
				{ 1.f,  1.f},
			};

			SV_GFX_BUFFER_DESC desc;
			desc.bufferType = SV_GFX_BUFFER_TYPE_VERTEX;
			desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
			desc.pData = vertices;
			desc.size = 4 * sizeof(vec2);
			desc.usage = SV_GFX_USAGE_STATIC;

			svCheck(graphics_buffer_create(&desc, g_VertexBuffer));
		}

		// Create Default PP
		{
			SV_GFX_SHADER_DESC shaderDesc;
			shaderDesc.filePath = "shaders/DefPostProcessVertex.shader";
			shaderDesc.shaderType = SV_GFX_SHADER_TYPE_VERTEX;

			svCheck(graphics_shader_create(&shaderDesc, g_DefVertexShader));

			shaderDesc.filePath = "shaders/DefPostProcessPixel.shader";
			shaderDesc.shaderType = SV_GFX_SHADER_TYPE_PIXEL;

			svCheck(graphics_shader_create(&shaderDesc, g_DefPixelShader));

			SV_GFX_SAMPLER_DESC samDesc;
			samDesc.addressModeU = SV_GFX_ADDRESS_MODE_WRAP;
			samDesc.addressModeV = SV_GFX_ADDRESS_MODE_WRAP;
			samDesc.addressModeW = SV_GFX_ADDRESS_MODE_WRAP;
			samDesc.magFilter = SV_GFX_FILTER_LINEAR;
			samDesc.minFilter = SV_GFX_FILTER_LINEAR;

			svCheck(graphics_sampler_create(&samDesc, g_DefSampler));

			SV_GFX_INPUT_LAYOUT_DESC inputLayout;
			inputLayout.elements.push_back({ 0u, "Position", 0u, 0u, SV_GFX_FORMAT_R32G32_FLOAT });
			inputLayout.slots.push_back({ 0u, sizeof(vec2), false });

			SV_GFX_GRAPHICS_PIPELINE_DESC pDesc;
			pDesc.pVertexShader = &g_DefVertexShader;
			pDesc.pPixelShader = &g_DefPixelShader;
			pDesc.pGeometryShader = nullptr;
			pDesc.pInputLayout = &inputLayout;
			pDesc.pBlendState = nullptr;
			pDesc.pRasterizerState = nullptr;
			pDesc.pDepthStencilState = nullptr;
			pDesc.topology = SV_GFX_TOPOLOGY_TRIANGLE_STRIP;

			svCheck(graphics_pipeline_create(&pDesc, g_DefPipeline));

			return true;
		}
	}

	bool renderer_postprocessing_close()
	{
		svCheck(graphics_destroy(g_VertexBuffer));
		svCheck(graphics_destroy(g_DefVertexShader));
		svCheck(graphics_destroy(g_DefPixelShader));
		svCheck(graphics_destroy(g_DefPipeline));
		return true;
	}

}

namespace _sv {

	// DEFAULT POSTPROCESSING

	bool renderer_postprocessing_default_create(SV_GFX_FORMAT dstFormat, SV_GFX_IMAGE_LAYOUT initialLayout, SV_GFX_IMAGE_LAYOUT finalLayout, PostProcessing_Default& pp)
	{
		SV_GFX_ATTACHMENT_DESC att;
		att.loadOp = SV_GFX_LOAD_OP_DONT_CARE;
		att.storeOp = SV_GFX_STORE_OP_STORE;
		att.format = dstFormat;
		att.initialLayout = initialLayout;
		att.layout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
		att.finalLayout = finalLayout;
		att.type = SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET;

		SV_GFX_RENDERPASS_DESC desc;
		desc.attachments.emplace_back(att);

		svCheck(graphics_renderpass_create(&desc, pp.renderPass));
		return true;
	}
	bool renderer_postprocessing_default_destroy(PostProcessing_Default& pp)
	{
		svCheck(graphics_destroy(pp.renderPass));
		return true;
	}
	void renderer_postprocessing_default_render(PostProcessing_Default& pp, Image& src, Image& dst, CommandList cmd)
	{
		Image* att[] = {
			&dst
		};
		graphics_renderpass_begin(pp.renderPass, att, nullptr, 1.f, 0u, cmd);

		Viewport viewport = dst->GetViewport();
		Scissor scissor = dst->GetScissor();
		graphics_set_viewports(&viewport, 1u, cmd);
		graphics_set_scissors(&scissor, 1u, cmd);

		graphics_pipeline_bind(g_DefPipeline, cmd);

		Buffer* vBuffers[] = {
			&g_VertexBuffer
		};
		ui32 offset = 0u;
		ui32 stride = 0u;
		graphics_vertexbuffer_bind(vBuffers, &offset, &stride, 1u, cmd);

		Sampler* samplers[] = {
			&g_DefSampler
		};
		graphics_sampler_bind(samplers, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);

		Image* images[] = {
			&src
		};
		graphics_image_bind(images, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);

		graphics_draw(4, 1u, 0u, 0u, cmd);

		graphics_renderpass_end(cmd);
	}

}