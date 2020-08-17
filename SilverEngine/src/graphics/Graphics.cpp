#include "core.h"

#include "graphics_internal.h"
#include "window.h"

#include "vulkan/graphics_vulkan.h"

namespace sv {

	static PipelineState							g_PipelineState;
	static std::unique_ptr<GraphicsDevice>			g_Device;
	
	static std::vector<std::unique_ptr<Adapter>>	g_Adapters;
	static ui32										g_AdapterIndex;

	// Default Primitives

	static InputLayoutState		g_DefInputLayoutState;
	static BlendState			g_DefBlendState;
	static DepthStencilState	g_DefDepthStencilState;
	static RasterizerState		g_DefRasterizerState;

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

		// Create default states
		{
			InputLayoutStateDesc desc;
			svCheck(graphics_inputlayoutstate_create(&desc, g_DefInputLayoutState));
		}
		{
			BlendStateDesc desc;
			desc.attachments.resize(1u);
			desc.attachments[0].blendEnabled = false;
			desc.attachments[0].srcColorBlendFactor = BlendFactor_One;
			desc.attachments[0].dstColorBlendFactor = BlendFactor_One;
			desc.attachments[0].colorBlendOp = BlendOperation_Add;
			desc.attachments[0].srcAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].dstAlphaBlendFactor = BlendFactor_One;
			desc.attachments[0].alphaBlendOp = BlendOperation_Add;
			desc.attachments[0].colorWriteMask = ColorComponent_All;
			desc.blendConstants = { 0.f, 0.f, 0.f, 0.f };
			svCheck(graphics_blendstate_create(&desc, g_DefBlendState));
		}
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = false;
			desc.depthWriteEnabled = false;
			desc.depthCompareOp = CompareOperation_Always;
			desc.stencilTestEnabled = false;
			desc.readMask = 0xFF;
			desc.writeMask = 0xFF;
			svCheck(graphics_depthstencilstate_create(&desc, g_DefDepthStencilState));
		}
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.lineWidth = 1.f;
			desc.cullMode = RasterizerCullMode_None;
			desc.clockwise = true;
			svCheck(graphics_rasterizerstate_create(&desc, g_DefRasterizerState));
		}

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

	size_t graphics_compute_hash_inputlayoutstate(const InputLayoutStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const InputLayoutStateDesc& desc = *d;
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

	bool graphics_inputlayoutstate_create(const InputLayoutStateDesc* desc, InputLayoutState& inputLayoutState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(inputLayoutState, GraphicsPrimitiveType_InputLayoutState, desc));

		InputLayoutState_internal* p = reinterpret_cast<InputLayoutState_internal*>(inputLayoutState.GetPtr());
		p->type = GraphicsPrimitiveType_InputLayoutState;
		p->desc = *desc;

		return true;
	}

	bool graphics_blendstate_create(const BlendStateDesc* desc, BlendState& blendState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(blendState, GraphicsPrimitiveType_BlendState, desc));

		BlendState_internal* p = reinterpret_cast<BlendState_internal*>(blendState.GetPtr());
		p->type = GraphicsPrimitiveType_BlendState;
		p->desc = *desc;

		return true;
	}

	bool graphics_depthstencilstate_create(const DepthStencilStateDesc* desc, DepthStencilState& depthStencilState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(depthStencilState, GraphicsPrimitiveType_DepthStencilState, desc));

		DepthStencilState_internal* p = reinterpret_cast<DepthStencilState_internal*>(depthStencilState.GetPtr());
		p->type = GraphicsPrimitiveType_DepthStencilState;
		p->desc = *desc;

		return true;
	}

	bool graphics_rasterizerstate_create(const RasterizerStateDesc* desc, RasterizerState& rasterizerState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(rasterizerState, GraphicsPrimitiveType_RasterizerState, desc));

		RasterizerState_internal* p = reinterpret_cast<RasterizerState_internal*>(rasterizerState.GetPtr());
		p->type = GraphicsPrimitiveType_RasterizerState;
		p->desc = *desc;

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
		CommandList cmd = g_Device->BeginCommandList();

		graphics_set_pipeline_mode(GraphicsPipelineMode_Graphics, cmd);

		return cmd;
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

	void graphics_shader_bind(Shader& shader_, CommandList cmd)
	{
		Shader_internal* shader = reinterpret_cast<Shader_internal*>(shader_.GetPtr());

		switch (shader->shaderType)
		{
		case ShaderType_Vertex:
			g_PipelineState.graphics[cmd].vertexShader = shader;
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Shader_VS;
			break;
		case ShaderType_Pixel:
			g_PipelineState.graphics[cmd].pixelShader = shader;
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Shader_PS;
			break;
		case ShaderType_Geometry:
			g_PipelineState.graphics[cmd].geometryShader = shader;
			g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Shader_GS;
			break;
		}
		
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Shader;
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

	void graphics_inputlayoutstate_bind(InputLayoutState& inputLayoutState, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].inputLayoutState = reinterpret_cast<InputLayoutState_internal*>(inputLayoutState.GetPtr());
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_InputLayoutState;
	}

	void graphics_blendstate_bind(BlendState& blendState, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].blendState = reinterpret_cast<BlendState_internal*>(blendState.GetPtr());
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_BlendState;
	}

	void graphics_depthstencilstate_bind(DepthStencilState& depthStencilState, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].depthStencilState = reinterpret_cast<DepthStencilState_internal*>(depthStencilState.GetPtr());
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_DepthStencilState;
	}

	void graphics_rasterizerstate_bind(RasterizerState& rasterizerState, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].rasterizerState = reinterpret_cast<RasterizerState_internal*>(rasterizerState.GetPtr());
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RasterizerState;
	}

	void graphics_pipeline_bind(GraphicsPipeline& pipeline, CommandList cmd)
	{
		// Bind shaders
		if (pipeline.pVertexShader) graphics_shader_bind(*pipeline.pVertexShader, cmd);
		if (pipeline.pPixelShader) graphics_shader_bind(*pipeline.pPixelShader, cmd);
		if (pipeline.pGeometryShader) graphics_shader_bind(*pipeline.pGeometryShader, cmd);

		// Bind states
		graphics_inputlayoutstate_bind(pipeline.pInputLayoutState ? *pipeline.pInputLayoutState : g_DefInputLayoutState, cmd);
		graphics_blendstate_bind(pipeline.pBlendState ? *pipeline.pBlendState : g_DefBlendState, cmd);
		graphics_depthstencilstate_bind(pipeline.pDepthStencilState ? *pipeline.pDepthStencilState : g_DefDepthStencilState, cmd);
		graphics_rasterizerstate_bind(pipeline.pRasterizerState ? *pipeline.pRasterizerState : g_DefRasterizerState, cmd);

		graphics_set_pipeline_mode(GraphicsPipelineMode_Graphics, cmd);
		graphics_set_stencil_reference(pipeline.stencilRef, cmd);
		graphics_set_topology(pipeline.topology, cmd);

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

	void graphics_set_topology(GraphicsTopology topology, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].topology = topology;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Topology;
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