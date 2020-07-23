#include "core.h"

#include "Graphics.h"
#include "vulkan/Graphics_vk.h"
#include "Window.h"
#include "PipelineState.h"

namespace SV {

	namespace _internal {

		/////////////////////////////////////// INTERNAL PRIMITIVES //////////////////////////////////////////////

		void RenderPass_internal::SetDescription(const SV_GFX_RENDERPASS_DESC& d) noexcept
		{
			m_DepthStencilAttachment = ui32(d.attachments.size());
			for (ui32 i = 0; i < d.attachments.size(); ++i) {
				if (d.attachments[i].type == SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL) {
					m_DepthStencilAttachment = i;
					break;
				}
			}
			m_Attachments = d.attachments;
		}

		void GraphicsPipeline_internal::SetDescription(const SV_GFX_GRAPHICS_PIPELINE_DESC& d) noexcept
		{
			m_VS = nullptr;
			m_PS = nullptr;
			m_GS = nullptr;
			if (d.pVertexShader)	m_VS = reinterpret_cast<Shader_internal*>(d.pVertexShader->GetPtr());
			if (d.pPixelShader)		m_PS = reinterpret_cast<Shader_internal*>(d.pPixelShader->GetPtr());
			if (d.pGeometryShader)	m_GS = reinterpret_cast<Shader_internal*>(d.pGeometryShader->GetPtr());

			m_InputLayout		= *d.pInputLayout;
			m_RasterizerState	= *d.pRasterizerState;
			m_BlendState		= *d.pBlendState;
			m_DepthStencilState = *d.pDepthStencilState;
			
			m_Topology = d.topology;
		}

		SV_GFX_VIEWPORT Image_internal::GetViewport() const noexcept
		{
			return { 0.f, 0.f, float(m_Width), float(m_Height), 0.f, 1.f };
		}

		SV_GFX_SCISSOR Image_internal::GetScissor() const noexcept
		{
			return { 0.f, 0.f, float(m_Width), float(m_Height) };
		}

		void Image_internal::SetDescription(const SV_GFX_IMAGE_DESC& d) noexcept
		{
			m_Dimension = d.dimension;
			m_Format = d.format;
			m_Width = d.width;
			m_Height = d.height;
			m_Depth = d.depth;
			m_Layers = d.layers;
		}

	}

	using namespace _internal;
	namespace Graphics {
		using namespace _internal;

		SV::_internal::PrimitiveAllocator		g_Allocator;
		SV::_internal::PipelineState			g_PipelineState;
		std::unique_ptr<GraphicsDevice>			g_Device;

		std::vector<std::unique_ptr<Adapter>>	g_Adapters;
		ui32									g_AdapterIndex;

		void ResetPipelineState(SV::CommandList cmd)
		{
			svZeroMemory(&g_PipelineState.graphics, sizeof(SV::_internal::GraphicsPipelineState));
			g_PipelineState.graphics->flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT;
			g_PipelineState.graphics->flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR;
		}
		void ResetPipelineState()
		{
			for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i)
				ResetPipelineState(i);
		}

		namespace _internal {

			bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
			{
				g_Device = std::make_unique<Graphics_vk>();

				// Initialize API
				if (!g_Device->Initialize(desc)) {
					SV::LogE("Can't initialize GraphicsAPI");
					return false;
				}

				ResetPipelineState();

				return true;
			}

			bool Close()
			{
				g_Allocator.Clear();

				if (!g_Device->Close()) {
					SV::LogE("Can't close GraphicsDevice");
				}

				return true;
			}

			void BeginFrame()
			{
				g_Device->BeginFrame();
			}
			void SubmitCommandLists()
			{
				g_Device->SubmitCommandLists();
				ResetPipelineState();
			}
			void Present()
			{
				g_Device->Present();
			}

			PrimitiveAllocator& GetPrimitiveAllocator() noexcept
			{
				return g_Allocator;
			}
			PipelineState& GetPipelineState() noexcept
			{
				return g_PipelineState;
			}
			GraphicsDevice* GetDevice() noexcept
			{
				return g_Device.get();
			}

			/////////////////////////////////////// HASH FUNCTIONS ///////////////////////////////////////

