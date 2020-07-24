#include "core.h"
#include "Engine.h"

#include "PostProcessing.h"
#include "graphics/GraphicsProperties.h"

namespace SV {
	namespace Renderer {

		SV::uvec2 g_Resolution;

		SV::Image g_Offscreen;

		SV::DefaultPostProcessing g_PP_OffscreenToBackBuffer;

		// Temp
		SV::RenderPass			g_TriangleRenderPass;
		SV::GraphicsPipeline	g_TrianglePipeline;
		SV::Shader				g_TriangleVertexShader;
		SV::Shader				g_TrianglePixelShader;
		SV::Buffer				g_TriangleVertexBuffer;
		SV::Buffer				g_TriangleIndexBuffer;
		SV::Buffer				g_TriangleConstantBuffer;
		float g_TriangleRotation = 0.f;
		struct Vertex {
			SV::vec2 position;
			SV::vec3 color;
		};
		struct Global {
			XMMATRIX matrix;
		};

		namespace _internal {
			bool Initialize(const SV_RENDERER_INITIALIZATION_DESC& desc)
			{
				// Initial Resolution
				g_Resolution = uvec2(desc.resolutionWidth, desc.resolutionHeight);

				// BackBuffer
				SV::Image& backBuffer = Graphics::GetSwapChainBackBuffer();

				// Create Offscreen
				{
					SV_GFX_IMAGE_DESC imageDesc;
					imageDesc.format = SV_GFX_FORMAT_R8G8B8A8_UNORM;
					imageDesc.layout = SV_GFX_IMAGE_LAYOUT_RENDER_TARGET;
					imageDesc.dimension = 2u;
					imageDesc.width = g_Resolution.x;
					imageDesc.height = g_Resolution.y;
					imageDesc.depth = 1u;
					imageDesc.layers = 1u;
					imageDesc.CPUAccess = 0u;
					imageDesc.usage = SV_GFX_USAGE_STATIC;
					imageDesc.pData = nullptr;
					imageDesc.type = SV_GFX_IMAGE_TYPE_RENDER_TARGET | SV_GFX_IMAGE_TYPE_SHADER_RESOURCE;

					svCheck(SV::Graphics::CreateImage(&imageDesc, g_Offscreen));
				}

				svCheck(g_PP_OffscreenToBackBuffer.Create(backBuffer->GetFormat(), SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET));

				// TEMP
				Vertex vertices[] = {
						{ { -0.5f, -0.5f }, { 0.f, 0.f, 0.f } },
						{ { -0.5f, 0.5f }, { 0.f, 0.2f, 0.9f } },
						{ { 0.5f, 0.5f }, { 1.f, 1.f, 1.f } },

						{ { 0.5f, -0.5f }, { 0.f, 0.f, 0.f } },
						{ { -0.5f, 0.5f }, { 0.f, 0.2f, 0.9f } },
						{ { 0.5f, 0.5f }, { 1.f, 1.f, 1.f } },
				};
				constexpr ui32 vertexCount = 6;

				ui32 indices[] = {
					0, 1, 2, 3, 4, 5
				};
				constexpr ui32 indexCount = 6;

				Global global;
				global.matrix = XMMatrixRotationZ(ToRadians(g_TriangleRotation));

				if (Graphics::GetProperties().transposedMatrices) {
					global.matrix = XMMatrixTranspose(global.matrix);
				}

				// Create Triangle Shaders
				{
					SV_GFX_SHADER_DESC vDesc;
					vDesc.filePath = "shaders/TriangleVertex.shader";
					vDesc.shaderType = SV_GFX_SHADER_TYPE_VERTEX;
					svCheck(Graphics::CreateShader(&vDesc, g_TriangleVertexShader));

					SV_GFX_SHADER_DESC pDesc;
					pDesc.filePath = "shaders/TrianglePixel.shader";
					pDesc.shaderType = SV_GFX_SHADER_TYPE_PIXEL;
					svCheck(Graphics::CreateShader(&pDesc, g_TrianglePixelShader));
				}
				// Create Triangle Vertex Buffer
				{
					SV_GFX_BUFFER_DESC desc;
					desc.bufferType = SV_GFX_BUFFER_TYPE_VERTEX;
					desc.CPUAccess = SV_GFX_CPU_ACCESS_WRITE;
					desc.pData = &vertices;
					desc.size = vertexCount * sizeof(Vertex);
					desc.usage = SV_GFX_USAGE_DYNAMIC;

					svCheck(Graphics::CreateBuffer(&desc, g_TriangleVertexBuffer));
				}
				// Create Triangle Index Buffer
				{
					SV_GFX_BUFFER_DESC desc;
					desc.bufferType = SV_GFX_BUFFER_TYPE_INDEX;
					desc.indexType = SV_GFX_INDEX_TYPE_32_BITS;
					desc.CPUAccess = SV_GFX_CPU_ACCESS_NONE;
					desc.pData = &indices;
					desc.size = indexCount * sizeof(ui32);
					desc.usage = SV_GFX_USAGE_STATIC;

					svCheck(Graphics::CreateBuffer(&desc, g_TriangleIndexBuffer));
				}
				// Create Triangle Constant Buffer
				{
					SV_GFX_BUFFER_DESC desc;
					desc.bufferType = SV_GFX_BUFFER_TYPE_CONSTANT;
					desc.CPUAccess = SV_GFX_CPU_ACCESS_WRITE;
					desc.pData = &global;
					desc.size = sizeof(Global);
					desc.usage = SV_GFX_USAGE_DEFAULT;

					svCheck(Graphics::CreateBuffer(&desc, g_TriangleConstantBuffer));
				}
				// Create Triangle RenderPass
				{
					SV_GFX_RENDERPASS_DESC desc;
					desc.attachments.emplace_back(Graphics::Default::RenderPassAttDesc_RenderTarget(g_Offscreen->GetFormat()));
					desc.attachments[0].finalLayout = SV_GFX_IMAGE_LAYOUT_SHADER_RESOUCE;

					svCheck(SV::Graphics::CreateRenderPass(&desc, g_TriangleRenderPass));
				}
				// Create Triangle Pipeline
				{
					SV_GFX_GRAPHICS_PIPELINE_DESC desc;

					SV_GFX_INPUT_LAYOUT_DESC inputLayout;
					inputLayout.elements.push_back({ 0u, "Color", 2u * sizeof(float), SV_GFX_FORMAT_R32G32B32_FLOAT });
					inputLayout.elements.push_back({ 0u, "Position", 0u, SV_GFX_FORMAT_R32G32_FLOAT });

					inputLayout.slots.push_back({ 0u, sizeof(Vertex), false });

					desc.pVertexShader		= &g_TriangleVertexShader;
					desc.pPixelShader		= &g_TrianglePixelShader;
					desc.pGeometryShader	= nullptr;
					desc.pInputLayout		= &inputLayout;
					desc.pBlendState		= nullptr;
					desc.pRasterizerState	= nullptr;
					desc.pDepthStencilState = nullptr;
					desc.topology			= SV_GFX_TOPOLOGY_TRIANGLES;

					svCheck(Graphics::CreateGraphicsPipeline(&desc, g_TrianglePipeline));
				}

				svCheck(Renderer::_internal::InitializePostProcessing());

				return true;
			}

