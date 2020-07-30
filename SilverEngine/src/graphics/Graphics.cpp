#include "core.h"

#include "Graphics.h"
#include "vulkan/graphics_vulkan.h"
#include "platform/Window.h"
#include "graphics_state.h"

using namespace sv;

namespace _sv {

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

		m_InputLayout = *d.pInputLayout;
		m_RasterizerState = *d.pRasterizerState;
		m_BlendState = *d.pBlendState;
		m_DepthStencilState = *d.pDepthStencilState;

		m_Topology = d.topology;
	}

	Viewport Image_internal::GetViewport() const noexcept
	{
		return { 0.f, 0.f, float(m_Width), float(m_Height), 0.f, 1.f };
	}

	Scissor Image_internal::GetScissor() const noexcept
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
		m_ImageType = d.type;
	}

}

namespace _sv {

	static PrimitiveAllocator						g_Allocator;
	static PipelineState							g_PipelineState;
	static std::unique_ptr<GraphicsDevice>			g_Device;
	
	static std::vector<std::unique_ptr<Adapter>>	g_Adapters;
	static ui32										g_AdapterIndex;

	void ResetPipelineState(CommandList cmd)
	{
		svZeroMemory(&g_PipelineState.graphics, sizeof(GraphicsPipelineState));
		g_PipelineState.graphics->flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT;
		g_PipelineState.graphics->flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR;
	}
	void ResetPipelineState()
	{
		for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i)
			ResetPipelineState(i);
	}

	bool graphics_initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		g_Device = std::make_unique<_sv::Graphics_vk>();

		// Initialize API
		if (!g_Device->Initialize(desc)) {
			sv::log_error("Can't initialize GraphicsAPI");
			return false;
		}

		ResetPipelineState();

		return true;
	}

	bool graphics_close()
	{
		g_Allocator.Clear();

		if (!g_Device->Close()) {
			sv::log_error("Can't close GraphicsDevice");
		}

		return true;
	}

	void graphics_begin()
	{
		g_Device->BeginFrame();
	}
	void graphics_commandlist_submit()
	{
		g_Device->SubmitCommandLists();
		graphics_state_reset();
	}
	void graphics_present()
	{
		g_Device->Present();
	}

	void graphics_swapchain_resize()
	{
		g_Device->ResizeSwapChain();
	}

	Image& graphics_swapchain_acquire_image()
	{
		return g_Device->AcquireSwapChainImage();
	}

	PrimitiveAllocator& graphics_allocator_get() noexcept
	{
		return g_Allocator;
	}
	PipelineState& graphics_state_get() noexcept
	{
		return g_PipelineState;
	}
	GraphicsDevice* graphics_device_get() noexcept
	{
		return g_Device.get();
	}

	void graphics_adapter_add(std::unique_ptr<Adapter>&& adapter)
	{
		g_Adapters.push_back(std::move(adapter));
	}

	/////////////////////////////////////// HASH FUNCTIONS ///////////////////////////////////////

	size_t graphics_compute_hash_inputlayout(const SV_GFX_INPUT_LAYOUT_DESC* d)
	{
		if (d == nullptr) return 0u;

		const SV_GFX_INPUT_LAYOUT_DESC& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.slots.size());

		for (ui32 i = 0; i < desc.slots.size(); ++i) {
			const SV_GFX_INPUT_SLOT_DESC& slot = desc.slots[i];
			sv::utils_hash_combine(hash, slot.slot);
			sv::utils_hash_combine(hash, slot.stride);
			sv::utils_hash_combine(hash, ui64(slot.instanced));
		}

		sv::utils_hash_combine(hash, desc.elements.size());

		for (ui32 i = 0; i < desc.elements.size(); ++i) {
			const SV_GFX_INPUT_ELEMENT_DESC& element = desc.elements[i];
			sv::utils_hash_combine(hash, ui64(element.format));
			sv::utils_hash_combine(hash, element.inputSlot);
			sv::utils_hash_combine(hash, element.offset);
			sv::utils_hash_string(hash, element.name);
		}

		return hash;
	}
	size_t graphics_compute_hash_blendstate(const SV_GFX_BLEND_STATE_DESC* d)
	{
		if (d == nullptr) return 0u;

		const SV_GFX_BLEND_STATE_DESC& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.x) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.y) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.z) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.w) * 2550.0));
		sv::utils_hash_combine(hash, desc.attachments.size());

		for (ui32 i = 0; i < desc.attachments.size(); ++i)
		{
			const SV_GFX_BLEND_ATTACHMENT_DESC& att = desc.attachments[i];
			sv::utils_hash_combine(hash, att.alphaBlendOp);
			sv::utils_hash_combine(hash, att.blendEnabled ? 1u : 0u);
			sv::utils_hash_combine(hash, att.colorBlendOp);
			sv::utils_hash_combine(hash, att.colorWriteMask);
			sv::utils_hash_combine(hash, att.dstAlphaBlendFactor);
			sv::utils_hash_combine(hash, att.dstColorBlendFactor);
			sv::utils_hash_combine(hash, att.srcAlphaBlendFactor);
			sv::utils_hash_combine(hash, att.srcColorBlendFactor);
		}

		return hash;
	}
	size_t graphics_compute_hash_rasterizerstate(const SV_GFX_RASTERIZER_STATE_DESC* d)
	{
		if (d == nullptr) return 0u;

		const SV_GFX_RASTERIZER_STATE_DESC& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.clockwise ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.cullMode);
		sv::utils_hash_combine(hash, ui64(double(desc.lineWidth) * 10000.0));
		sv::utils_hash_combine(hash, desc.wireframe ? 1u : 0u);

		return hash;
	}
	size_t graphics_compute_hash_depthstencilstate(const SV_GFX_DEPTHSTENCIL_STATE_DESC* d)
	{
		if (d == nullptr) return 0u;

		const SV_GFX_DEPTHSTENCIL_STATE_DESC& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.depthCompareOp);
		sv::utils_hash_combine(hash, desc.depthTestEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.depthWriteEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.stencilTestEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.front.compareMask);
		sv::utils_hash_combine(hash, desc.front.compareOp);
		sv::utils_hash_combine(hash, desc.front.depthFailOp);
		sv::utils_hash_combine(hash, desc.front.failOp);
		sv::utils_hash_combine(hash, desc.front.passOp);
		sv::utils_hash_combine(hash, desc.front.reference);
		sv::utils_hash_combine(hash, desc.front.writeMask);
		sv::utils_hash_combine(hash, desc.back.compareMask);
		sv::utils_hash_combine(hash, desc.back.compareOp);
		sv::utils_hash_combine(hash, desc.back.depthFailOp);
		sv::utils_hash_combine(hash, desc.back.failOp);
		sv::utils_hash_combine(hash, desc.back.passOp);
		sv::utils_hash_combine(hash, desc.back.reference);
		sv::utils_hash_combine(hash, desc.back.writeMask);

		return hash;
	}

}