			size_t ComputeInputLayoutHash(const SV_GFX_INPUT_LAYOUT_DESC* d)
			{
				if (d == nullptr) return 0u;

				const SV_GFX_INPUT_LAYOUT_DESC& desc = *d;
				size_t hash = 0u;
				SV::HashCombine(hash, desc.slots.size());

				for (ui32 i = 0; i < desc.slots.size(); ++i) {
					const SV_GFX_INPUT_SLOT_DESC& slot = desc.slots[i];
					SV::HashCombine(hash, slot.slot);
					SV::HashCombine(hash, slot.stride);
					SV::HashCombine(hash, ui64(slot.instanced));
				}

				SV::HashCombine(hash, desc.elements.size());

				for (ui32 i = 0; i < desc.elements.size(); ++i) {
					const SV_GFX_INPUT_ELEMENT_DESC& element = desc.elements[i];
					SV::HashCombine(hash, ui64(element.format));
					SV::HashCombine(hash, element.inputSlot);
					SV::HashCombine(hash, element.offset);
					SV::HashString(hash, element.name);
				}

				return hash;
			}
			size_t ComputeBlendStateHash(const SV_GFX_BLEND_STATE_DESC* d)
			{
				if (d == nullptr) return 0u;

				const SV_GFX_BLEND_STATE_DESC& desc = *d;
				size_t hash = 0u;
				SV::HashCombine(hash, ui64(double(desc.blendConstants.x) * 2550.0));
				SV::HashCombine(hash, ui64(double(desc.blendConstants.y) * 2550.0));
				SV::HashCombine(hash, ui64(double(desc.blendConstants.z) * 2550.0));
				SV::HashCombine(hash, ui64(double(desc.blendConstants.w) * 2550.0));
				SV::HashCombine(hash, desc.attachments.size());

				for (ui32 i = 0; i < desc.attachments.size(); ++i)
				{
					const SV_GFX_BLEND_ATTACHMENT_DESC& att = desc.attachments[i];
					SV::HashCombine(hash, att.alphaBlendOp);
					SV::HashCombine(hash, att.blendEnabled ? 1u : 0u);
					SV::HashCombine(hash, att.colorBlendOp);
					SV::HashCombine(hash, att.colorWriteMask);
					SV::HashCombine(hash, att.dstAlphaBlendFactor);
					SV::HashCombine(hash, att.dstColorBlendFactor);
					SV::HashCombine(hash, att.srcAlphaBlendFactor);
					SV::HashCombine(hash, att.srcColorBlendFactor);
				}

				return hash;
			}
			size_t ComputeRasterizerStateHash(const SV_GFX_RASTERIZER_STATE_DESC* d)
			{
				if (d == nullptr) return 0u;

				const SV_GFX_RASTERIZER_STATE_DESC& desc = *d;
				size_t hash = 0u;
				SV::HashCombine(hash, desc.clockwise ? 1u : 0u);
				SV::HashCombine(hash, desc.cullMode);
				SV::HashCombine(hash, ui64(double(desc.lineWidth) * 10000.0));
				SV::HashCombine(hash, desc.wireframe ? 1u : 0u);
				
				return hash;
			}
			size_t ComputeDepthStencilStateHash(const SV_GFX_DEPTHSTENCIL_STATE_DESC* d)
			{
				if (d == nullptr) return 0u;

				const SV_GFX_DEPTHSTENCIL_STATE_DESC& desc = *d;
				size_t hash = 0u;
				SV::HashCombine(hash, desc.depthCompareOp);
				SV::HashCombine(hash, desc.depthTestEnabled ? 1u : 0u);
				SV::HashCombine(hash, desc.depthWriteEnabled ? 1u : 0u);
				SV::HashCombine(hash, desc.stencilTestEnabled ? 1u : 0u);
				SV::HashCombine(hash, desc.front.compareMask);
				SV::HashCombine(hash, desc.front.compareOp);
				SV::HashCombine(hash, desc.front.depthFailOp);
				SV::HashCombine(hash, desc.front.failOp);
				SV::HashCombine(hash, desc.front.passOp);
				SV::HashCombine(hash, desc.front.reference);
				SV::HashCombine(hash, desc.front.writeMask);
				SV::HashCombine(hash, desc.back.compareMask);
				SV::HashCombine(hash, desc.back.compareOp);
				SV::HashCombine(hash, desc.back.depthFailOp);
				SV::HashCombine(hash, desc.back.failOp);
				SV::HashCombine(hash, desc.back.passOp);
				SV::HashCombine(hash, desc.back.reference);
				SV::HashCombine(hash, desc.back.writeMask);

				return hash;
			}

		}