			bool Close()
			{
				svCheck(g_PP_OffscreenToBackBuffer.Destroy());

				svCheck(Renderer::_internal::ClosePostProcessing());

				return true;
			}

			void BeginFrame()
			{
				SV::Graphics::_internal::BeginFrame();
			}
			void Render()
			{
				CommandList cmd = Graphics::BeginCommandList();

				Graphics::SetPipelineMode(SV_GFX_PIPELINE_MODE_GRAPHICS, cmd);

				SV::vec4 clearColor = {1.f, 1.f, 0.f, 1.f};
				Graphics::SetClearColors(&clearColor, 1u, cmd);
				
				SV_GFX_VIEWPORT viewport = g_Offscreen->GetViewport();
				Graphics::SetViewports(&viewport, 1u, cmd);

				SV_GFX_SCISSOR scissor = g_Offscreen->GetScissor();
				scissor.width = (Engine::GetMousePos().x + 0.5f) * g_Offscreen->GetWidth();
				Graphics::SetScissors(&scissor, 1u, cmd);

				// Temp
				SV::Image* attachments[] = {
					&g_Offscreen
				};

				Graphics::BindRenderPass(g_TriangleRenderPass, attachments, cmd);
				Graphics::BindGraphicsPipeline(g_TrianglePipeline, cmd);

				SV::Buffer* vertexBuffers[] = {
					&g_TriangleVertexBuffer
				};
				ui32 offsets[] = {
					0u
				};
				ui32 strides[] = {
					0u
				};
				Graphics::BindVertexBuffers(vertexBuffers, offsets, strides, 1u, cmd);
				Graphics::BindIndexBuffer(g_TriangleIndexBuffer, 0u, cmd);

				SV::Buffer* constantBuffers[] = {
					&g_TriangleConstantBuffer
				};
				Graphics::BindConstantBuffers(constantBuffers, 1u, SV_GFX_SHADER_TYPE_VERTEX, cmd);

				// Update Constant Buffer
				Global global;
				global.matrix = XMMatrixRotationZ(ToRadians(g_TriangleRotation));
				g_TriangleRotation+=SV::Engine::GetDeltaTime() * 90.f;
				Graphics::UpdateBuffer(g_TriangleConstantBuffer, &global, sizeof(Global), 0u, cmd);

				Graphics::DrawIndexed(g_TriangleIndexBuffer->GetSize() / sizeof(ui32), 1u, 0u, 0u, 0u, cmd);

				// PostProcess to BackBuffer
				SV::Image& backBuffer = Graphics::GetSwapChainBackBuffer();

				g_PP_OffscreenToBackBuffer.PostProcess(g_Offscreen, backBuffer, cmd);

			}
			void EndFrame()
			{
				SV::Graphics::_internal::SubmitCommandLists();
				SV::Graphics::_internal::Present();
			}
		}

		void ResizeBuffers()
		{

		}

		void SetResolution(ui32 width, ui32 height)
		{
			if (g_Resolution.x == width && g_Resolution.y == height) return;

			g_Resolution = SV::uvec2(width, height);

			ResizeBuffers();
		}

		uvec2 GetResolution() noexcept { return g_Resolution; }
		ui32 GetResolutionWidth() noexcept { return g_Resolution.x; }
		ui32 GetResolutionHeight() noexcept { return g_Resolution.y; }
		float GetResolutionAspect() noexcept { return float(g_Resolution.x) / float(g_Resolution.y); }

	}
}