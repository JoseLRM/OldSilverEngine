#include "core.h"

#include "graphics_internal.h"
#include "window.h"

#include "vulkan/graphics_vulkan.h"

namespace sv {

	static PipelineState							g_PipelineState;
	static std::unique_ptr<GraphicsDevice>			g_Device;
	
	static std::vector<std::unique_ptr<Adapter>>	g_Adapters;
	static ui32										g_AdapterIndex;

	void ResetPipelineState(CommandList cmd)
	{
		svZeroMemory(&g_PipelineState.graphics, sizeof(GraphicsPipelineState));
		g_PipelineState.graphics->flags |= GraphicsPipelineState_Viewport;
		g_PipelineState.graphics->flags |= GraphicsPipelineState_Scissor;
	}
	void ResetPipelineState()
	{
		for (ui32 i = 0; i < SV_GFX_COMMAND_LIST_COUNT; ++i)
			ResetPipelineState(i);
	}

	bool graphics_initialize(const InitializationGraphicsDesc& desc)
	{
		g_Device = std::make_unique<Graphics_vk>();

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
		graphics_allocator_clear();

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

	GPUImage& graphics_swapchain_acquire_image()
	{
		return g_Device->AcquireSwapChainImage();
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

	size_t graphics_compute_hash_inputlayout(const InputLayoutDesc* d)
	{
		if (d == nullptr) return 0u;

		const InputLayoutDesc& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.slots.size());

		for (ui32 i = 0; i < desc.slots.size(); ++i) {
			const InputSlotDesc& slot = desc.slots[i];
			sv::utils_hash_combine(hash, slot.slot);
			sv::utils_hash_combine(hash, slot.stride);
			sv::utils_hash_combine(hash, ui64(slot.instanced));
		}

		sv::utils_hash_combine(hash, desc.elements.size());

		for (ui32 i = 0; i < desc.elements.size(); ++i) {
			const InputElementDesc& element = desc.elements[i];
			sv::utils_hash_combine(hash, ui64(element.format));
			sv::utils_hash_combine(hash, element.inputSlot);
			sv::utils_hash_combine(hash, element.offset);
			sv::utils_hash_string(hash, element.name);
		}

		return hash;
	}
	size_t graphics_compute_hash_blendstate(const BlendStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const BlendStateDesc& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.x) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.y) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.z) * 2550.0));
		sv::utils_hash_combine(hash, ui64(double(desc.blendConstants.w) * 2550.0));
		sv::utils_hash_combine(hash, desc.attachments.size());

		for (ui32 i = 0; i < desc.attachments.size(); ++i)
		{
			const BlendAttachmentDesc& att = desc.attachments[i];
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
	size_t graphics_compute_hash_rasterizerstate(const RasterizerStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const RasterizerStateDesc& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.clockwise ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.cullMode);
		sv::utils_hash_combine(hash, ui64(double(desc.lineWidth) * 10000.0));
		sv::utils_hash_combine(hash, desc.wireframe ? 1u : 0u);

		return hash;
	}
	size_t graphics_compute_hash_depthstencilstate(const DepthStencilStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const DepthStencilStateDesc& desc = *d;
		size_t hash = 0u;
		sv::utils_hash_combine(hash, desc.depthCompareOp);
		sv::utils_hash_combine(hash, desc.depthTestEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.depthWriteEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.stencilTestEnabled ? 1u : 0u);
		sv::utils_hash_combine(hash, desc.readMask);
		sv::utils_hash_combine(hash, desc.writeMask);
		sv::utils_hash_combine(hash, desc.front.compareOp);
		sv::utils_hash_combine(hash, desc.front.depthFailOp);
		sv::utils_hash_combine(hash, desc.front.failOp);
		sv::utils_hash_combine(hash, desc.front.passOp);
		sv::utils_hash_combine(hash, desc.back.compareOp);
		sv::utils_hash_combine(hash, desc.back.depthFailOp);
		sv::utils_hash_combine(hash, desc.back.failOp);
		sv::utils_hash_combine(hash, desc.back.passOp);

		return hash;
	}

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

	bool Allocate(Primitive& primitive, GraphicsPrimitiveType type, const void* desc)
	{
		if (primitive.IsValid()) graphics_destroy(primitive);
		else primitive = Primitive(graphics_allocator_construct(type, desc));
		return primitive.IsValid();
	}

	bool graphics_buffer_create(const GPUBufferDesc* desc, GPUBuffer& buffer)
	{
#ifdef SV_DEBUG
		if (desc->usage == ResourceUsage_Static && desc->CPUAccess & CPUAccess_Write) {
			sv::log_error("Buffer with static usage can't have CPU access");
			return false;
		}
#endif
		
		svCheck(Allocate(buffer, GraphicsPrimitiveType_Buffer, desc));

		GPUBuffer_internal* p = reinterpret_cast<GPUBuffer_internal*>(buffer.GetPtr());
		p->type = GraphicsPrimitiveType_Buffer;
		p->bufferType = desc->bufferType;
		p->usage = desc->usage;
		p->size = desc->size;
		p->indexType = desc->indexType;
		p->cpuAccess = desc->CPUAccess;

		return true;
	}

	bool graphics_shader_create(const ShaderDesc* desc, Shader& shader)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(shader, GraphicsPrimitiveType_Shader, desc));

		Shader_internal* p = reinterpret_cast<Shader_internal*>(shader.GetPtr());
		p->type = GraphicsPrimitiveType_Shader;
		p->shaderType = desc->shaderType;
		p->filePath = desc->filePath;

		return true;
	}

	bool graphics_image_create(const GPUImageDesc* desc, GPUImage& image)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(image, GraphicsPrimitiveType_Image, desc));

		GPUImage_internal* p = reinterpret_cast<GPUImage_internal*>(image.GetPtr());
		p->type = GraphicsPrimitiveType_Image;
		p->dimension = desc->dimension;
		p->format = desc->format;
		p->width = desc->width;
		p->height = desc->height;
		p->depth = desc->depth;
		p->layers = desc->layers;
		p->imageType = desc->type;

		return true;
	}

	bool graphics_sampler_create(const SamplerDesc* desc, Sampler& sampler)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(sampler, GraphicsPrimitiveType_Sampler, desc));

		Sampler_internal* p = reinterpret_cast<Sampler_internal*>(sampler.GetPtr());
		p->type = GraphicsPrimitiveType_Sampler;

		return true;
	}

	bool graphics_renderpass_create(const RenderPassDesc* desc, RenderPass& renderPass)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(renderPass, GraphicsPrimitiveType_RenderPass, desc));

		RenderPass_internal* p = reinterpret_cast<RenderPass_internal*>(renderPass.GetPtr());
		p->type = GraphicsPrimitiveType_RenderPass;
		p->depthStencilAttachment = ui32(desc->attachments.size());
		for (ui32 i = 0; i < desc->attachments.size(); ++i) {
			if (desc->attachments[i].type == AttachmentType_DepthStencil) {
				p->depthStencilAttachment = i;
				break;
			}
		}
		p->attachments = desc->attachments;

		return true;
	}
	
	bool graphics_pipeline_create(const GraphicsPipelineDesc* desc, GraphicsPipeline& graphicsPipeline)
	{
		const GraphicsPipelineDesc* res = nullptr;

		GraphicsPipelineDesc auxDesc;
		InputLayoutDesc auxInputLayout{};
		RasterizerStateDesc auxRasterizerState;
		BlendStateDesc auxBlendState;
		DepthStencilStateDesc auxDepthStencilState{};

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
				auxRasterizerState.cullMode = RasterizerCullMode_None;
				auxRasterizerState.lineWidth = 1.f;
				auxRasterizerState.wireframe = false;

				auxDesc.pRasterizerState = &auxRasterizerState;
			}
			if (desc->pBlendState) auxDesc.pBlendState = desc->pBlendState;
			else {
				BlendAttachmentDesc auxBlendAtt{};

				auxBlendState.blendConstants = { 0.f, 0.f, 0.f, 0.f };
				auxBlendAtt.blendEnabled = false;
				auxBlendAtt.colorWriteMask = ColorComponent_All;

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

#ifdef SV_DEBUG
#endif

		svCheck(Allocate(graphicsPipeline, GraphicsPrimitiveType_GraphicsPipeline, desc));

		GraphicsPipeline_internal* p = reinterpret_cast<GraphicsPipeline_internal*>(graphicsPipeline.GetPtr());
		p->type = GraphicsPrimitiveType_GraphicsPipeline;

		p->vs = nullptr;
		p->ps = nullptr;
		p->gs = nullptr;
		if (res->pVertexShader)	p->vs = reinterpret_cast<Shader_internal*>(res->pVertexShader->GetPtr());
		if (res->pPixelShader)		p->ps = reinterpret_cast<Shader_internal*>(res->pPixelShader->GetPtr());
		if (res->pGeometryShader)	p->gs = reinterpret_cast<Shader_internal*>(res->pGeometryShader->GetPtr());
		
		p->inputLayout =		*res->pInputLayout;
		p->rasterizerState =	*res->pRasterizerState;
		p->blendState =			*res->pBlendState;
		p->depthStencilState =	*res->pDepthStencilState;
		
		p->topology = res->topology;

		return true;
	}

	bool graphics_destroy(Primitive& primitive)
	{
		if (primitive.IsValid()) {
			bool res = graphics_allocator_destroy(primitive);
			primitive = Primitive(nullptr);
			return res;
		}
		return true;
	}

	CommandList graphics_commandlist_begin()
	{
		return g_Device->BeginCommandList();
	}

	CommandList graphics_commandlist_last()
	{
		return g_Device->GetLastCommandList();
	}

	void graphics_gpu_wait()
	{
		g_Device->WaitGPU();
	}

	void graphics_state_reset()
	{
	}

	void graphics_vertexbuffer_bind(GPUBuffer** buffers, ui32* offsets, ui32* strides, ui32 count, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].vertexBuffersCount = count;
		for (ui32 i = 0; i < count; ++i) {
			g_PipelineState.graphics[cmd].vertexBuffers[i] = reinterpret_cast<GPUBuffer_internal*>(buffers[i]->GetPtr());
		}
		memcpy(g_PipelineState.graphics[cmd].vertexBufferOffsets, offsets, sizeof(ui32) * count);
		memcpy(g_PipelineState.graphics[cmd].vertexBufferStrides, strides, sizeof(ui32) * count);
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_indexbuffer_bind(GPUBuffer& buffer, ui32 offset, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].indexBuffer = reinterpret_cast<GPUBuffer_internal*>(buffer.GetPtr());
		g_PipelineState.graphics[cmd].indexBufferOffset = offset;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_IndexBuffer;
	}

	void graphics_constantbuffer_bind(GPUBuffer** buffers, ui32 count, ShaderType shaderType, CommandList cmd)
	{
		for (ui32 i = 0; i < count; ++i) {
			g_PipelineState.graphics[cmd].constantBuffers[shaderType][i] = reinterpret_cast<GPUBuffer_internal*>(buffers[i]->GetPtr());
		}
		g_PipelineState.graphics[cmd].constantBuffersCount[shaderType] = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer;

		switch (shaderType)
		{
		case ShaderType_Vertex:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer_VS;
			break;
		case ShaderType_Pixel:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer_PS;
			break;
		case ShaderType_Geometry:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer_GS;
			break;
		case ShaderType_Hull:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer_HS;
			break;
		case ShaderType_Domain:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_ConstantBuffer_DS;
			break;
		}
	}

	void graphics_image_bind(GPUImage** images, ui32 count, ShaderType shaderType, CommandList cmd)
	{
		for (ui32 i = 0; i < count; ++i) {
			g_PipelineState.graphics[cmd].images[shaderType][i] = reinterpret_cast<GPUImage_internal*>(images[i]->GetPtr());
		}
		g_PipelineState.graphics[cmd].imagesCount[shaderType] = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image;

		switch (shaderType)
		{
		case ShaderType_Vertex:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image_VS;
			break;
		case ShaderType_Pixel:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image_PS;
			break;
		case ShaderType_Geometry:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image_GS;
			break;
		case ShaderType_Hull:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image_HS;
			break;
		case ShaderType_Domain:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Image_DS;
			break;
		}
	}
	void graphics_sampler_bind(Sampler** samplers, ui32 count, ShaderType shaderType, CommandList cmd)
	{
		for (ui32 i = 0; i < count; ++i) {
			g_PipelineState.graphics[cmd].sampers[shaderType][i] = reinterpret_cast<Sampler_internal*>(samplers[i]->GetPtr());
		}
		g_PipelineState.graphics[cmd].samplersCount[shaderType] = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler;

		switch (shaderType)
		{
		case ShaderType_Vertex:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler_VS;
			break;
		case ShaderType_Pixel:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler_PS;
			break;
		case ShaderType_Geometry:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler_GS;
			break;
		case ShaderType_Hull:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler_HS;
			break;
		case ShaderType_Domain:
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Sampler_DS;
			break;
		}
	}

	void graphics_pipeline_bind(GraphicsPipeline& pipeline, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].pipeline = reinterpret_cast<GraphicsPipeline_internal*>(pipeline.GetPtr());
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Pipeline;
	}

	void graphics_renderpass_begin(RenderPass& renderPass, GPUImage** attachments, const Color4f* colors, float depth, ui32 stencil, CommandList cmd)
	{
		RenderPass_internal* rp = reinterpret_cast<RenderPass_internal*>(renderPass.GetPtr());
		g_PipelineState.graphics[cmd].renderPass = rp;

		ui32 attCount = ui32(rp->attachments.size());
		for (ui32 i = 0; i < attCount; ++i) {
			g_PipelineState.graphics[cmd].attachments[i] = reinterpret_cast<GPUImage_internal*>(attachments[i]->GetPtr());
		}
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;

		if(colors != nullptr)
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, rp->attachments.size() * sizeof(vec4));
		g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);

		g_Device->BeginRenderPass(cmd);
	}
	void graphics_renderpass_end(CommandList cmd)
	{
		g_Device->EndRenderPass(cmd);
		g_PipelineState.graphics[cmd].renderPass = nullptr;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;
	}

	void graphics_set_pipeline_mode(GraphicsPipelineMode mode, CommandList cmd)
	{
		g_PipelineState.mode[cmd] = mode;
	}
	void graphics_set_viewports(const Viewport* viewports, ui32 count, CommandList cmd)
	{
		assert(count < SV_GFX_VIEWPORT_COUNT);
		memcpy(g_PipelineState.graphics[cmd].viewports, viewports, size_t(count) * sizeof(Viewport));
		g_PipelineState.graphics[cmd].viewportsCount = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Viewport;
	}
	void graphics_set_scissors(const Scissor* scissors, ui32 count, CommandList cmd)
	{
		assert(count < SV_GFX_SCISSOR_COUNT);
		memcpy(g_PipelineState.graphics[cmd].scissors, scissors, size_t(count) * sizeof(Scissor));
		g_PipelineState.graphics[cmd].scissorsCount = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_set_stencil_reference(ui32 ref, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].stencilReference = ref;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_StencilRef;
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

	void graphics_buffer_update(GPUBuffer& buffer, void* pData, ui32 size, ui32 offset, CommandList cmd)
	{
		g_Device->UpdateBuffer(buffer, pData, size, offset, cmd);
	}
	void graphics_barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd)
	{
		g_Device->Barrier(barriers, count, cmd);
	}
	void graphics_image_clear(GPUImage& image, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, ui32 stencil, CommandList cmd)
	{
		g_Device->ClearImage(image, oldLayout, newLayout, clearColor, depth, stencil, cmd);
	}

	ui32 graphics_image_get_width(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->width;
	}
	ui32 graphics_image_get_height(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->height;
	}
	ui32 graphics_image_get_depth(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->depth;
	}
	ui32 graphics_image_get_layers(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->layers;
	}
	ui32 graphics_image_get_dimension(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->dimension;
	}
	Format graphics_image_get_format(const GPUImage& image)
	{
		return reinterpret_cast<GPUImage_internal*>(image.GetPtr())->format;
	}
	Viewport graphics_image_get_viewport(const GPUImage& image)
	{
		GPUImage_internal& p = *reinterpret_cast<GPUImage_internal*>(image.GetPtr());
		return { 0.f, 0.f, float(p.width), float(p.height), 0.f, 1.f };
	}
	Scissor graphics_image_get_scissor(const GPUImage& image)
	{
		GPUImage_internal& p = *reinterpret_cast<GPUImage_internal*>(image.GetPtr());
		return { 0.f, 0.f, float(p.width), float(p.height) };
	}

}