		//////////////////////////////////////ADAPTERS//////////////////////////////////////////
		void _internal::AddAdapter(std::unique_ptr<Adapter>&& adapter)
		{
			g_Adapters.push_back(std::move(adapter));
		}
		const std::vector<std::unique_ptr<SV::Adapter>>& GetAdapters() noexcept
		{
			return g_Adapters;
		}
		SV::Adapter* GetAdapter() noexcept
		{
			return g_Adapters[g_AdapterIndex].get();
		}
		void SetAdapter(ui32 index)
		{
			g_AdapterIndex = index;
			// TODO:
		}

		/////////////////////////////////////// API //////////////////////////////////////////////
#define SVAssertValidation(x) SV_ASSERT(x.IsValid())

		void Allocate(SV::Primitive& primitive, SV_GFX_PRIMITIVE type, const void* desc)
		{
			if (primitive.IsValid()) Destroy(primitive);
			else primitive = Primitive(g_Allocator.Construct(type, desc));
		}

		bool Destroy(SV::Primitive& primitive)
		{
			if (primitive.IsValid()) {
				return g_Allocator.Destroy(primitive);
			}
			return true;
		}

		CommandList BeginCommandList()
		{
			return g_Device->BeginCommandList();
		}

		void ResetState()
		{
		}

		void BindVertexBuffers(SV::Buffer** buffers, ui32* offsets, ui32* strides, ui32 count, SV::CommandList cmd)
		{
			g_PipelineState.graphics[cmd].vertexBuffersCount = count;
			for (ui32 i = 0; i < count; ++i) {
				g_PipelineState.graphics[cmd].vertexBuffers[i] = reinterpret_cast<Buffer_internal*>(buffers[i]->GetPtr());
			}
			memcpy(g_PipelineState.graphics[cmd].vertexBufferOffsets, offsets, sizeof(ui32) * count);
			memcpy(g_PipelineState.graphics[cmd].vertexBufferStrides, strides, sizeof(ui32) * count);
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VERTEX_BUFFER;
		}

		void BindIndexBuffer(SV::Buffer& buffer, ui32 offset, SV::CommandList cmd)
		{
			g_PipelineState.graphics[cmd].indexBuffer = reinterpret_cast<Buffer_internal*>(buffer.GetPtr());
			g_PipelineState.graphics[cmd].indexBufferOffset = offset;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_INDEX_BUFFER;
		}

		void BindConstantBuffers(SV::Buffer** buffers, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd)
		{
			for (ui32 i = 0; i < count; ++i) {
				g_PipelineState.graphics[cmd].constantBuffers[shaderType][i] = reinterpret_cast<Buffer_internal*>(buffers[i]->GetPtr());
			}
			g_PipelineState.graphics[cmd].constantBuffersCount[shaderType] = count;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER;

			switch (shaderType)
			{
			case SV_GFX_SHADER_TYPE_VERTEX:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_VS;
				break;
			case SV_GFX_SHADER_TYPE_PIXEL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_PS;
				break;
			case SV_GFX_SHADER_TYPE_GEOMETRY:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_GS;
				break;
			case SV_GFX_SHADER_TYPE_HULL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_HS;
				break;
			case SV_GFX_SHADER_TYPE_DOMAIN:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CONSTANT_BUFFER_DS;
				break;
			}
		}

		void BindImages(SV::Image** images, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd)
		{
			for (ui32 i = 0; i < count; ++i) {
				g_PipelineState.graphics[cmd].images[shaderType][i] = reinterpret_cast<Image_internal*>(images[i]->GetPtr());
			}
			g_PipelineState.graphics[cmd].imagesCount[shaderType] = count;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE;

			switch (shaderType)
			{
			case SV_GFX_SHADER_TYPE_VERTEX:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_VS;
				break;
			case SV_GFX_SHADER_TYPE_PIXEL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_PS;
				break;
			case SV_GFX_SHADER_TYPE_GEOMETRY:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_GS;
				break;
			case SV_GFX_SHADER_TYPE_HULL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_HS;
				break;
			case SV_GFX_SHADER_TYPE_DOMAIN:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_IMAGE_DS;
				break;
			}
		}
		void BindSamplers(SV::Sampler** samplers, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd)
		{
			for (ui32 i = 0; i < count; ++i) {
				g_PipelineState.graphics[cmd].sampers[shaderType][i] = reinterpret_cast<Sampler_internal*>(samplers[i]->GetPtr());
			}
			g_PipelineState.graphics[cmd].samplersCount[shaderType] = count;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER;

			switch (shaderType)
			{
			case SV_GFX_SHADER_TYPE_VERTEX:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_VS;
				break;
			case SV_GFX_SHADER_TYPE_PIXEL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_PS;
				break;
			case SV_GFX_SHADER_TYPE_GEOMETRY:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_GS;
				break;
			case SV_GFX_SHADER_TYPE_HULL:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_HS;
				break;
			case SV_GFX_SHADER_TYPE_DOMAIN:
				g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SAMPLER_DS;
				break;
			}
		}

