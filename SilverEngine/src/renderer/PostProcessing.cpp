#include "core.h"

#include "PostProcessing.h"

namespace SV {

	// Common Buffers

	SV::Buffer				g_VertexBuffer;

	// Default PP

	SV::Shader				g_DefVertexShader;
	SV::Shader				g_DefPixelShader;
	SV::Sampler				g_DefSampler;
	SV::GraphicsPipeline	g_DefPipeline;

	namespace Renderer {
		namespace _internal {
			bool InitializePostProcessing()
			{
				// Create Common Buffers
				{
					SV::vec2 vertices[] = {
						{-1.f, -1.f},
						{ 1.f, -1.f},
						{-1.f,  1.f},
						{ 1.f,  1.f},
					};

					SV_GFX_BUFFER_DESC desc;
					desc.bufferType = SV_GFX_BUFFER_TYPE_VERTEX;
					desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
					desc.pData = vertices;
					desc.size = 4 * sizeof(SV::vec2);
					desc.usage = SV_GFX_USAGE_STATIC;

					svCheck(Graphics::CreateBuffer(&desc, g_VertexBuffer));
				}

				// Create Default PP
				{
					SV_GFX_SHADER_DESC shaderDesc;
					shaderDesc.filePath = "shaders/DefPostProcessVertex.shader";
					shaderDesc.shaderType = SV_GFX_SHADER_TYPE_VERTEX;

					svCheck(Graphics::CreateShader(&shaderDesc, g_DefVertexShader));

					shaderDesc.filePath = "shaders/DefPostProcessPixel.shader";
					shaderDesc.shaderType = SV_GFX_SHADER_TYPE_PIXEL;

					svCheck(Graphics::CreateShader(&shaderDesc, g_DefPixelShader));

					SV_GFX_SAMPLER_DESC samDesc;
					samDesc.addressModeU = SV_GFX_ADDRESS_MODE_WRAP;
					samDesc.addressModeV = SV_GFX_ADDRESS_MODE_WRAP;
					samDesc.addressModeW = SV_GFX_ADDRESS_MODE_WRAP;
					samDesc.magFilter = SV_GFX_FILTER_LINEAR;
					samDesc.minFilter = SV_GFX_FILTER_LINEAR;

					svCheck(Graphics::CreateSampler(&samDesc, g_DefSampler));

					SV_GFX_INPUT_LAYOUT_DESC inputLayout;
					inputLayout.elements.push_back({ 0u, "Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT });
					inputLayout.slots.push_back({ 0u, sizeof(SV::vec2), false });

					SV_GFX_GRAPHICS_PIPELINE_DESC pDesc;
					pDesc.pVertexShader = &g_DefVertexShader;
					pDesc.pPixelShader = &g_DefPixelShader;
					pDesc.pGeometryShader = nullptr;
					pDesc.pInputLayout = &inputLayout;
					pDesc.pBlendState = nullptr;
					pDesc.pRasterizerState = nullptr;
					pDesc.pDepthStencilState = nullptr;
					pDesc.topology = SV_GFX_TOPOLOGY_TRIANGLE_STRIP;

					svCheck(Graphics::CreateGraphicsPipeline(&pDesc, g_DefPipeline));
					
					return true;
				}
			}

			bool ClosePostProcessing()
			{
				svCheck(Graphics::Destroy(g_VertexBuffer));
				svCheck(Graphics::Destroy(g_DefVertexShader));
				svCheck(Graphics::Destroy(g_DefPixelShader));
				svCheck(Graphics::Destroy(g_DefPipeline));
				return true;
			}
		}
	}

	// DEFAULT POST PROCESSING
	bool DefaultPostProcessing::Create(SV_GFX_FORMAT dstFormat, SV_GFX_IMAGE_LAYOUT initialLayout, SV_GFX_IMAGE_LAYOUT finalLayout)
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

		svCheck(Graphics::CreateRenderPass(&desc, m_RenderPass));
		return true;
	}
	bool DefaultPostProcessing::Destroy()
	{
		svCheck(Graphics::Destroy(m_RenderPass));
		return true;
	}
	void DefaultPostProcessing::PostProcess(SV::Image& src, SV::Image& dst, CommandList cmd)
	{
		SV_GFX_VIEWPORT viewport = dst->GetViewport();
		SV_GFX_SCISSOR scissor = dst->GetScissor();
		Graphics::SetViewports(&viewport, 1u, cmd);
		Graphics::SetScissors(&scissor, 1u, cmd);

		Graphics::BindGraphicsPipeline(g_DefPipeline, cmd);
		SV::Image* att[] = {
			&dst
		};
		Graphics::BindRenderPass(m_RenderPass, att, cmd);

		SV::Buffer* vBuffers[] = {
			&g_VertexBuffer
		};
		ui32 offset = 0u;
		ui32 stride = 0u;
		Graphics::BindVertexBuffers(vBuffers, &offset, &stride, 1u, cmd);

		SV::Sampler* samplers [] = {
			&g_DefSampler
		};
		Graphics::BindSamplers(samplers, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);

		SV::Image* images[] = {
			&src
		};
		Graphics::BindImages(images, 1u, SV_GFX_SHADER_TYPE_PIXEL, cmd);
		
		Graphics::Draw(4, 1u, 0u, 0u, cmd);
	}

}