namespace sv {

	using namespace _sv;

	//////////////////////////////////////ADAPTERS//////////////////////////////////////////
	const std::vector<std::unique_ptr<Adapter>>& graphics_adapter_get_list() noexcept
	{
		return g_Adapters;
	}
	Adapter* graphics_adapter_get() noexcept
	{
		return g_Adapters[g_AdapterIndex].get();
	}
	void graphics_adapter_set(ui32 index)
	{
		g_AdapterIndex = index;
		// TODO:
	}

	////////////////////////////////////////// PRIMITIVES /////////////////////////////////////////

#define SVAssertValidation(x) SV_ASSERT(x.IsValid())

	void Allocate(Primitive& primitive, SV_GFX_PRIMITIVE type, const void* desc)
	{
		if (primitive.IsValid()) graphics_destroy(primitive);
		else primitive = Primitive(g_Allocator.Construct(type, desc));
	}

#define AllocateAndCreatePrimitive(primitive, type, desc) Allocate(primitive, type, desc); if(primitive.GetPtr() == nullptr) return false; \
		primitive->SetType(type); primitive->SetDescription(*desc); return true;

	bool graphics_buffer_create(const SV_GFX_BUFFER_DESC* desc, Buffer& buffer)
	{
#ifdef SV_DEBUG
		if (desc->usage == SV_GFX_USAGE_STATIC && desc->CPUAccess != SV_GFX_CPU_ACCESS_NONE) {
			sv::log_error("Buffer with static usage can't have CPU access");
			return false;
		}
#endif
		AllocateAndCreatePrimitive(buffer, SV_GFX_PRIMITIVE_BUFFER, desc);
	}
	bool graphics_shader_create(const SV_GFX_SHADER_DESC* desc, Shader& shader)
	{
		AllocateAndCreatePrimitive(shader, SV_GFX_PRIMITIVE_SHADER, desc);
	}
	bool graphics_image_create(const SV_GFX_IMAGE_DESC* desc, Image& image)
	{
		AllocateAndCreatePrimitive(image, SV_GFX_PRIMITIVE_IMAGE, desc);
	}
	bool graphics_sampler_create(const SV_GFX_SAMPLER_DESC* desc, Sampler& sampler)
	{
		AllocateAndCreatePrimitive(sampler, SV_GFX_PRIMITIVE_SAMPLER, desc);
	}
	bool graphics_renderpass_create(const SV_GFX_RENDERPASS_DESC* desc, RenderPass& renderPass)
	{
		AllocateAndCreatePrimitive(renderPass, SV_GFX_PRIMITIVE_RENDERPASS, desc);
	}
	bool graphics_pipeline_create(const SV_GFX_GRAPHICS_PIPELINE_DESC* desc, GraphicsPipeline& graphicsPipeline)
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

				auxBlendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
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

	bool graphics_destroy(Primitive& primitive)
	{
		if (primitive.IsValid()) {
			return g_Allocator.Destroy(primitive);
		}
		return true;
	}

	CommandList graphics_commandlist_begin()
	{
		return g_Device->BeginCommandList();
	}

	void graphics_gpu_wait()
	{
		g_Device->WaitGPU();
	}

	void graphics_state_reset()
	{
	}

	void graphics_vertexbuffer_bind(Buffer** buffers, ui32* offsets, ui32* strides, ui32 count, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].vertexBuffersCount = count;
		for (ui32 i = 0; i < count; ++i) {
			g_PipelineState.graphics[cmd].vertexBuffers[i] = reinterpret_cast<Buffer_internal*>(buffers[i]->GetPtr());
		}
		memcpy(g_PipelineState.graphics[cmd].vertexBufferOffsets, offsets, sizeof(ui32) * count);
		memcpy(g_PipelineState.graphics[cmd].vertexBufferStrides, strides, sizeof(ui32) * count);
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VERTEX_BUFFER;
	}

	void graphics_indexbuffer_bind(Buffer& buffer, ui32 offset, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].indexBuffer = reinterpret_cast<Buffer_internal*>(buffer.GetPtr());
		g_PipelineState.graphics[cmd].indexBufferOffset = offset;
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_INDEX_BUFFER;
	}

	void graphics_constantbuffer_bind(Buffer** buffers, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd)
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

	void graphics_image_bind(Image** images, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd)
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
	void graphics_sampler_bind(Sampler** samplers, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd)
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

	void graphics_pipeline_bind(GraphicsPipeline& pipeline, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].pipeline = reinterpret_cast<GraphicsPipeline_internal*>(pipeline.GetPtr());
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_PIPELINE;
	}

	void graphics_renderpass_begin(RenderPass& renderPass, Image** attachments, const Color4f* colors, float depth, ui32 stencil, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].renderPass = reinterpret_cast<RenderPass_internal*>(renderPass.GetPtr());

		ui32 attCount = renderPass->GetAttachmentsCount();
		for (ui32 i = 0; i < attCount; ++i) {
			g_PipelineState.graphics[cmd].attachments[i] = reinterpret_cast<Image_internal*>(attachments[i]->GetPtr());
		}
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS;

		if(colors != nullptr)
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, renderPass->GetAttachmentsCount() * sizeof(vec4));
		g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);

		g_Device->BeginRenderPass(cmd);
	}
	void graphics_renderpass_end(CommandList cmd)
	{
		g_Device->EndRenderPass(cmd);
		g_PipelineState.graphics[cmd].renderPass = nullptr;
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_RENDER_PASS;
	}

	void graphics_set_pipeline_mode(SV_GFX_PIPELINE_MODE mode, CommandList cmd)
	{
		g_PipelineState.mode[cmd] = mode;
	}
	void graphics_set_viewports(const Viewport* viewports, ui32 count, CommandList cmd)
	{
		assert(count < SV_GFX_VIEWPORT_COUNT);
		memcpy(g_PipelineState.graphics[cmd].viewports, viewports, size_t(count) * sizeof(Viewport));
		g_PipelineState.graphics[cmd].viewportsCount = count;
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_VIEWPORT;
	}
	void graphics_set_scissors(const Scissor* scissors, ui32 count, CommandList cmd)
	{
		assert(count < SV_GFX_SCISSOR_COUNT);
		memcpy(g_PipelineState.graphics[cmd].scissors, scissors, size_t(count) * sizeof(Scissor));
		g_PipelineState.graphics[cmd].scissorsCount = count;
		g_PipelineState.graphics[cmd].flags |= SV_GFX_GRAPHICS_PIPELINE_STATE_SCISSOR;
	}

	////////////////////////////////////////// DRAW CALLS /////////////////////////////////////////

	void graphics_draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		g_Device->Draw(vertexCount, instanceCount, startVertex, startInstance, cmd);
	}
	void graphics_draw_indexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		g_Device->DrawIndexed(indexCount, instanceCount, startIndex, startVertex, startInstance, cmd);
	}

	////////////////////////////////////////// MEMORY /////////////////////////////////////////

	void graphics_buffer_update(Buffer& buffer, void* pData, ui32 size, ui32 offset, CommandList cmd)
	{
		g_Device->UpdateBuffer(buffer, pData, size, offset, cmd);
	}
	void graphics_barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd)
	{
		g_Device->Barrier(barriers, count, cmd);
	}
}