		void BindRenderPass(SV::RenderPass& renderPass, SV::Image** attachments, SV::CommandList cmd)
		{
			g_PipelineState.graphics[cmd].renderPass = reinterpret_cast<RenderPass_internal*>(renderPass.GetPtr());

			ui32 attCount = renderPass->GetAttachmentsCount();
			for (ui32 i = 0; i < attCount; ++i) {
				g_PipelineState.graphics[cmd].attachments[i] = reinterpret_cast<Image_internal*>(attachments[i]->GetPtr());
			}

			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS;
		}
		void BindGraphicsPipeline(SV::GraphicsPipeline& pipeline, SV::CommandList cmd)
		{
			g_PipelineState.graphics[cmd].pipeline = reinterpret_cast<GraphicsPipeline_internal*>(pipeline.GetPtr());
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_PIPELINE;
		}
		void SetPipelineMode(SV_GFX_PIPELINE_MODE mode, SV::CommandList cmd)
		{
			g_PipelineState.mode[cmd] = mode;
		}
		void SetViewports(const SV_GFX_VIEWPORT* viewports, ui32 count, SV::CommandList cmd)
		{
			assert(count < SV_GFX_VIEWPORT_COUNT);
			memcpy(g_PipelineState.graphics[cmd].viewports, viewports, size_t(count) * sizeof(SV_GFX_VIEWPORT));
			g_PipelineState.graphics[cmd].viewportsCount = count;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT;
		}
		void SetScissors(const SV_GFX_SCISSOR* scissors, ui32 count, SV::CommandList cmd)
		{
			assert(count < SV_GFX_SCISSOR_COUNT);
			memcpy(g_PipelineState.graphics[cmd].scissors, scissors, size_t(count) * sizeof(SV_GFX_SCISSOR));
			g_PipelineState.graphics[cmd].scissorsCount = count;
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR;
		}
		void SetClearColors(const SV::vec4* colors, ui32 count, SV::CommandList cmd)
		{
			assert(count < SV_GFX_ATTACHMENTS_COUNT);
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, size_t(count) * sizeof(SV::vec4));
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_COLOR;
		}
		void SetClearDepthStencil(float depth, ui32 stencil, SV::CommandList cmd)
		{
			g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);
			g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_CLEAR_DEPTH_STENCIL;
		}

		////////////////////////////////////////// DRAW CALLS /////////////////////////////////////////
		
		void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, SV::CommandList cmd)
		{
			g_Device->Draw(vertexCount, instanceCount, startVertex, startInstance, cmd);
		}
		void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, SV::CommandList cmd)
		{
			g_Device->DrawIndexed(indexCount, instanceCount, startIndex, startVertex, startInstance, cmd);
		}

		////////////////////////////////////////// SWAPCHAIN /////////////////////////////////////////

		void ResizeSwapChain()
		{
			g_Device->ResizeSwapChain();
		}

		SV::Image& GetSwapChainBackBuffer()
		{
			return g_Device->GetSwapChainBackBuffer();
		}

		void UpdateBuffer(SV::Buffer& buffer, void* pData, ui32 size, ui32 offset, SV::CommandList cmd)
		{
			g_Device->UpdateBuffer(buffer, pData, size, offset, cmd);
		}

		////////////////////////////////////////// PRIMITIVES /////////////////////////////////////////

#define AllocateAndCreatePrimitive(primitive, type, desc) Allocate(primitive, type, desc); if(primitive.GetPtr() == nullptr) return false; \
		primitive->SetType(type); primitive->SetDescription(*desc); return true;

		bool CreateBuffer(const SV_GFX_BUFFER_DESC* desc, SV::Buffer& buffer)
		{
#ifdef SV_DEBUG
			if (desc->usage == SV_GFX_USAGE_STATIC && desc->CPUAccess != SV_GFX_CPU_ACCESS_NONE) {
				SV::LogE("Buffer with static usage can't have CPU access");
				return false;
			}
#endif
			AllocateAndCreatePrimitive(buffer, SV_GFX_PRIMITIVE_BUFFER, desc);
		}
		bool CreateShader(const SV_GFX_SHADER_DESC* desc, SV::Shader& shader)
		{
			AllocateAndCreatePrimitive(shader, SV_GFX_PRIMITIVE_SHADER, desc);
		}
		bool CreateImage(const SV_GFX_IMAGE_DESC* desc, SV::Image& image)
		{
			AllocateAndCreatePrimitive(image, SV_GFX_PRIMITIVE_IMAGE, desc);
		}
		bool CreateSampler(const SV_GFX_SAMPLER_DESC* desc, SV::Sampler& sampler)
		{
			AllocateAndCreatePrimitive(sampler, SV_GFX_PRIMITIVE_SAMPLER, desc);
		}
		bool CreateRenderPass(const SV_GFX_RENDERPASS_DESC* desc, SV::RenderPass& renderPass)
		{
			AllocateAndCreatePrimitive(renderPass, SV_GFX_PRIMITIVE_RENDERPASS, desc);
		}
		bool CreateGraphicsPipeline(const SV_GFX_GRAPHICS_PIPELINE_DESC* desc, SV::GraphicsPipeline& graphicsPipeline)
		{
			const SV_GFX_GRAPHICS_PIPELINE_DESC* res = nullptr;

			SV_GFX_GRAPHICS_PIPELINE_DESC auxDesc;
			SV_GFX_INPUT_LAYOUT_DESC auxInputLayout{};
			SV_GFX_RASTERIZER_STATE_DESC auxRasterizerState;
			SV_GFX_BLEND_STATE_DESC auxBlendState;
			SV_GFX_DEPTHSTENCIL_STATE_DESC auxDepthStencilState{};

			if (desc->pBlendState && desc->pDepthStencilState && desc->pInputLayout && desc->pRasterizerState) {
				res = desc;
			}
			else {
				res = &auxDesc;

				auxDesc.pVertexShader = desc->pVertexShader;
				auxDesc.pPixelShader = desc->pPixelShader;
				auxDesc.pGeometryShader = desc->pGeometryShader;

				auxDesc.topology = desc->topology;

				if (desc->pInputLayout) auxDesc.pInputLayout = desc->pInputLayout;
				else {
					auxDesc.pInputLayout = &auxInputLayout;
				}
				if (desc->pRasterizerState) auxDesc.pRasterizerState = desc->pRasterizerState;
				else {
					auxRasterizerState.clockwise = true;
					auxRasterizerState.cullMode = SV_GFX_CULL_NONE;
					auxRasterizerState.lineWidth = 1.f;
					auxRasterizerState.wireframe = false;

					auxDesc.pRasterizerState = &auxRasterizerState;
				}
				if (desc->pBlendState) auxDesc.pBlendState = desc->pBlendState;
				else {
					SV_GFX_BLEND_ATTACHMENT_DESC auxBlendAtt{};

					auxBlendState.blendConstants = {0.f, 0.f, 0.f, 0.f};
					auxBlendAtt.blendEnabled = false;
					auxBlendAtt.colorWriteMask = SV_GFX_COLOR_COMPONENT_ALL;

					auxBlendState.attachments.emplace_back(auxBlendAtt);

					auxDesc.pBlendState = &auxBlendState;
				}
				if (desc->pDepthStencilState) auxDesc.pDepthStencilState = desc->pDepthStencilState;
				else {
					auxDepthStencilState.depthTestEnabled = false;
					auxDepthStencilState.depthWriteEnabled = false;
					auxDepthStencilState.stencilTestEnabled = false;

					auxDesc.pDepthStencilState = &auxDepthStencilState;
				}
			}

			AllocateAndCreatePrimitive(graphicsPipeline, SV_GFX_PRIMITIVE_GRAPHICS_PIPELINE, res);
		}

	}
}