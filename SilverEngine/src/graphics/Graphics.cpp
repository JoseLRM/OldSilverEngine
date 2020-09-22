#include "core.h"

#include "graphics_internal.h"
#include "window.h"

#include "vulkan/graphics_vulkan.h"

namespace sv {

	static PipelineState							g_PipelineState;
	static std::unique_ptr<GraphicsDevice>			g_Device;
	
	static std::vector<std::unique_ptr<Adapter>>	g_Adapters;
	static ui32										g_AdapterIndex;

	static bool g_SwapChainImageAcquired = false;

	// Default Primitives

	static GraphicsState		g_DefGraphicsState;

	static InputLayoutState		g_DefInputLayoutState;
	static BlendState			g_DefBlendState;
	static DepthStencilState	g_DefDepthStencilState;
	static RasterizerState		g_DefRasterizerState;

	Result graphics_initialize(const InitializationGraphicsDesc& desc)
	{
		g_Device = std::make_unique<Graphics_vk>();

		// Initialize API
		svCheck(g_Device->Initialize(desc));

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
			desc.cullMode = RasterizerCullMode_None;
			desc.clockwise = true;
			svCheck(graphics_rasterizerstate_create(&desc, g_DefRasterizerState));
		}

		// Graphic State

		g_DefGraphicsState = {};
		g_DefGraphicsState.inputLayoutState = reinterpret_cast<InputLayoutState_internal*>(g_DefInputLayoutState.GetPtr());
		g_DefGraphicsState.blendState = reinterpret_cast<BlendState_internal*>(g_DefBlendState.GetPtr());;
		g_DefGraphicsState.depthStencilState = reinterpret_cast<DepthStencilState_internal*>(g_DefDepthStencilState.GetPtr());;
		g_DefGraphicsState.rasterizerState = reinterpret_cast<RasterizerState_internal*>(g_DefRasterizerState.GetPtr());;
		for (ui32 i = 0; i < SV_GFX_VIEWPORT_COUNT; ++i) {
			g_DefGraphicsState.viewports[i] = { 0.f, 0.f, 100.f, 100.f, 0.f, 1.f };
		}
		g_DefGraphicsState.viewportsCount = SV_GFX_VIEWPORT_COUNT;
		for (ui32 i = 0; i < SV_GFX_SCISSOR_COUNT; ++i) {
			g_DefGraphicsState.scissors[i] = { 0.f, 0.f, 100.f, 100.f };
		}
		g_DefGraphicsState.scissorsCount = SV_GFX_SCISSOR_COUNT;
		g_DefGraphicsState.topology = GraphicsTopology_Triangles;
		g_DefGraphicsState.stencilReference = 0u;
		g_DefGraphicsState.lineWidth = 1.f;
		for (ui32 i = 0; i < SV_GFX_ATTACHMENTS_COUNT; ++i) {
			g_DefGraphicsState.clearColors[i] = { 0.f, 0.f, 0.f, 1.f };
		}
		g_DefGraphicsState.clearDepthStencil.first = 1.f;
		g_DefGraphicsState.clearDepthStencil.second = 0u;
		g_DefGraphicsState.flags =
			GraphicsPipelineState_InputLayoutState |
			GraphicsPipelineState_BlendState |
			GraphicsPipelineState_DepthStencilState |
			GraphicsPipelineState_RasterizerState |
			GraphicsPipelineState_Viewport |
			GraphicsPipelineState_Scissor |
			GraphicsPipelineState_Topology |
			GraphicsPipelineState_StencilRef |
			GraphicsPipelineState_LineWidth |
			GraphicsPipelineState_RenderPass
			;

		svCheck(graphics_shader_initialize());

		return Result_Success;
	}

	Result graphics_close()
	{
		graphics_allocator_clear();

		svCheck(g_Device->Close());

		svCheck(graphics_shader_close());

		return Result_Success;
	}

	void graphics_begin()
	{
		g_SwapChainImageAcquired = false;
		g_Device->BeginFrame();
	}
	void graphics_commandlist_submit()
	{
		g_Device->SubmitCommandLists();
	}
	void graphics_present()
	{
		if (!g_SwapChainImageAcquired) svLogWarning("Must acquire swaphchain image once per frame");
		g_Device->Present();
	}

	void graphics_swapchain_resize()
	{
		g_Device->ResizeSwapChain();
	}

	GPUImage& graphics_swapchain_acquire_image()
	{
		if (g_SwapChainImageAcquired) svLogWarning("Must acquire swapchain image once per frame");
		g_SwapChainImageAcquired = true;
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
			sv::utils_hash_combine(hash, sv::utils_hash_string(element.name));
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

	bool graphics_format_has_stencil(Format format)
	{
		if (format == Format_D24_UNORM_S8_UINT)
			return true;
		else return false;
	}
	constexpr ui32 graphics_format_size(Format format)
	{
		switch (format)
		{
		case Format_R32G32B32A32_FLOAT:
		case Format_R32G32B32A32_UINT:
		case Format_R32G32B32A32_SINT:
			return 16u;

		case Format_R32G32B32_FLOAT:
		case Format_R32G32B32_UINT:
		case Format_R32G32B32_SINT:
			return 12u;
		case Format_R16G16B16A16_FLOAT:
		case Format_R16G16B16A16_UNORM:
		case Format_R16G16B16A16_UINT:
		case Format_R16G16B16A16_SNORM:
		case Format_R16G16B16A16_SINT:
		case Format_R32G32_FLOAT:
		case Format_R32G32_UINT:
		case Format_R32G32_SINT:
			return 8u;

		case Format_R8G8B8A8_UNORM:
		case Format_R8G8B8A8_SRGB:
		case Format_R8G8B8A8_UINT:
		case Format_R8G8B8A8_SNORM:
		case Format_R8G8B8A8_SINT:
		case Format_R16G16_FLOAT:
		case Format_R16G16_UNORM:
		case Format_R16G16_UINT:
		case Format_R16G16_SNORM:
		case Format_R16G16_SINT:
		case Format_D32_FLOAT:
		case Format_R32_FLOAT:
		case Format_R32_UINT:
		case Format_R32_SINT:
		case Format_D24_UNORM_S8_UINT:
		case Format_B8G8R8A8_UNORM:
		case Format_B8G8R8A8_SRGB:
			return 4u;

		case Format_R8G8_UNORM:
		case Format_R8G8_UINT:
		case Format_R8G8_SNORM:
		case Format_R8G8_SINT:
		case Format_R16_FLOAT:
		case Format_D16_UNORM:
		case Format_R16_UNORM:
		case Format_R16_UINT:
		case Format_R16_SNORM:
		case Format_R16_SINT:
			return 2u;

		case Format_R8_UNORM:
		case Format_R8_UINT:
		case Format_R8_SNORM:
		case Format_R8_SINT:
			return 1u;

		case Format_Unknown:
			return 0u;

		case Format_BC1_UNORM:
		case Format_BC1_SRGB:
		case Format_BC2_UNORM:
		case Format_BC2_SRGB:
		case Format_BC3_UNORM:
		case Format_BC3_SRGB:
		case Format_BC4_UNORM:
		case Format_BC4_SNORM:
		case Format_BC5_UNORM:
		case Format_BC5_SNORM:
		default:
			svLog("Unknown format size");
			return 0u;
		}
	}

	GraphicsAPI graphics_api_get()
	{
		return GraphicsAPI_Vulkan;
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

	Result Allocate(Primitive& primitive, GraphicsPrimitiveType type, const void* desc)
	{
		if (primitive.IsValid()) svCheck(graphics_destroy(primitive));
		Primitive_internal* res = reinterpret_cast<Primitive_internal*>(primitive.GetPtr());
		svCheck(graphics_allocator_construct(type, desc, &res));
		primitive = Primitive(res);
		return Result_Success;
	}

	Result graphics_buffer_create(const GPUBufferDesc* desc, GPUBuffer& buffer)
	{
#ifdef SV_DEBUG
		if (desc->usage == ResourceUsage_Static && desc->CPUAccess & CPUAccess_Write) {
			SV_THROW("GRAPHICS_ERROR", "Buffer with static usage can't have CPU access");
			return Result_InvalidUsage;
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

		return Result_Success;
	}

	Result graphics_shader_create(const ShaderDesc* desc, Shader& shader)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(shader, GraphicsPrimitiveType_Shader, desc));

		Shader_internal* p = reinterpret_cast<Shader_internal*>(shader.GetPtr());
		p->type = GraphicsPrimitiveType_Shader;
		p->shaderType = desc->shaderType;

		return Result_Success;
	}

	Result graphics_image_create(const GPUImageDesc* desc, GPUImage& image)
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

		return Result_Success;
	}

	Result graphics_sampler_create(const SamplerDesc* desc, Sampler& sampler)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(sampler, GraphicsPrimitiveType_Sampler, desc));

		Sampler_internal* p = reinterpret_cast<Sampler_internal*>(sampler.GetPtr());
		p->type = GraphicsPrimitiveType_Sampler;

		return Result_Success;
	}

	Result graphics_renderpass_create(const RenderPassDesc* desc, RenderPass& renderPass)
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

		return Result_Success;
	}

	Result graphics_inputlayoutstate_create(const InputLayoutStateDesc* desc, InputLayoutState& inputLayoutState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(inputLayoutState, GraphicsPrimitiveType_InputLayoutState, desc));

		InputLayoutState_internal* p = reinterpret_cast<InputLayoutState_internal*>(inputLayoutState.GetPtr());
		p->type = GraphicsPrimitiveType_InputLayoutState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_blendstate_create(const BlendStateDesc* desc, BlendState& blendState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(blendState, GraphicsPrimitiveType_BlendState, desc));

		BlendState_internal* p = reinterpret_cast<BlendState_internal*>(blendState.GetPtr());
		p->type = GraphicsPrimitiveType_BlendState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_depthstencilstate_create(const DepthStencilStateDesc* desc, DepthStencilState& depthStencilState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(depthStencilState, GraphicsPrimitiveType_DepthStencilState, desc));

		DepthStencilState_internal* p = reinterpret_cast<DepthStencilState_internal*>(depthStencilState.GetPtr());
		p->type = GraphicsPrimitiveType_DepthStencilState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_rasterizerstate_create(const RasterizerStateDesc* desc, RasterizerState& rasterizerState)
	{
#ifdef SV_DEBUG
#endif

		svCheck(Allocate(rasterizerState, GraphicsPrimitiveType_RasterizerState, desc));

		RasterizerState_internal* p = reinterpret_cast<RasterizerState_internal*>(rasterizerState.GetPtr());
		p->type = GraphicsPrimitiveType_RasterizerState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_destroy(Primitive& primitive)
	{
		if (primitive.IsValid()) {
			Result res = graphics_allocator_destroy(primitive);
			primitive = Primitive(nullptr);
			return res;
		}
		return Result_Success;
	}

	CommandList graphics_commandlist_begin()
	{
		CommandList cmd = g_Device->BeginCommandList();

		g_PipelineState.mode[cmd] = GraphicsPipelineMode_Graphics;
		g_PipelineState.graphics[cmd] = g_DefGraphicsState;

		return cmd;
	}

	CommandList graphics_commandlist_last()
	{
		return g_Device->GetLastCommandList();
	}

	CommandList graphics_commandlist_get()
	{
		return (graphics_commandlist_count() != 0u) ? graphics_commandlist_last() : graphics_commandlist_begin();
	}

	ui32 graphics_commandlist_count()
	{
		return g_Device->GetCommandListCount();
	}

	void graphics_gpu_wait()
	{
		g_Device->WaitGPU();
	}

	/////////////////////////////////////// RESOURCES ////////////////////////////////////////////////////////////

	constexpr GraphicsPipelineState graphics_pipelinestateflags_constantbuffer(ShaderType shaderType)
	{
		switch (shaderType)
		{
		case ShaderType_Vertex:
			return GraphicsPipelineState_ConstantBuffer_VS;
		case ShaderType_Pixel:
			return GraphicsPipelineState_ConstantBuffer_PS;
		case ShaderType_Geometry:
			return GraphicsPipelineState_ConstantBuffer_GS;
		case ShaderType_Hull:
			return GraphicsPipelineState_ConstantBuffer_HS;
		case ShaderType_Domain:
			return GraphicsPipelineState_ConstantBuffer_DS;
		}
	}

	constexpr GraphicsPipelineState graphics_pipelinestateflags_image(ShaderType shaderType)
	{
		switch (shaderType)
		{
		case ShaderType_Vertex:
			return GraphicsPipelineState_Image_VS;
		case ShaderType_Pixel:
			return GraphicsPipelineState_Image_PS;
		case ShaderType_Geometry:
			return GraphicsPipelineState_Image_GS;
		case ShaderType_Hull:
			return GraphicsPipelineState_Image_HS;
		case ShaderType_Domain:
			return GraphicsPipelineState_Image_DS;
		}
	}

	constexpr GraphicsPipelineState graphics_pipelinestateflags_sampler(ShaderType shaderType)
	{
		switch (shaderType)
		{
		case ShaderType_Vertex:
			return GraphicsPipelineState_Sampler_VS;
		case ShaderType_Pixel:
			return GraphicsPipelineState_Sampler_PS;
		case ShaderType_Geometry:
			return GraphicsPipelineState_Sampler_GS;
		case ShaderType_Hull:
			return GraphicsPipelineState_Sampler_HS;
		case ShaderType_Domain:
			return GraphicsPipelineState_Sampler_DS;
		}
	}

	void graphics_resources_unbind(CommandList cmd)
	{
		graphics_vertexbuffer_unbind(cmd);
		graphics_indexbuffer_unbind(cmd);
		graphics_constantbuffer_unbind(cmd);
		graphics_image_unbind(cmd);
		graphics_sampler_unbind(cmd);
	}

	void graphics_vertexbuffer_bind(GPUBuffer** buffers, ui32* offsets, ui32 count, ui32 beginSlot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, beginSlot + count);

		GPUBuffer_internal** vBuffers = state.vertexBuffers + beginSlot;
		GPUBuffer_internal** vBuffersEnd = vBuffers + count;

		GPUBuffer** buf = buffers;
		GPUBuffer** bufEnd = buf + count;

		while (vBuffers != vBuffersEnd) {
			*vBuffers = reinterpret_cast<GPUBuffer_internal*>((*buf)->GetPtr());

			++vBuffers;
			++buf;
		}
		memcpy(state.vertexBufferOffsets + beginSlot, offsets, sizeof(ui32) * count);
		state.flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_bind(GPUBuffer& buffer, ui32 offset, ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, slot + 1u);
		state.vertexBuffers[slot] = reinterpret_cast<GPUBuffer_internal*>(buffer.GetPtr());
		state.vertexBufferOffsets[slot] = offset;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_unbind(ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffers[slot] = nullptr;

		// Compute Vertex Buffer Count
		for (i32 i = i32(state.vertexBuffersCount) - 1; i >= 0; --i) {
			if (state.vertexBuffers[i] != nullptr) {
				state.vertexBuffersCount = i + 1;
				break;
			}
		}

		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.vertexBuffers, state.vertexBuffersCount * sizeof(GPUBuffer_internal*));
		state.vertexBuffersCount = 0u;

		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_indexbuffer_bind(GPUBuffer& buffer, ui32 offset, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.indexBuffer = reinterpret_cast<GPUBuffer_internal*>(buffer.GetPtr());
		state.indexBufferOffset = offset;
		state.flags |= GraphicsPipelineState_IndexBuffer;
	}

	void graphics_indexbuffer_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.indexBuffer = nullptr;
		state.flags |= GraphicsPipelineState_IndexBuffer;
	}

	void graphics_constantbuffer_bind(GPUBuffer** buffers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffersCount[shaderType] = std::max(state.constantBuffersCount[shaderType], beginSlot + count);

		GPUBuffer_internal** cBuffers = state.constantBuffers[shaderType] + beginSlot;
		GPUBuffer_internal** cBuffersEnd = cBuffers + count;

		GPUBuffer** buf = buffers;
		GPUBuffer** bufEnd = buf + count;

		while (cBuffers != cBuffersEnd) {
			*cBuffers = reinterpret_cast<GPUBuffer_internal*>((*buf)->GetPtr());

			++cBuffers;
			++buf;
		}
		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_bind(GPUBuffer& buffer, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffers[shaderType][slot] = reinterpret_cast<GPUBuffer_internal*>(buffer.GetPtr());
		state.constantBuffersCount[shaderType] = std::max(state.constantBuffersCount[shaderType], slot + 1u);

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind(ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffers[shaderType][slot] = nullptr;

		// Compute Constant Buffer Count
		ui32& count = state.constantBuffersCount[shaderType];

		for (i32 i = i32(count) - 1; i >= 0; --i) {
			if (state.constantBuffers[shaderType][i] != nullptr) {
				count = i + 1;
				break;
			}
		}

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.constantBuffers[shaderType], state.constantBuffersCount[shaderType] * sizeof(GPUBuffer_internal*));
		state.constantBuffersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.constantBuffersCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_constantbuffer((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_ConstantBuffer;
	}

	void graphics_image_bind(GPUImage** images, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.imagesCount[shaderType] = std::max(state.imagesCount[shaderType], beginSlot + count);

		GPUImage_internal** imagesIt = state.images[shaderType] + beginSlot;
		GPUImage_internal** imagesEnd = imagesIt + count;

		GPUImage** buf = images;
		GPUImage** bufEnd = buf + count;

		while (imagesIt != imagesEnd) {
			*imagesIt = reinterpret_cast<GPUImage_internal*>((*buf)->GetPtr());

			++imagesIt;
			++buf;
		}
		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_bind(GPUImage& image, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.images[shaderType][slot] = reinterpret_cast<GPUImage_internal*>(image.GetPtr());
		state.imagesCount[shaderType] = std::max(state.imagesCount[shaderType], slot + 1u);
		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_unbind(ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.images[shaderType][slot] = nullptr;

		// Compute Images Count
		ui32& count = state.imagesCount[shaderType];

		for (i32 i = i32(count) - 1; i >= 0; --i) {
			if (state.images[shaderType][i] != nullptr) {
				count = i + 1;
				break;
			}
		}

		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_unbind(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.images[shaderType], state.imagesCount[shaderType] * sizeof(GPUImage_internal*));
		state.imagesCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}
	
	void graphics_image_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.imagesCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_image((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_Image;
	}

	void graphics_sampler_bind(Sampler** samplers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplersCount[shaderType] = std::max(state.samplersCount[shaderType], beginSlot + count);

		Sampler_internal** samplersIt = state.samplers[shaderType] + beginSlot;
		Sampler_internal** samplersEnd = samplersIt + count;

		Sampler** buf = samplers;
		Sampler** bufEnd = buf + count;

		while (samplersIt != samplersEnd) {
			*samplersIt = reinterpret_cast<Sampler_internal*>((*buf)->GetPtr());

			++samplersIt;
			++buf;
		}
		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_bind(Sampler& sampler, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplers[shaderType][slot] = reinterpret_cast<Sampler_internal*>(sampler.GetPtr());
		state.samplersCount[shaderType] = std::max(state.samplersCount[shaderType], slot + 1u);
		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind(ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplers[shaderType][slot] = nullptr;

		// Compute Images Count
		ui32& count = state.samplersCount[shaderType];

		for (i32 i = i32(count) - 1; i >= 0; --i) {
			if (state.samplers[shaderType][i] != nullptr) {
				count = i + 1;
				break;
			}
		}

		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.samplers[shaderType], state.samplersCount[shaderType] * sizeof(Sampler_internal*));
		state.samplersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.samplersCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_sampler((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_Sampler;
	}

	////////////////////////////////////////////// STATE //////////////////////////////////////////////////

	void graphics_state_unbind(CommandList cmd)
	{
		if (g_PipelineState.graphics[cmd].renderPass) graphics_renderpass_end(cmd);
		
		graphics_shader_unbind(cmd);
		graphics_inputlayoutstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_rasterizerstate_unbind(cmd);

		graphics_stencil_reference_set(0u, cmd);
		graphics_line_width_set(1.f, cmd);
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

	void graphics_shader_unbind(CommandList cmd)
	{
		// TODO: Default shaders
	}

	void graphics_inputlayoutstate_unbind(CommandList cmd)
	{
		graphics_inputlayoutstate_bind(g_DefInputLayoutState, cmd);
	}

	void graphics_blendstate_unbind(CommandList cmd)
	{
		graphics_blendstate_bind(g_DefBlendState, cmd);
	}

	void graphics_depthstencilstate_unbind(CommandList cmd)
	{
		graphics_depthstencilstate_bind(g_DefDepthStencilState, cmd);
	}

	void graphics_rasterizerstate_unbind(CommandList cmd)
	{
		graphics_rasterizerstate_bind(g_DefRasterizerState, cmd);
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

		graphics_mode_set(GraphicsPipelineMode_Graphics, cmd);
		graphics_stencil_reference_set(pipeline.stencilRef, cmd);
		graphics_topology_set(pipeline.topology, cmd);
		graphics_line_width_set(pipeline.lineWidth, cmd);
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
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, rp->attachments.size() * sizeof(vec4f));
		g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);

		g_Device->BeginRenderPass(cmd);
	}
	void graphics_renderpass_end(CommandList cmd)
	{
		g_Device->EndRenderPass(cmd);
		g_PipelineState.graphics[cmd].renderPass = nullptr;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;
	}

	void graphics_mode_set(GraphicsPipelineMode mode, CommandList cmd)
	{
		g_PipelineState.mode[cmd] = mode;
	}

	void graphics_viewport_set(const Viewport* viewports, ui32 count, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		SV_ASSERT(count < SV_GFX_VIEWPORT_COUNT);
		memcpy(state.viewports, viewports, size_t(count) * sizeof(Viewport));
		state.viewportsCount = count;
		state.flags |= GraphicsPipelineState_Viewport;
	}

	void graphics_viewport_set(const Viewport& viewport, ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.viewports[slot] = viewport;
		state.viewportsCount = std::max(g_PipelineState.graphics[cmd].viewportsCount, slot + 1u);
		state.flags |= GraphicsPipelineState_Viewport;
	}

	void graphics_viewport_set(const GPUImage& image, ui32 slot, CommandList cmd)
	{
		graphics_viewport_set(graphics_image_get_viewport(image), slot, cmd);
	}

	void graphics_scissor_set(const Scissor* scissors, ui32 count, CommandList cmd)
	{
		SV_ASSERT(count < SV_GFX_SCISSOR_COUNT);
		memcpy(g_PipelineState.graphics[cmd].scissors, scissors, size_t(count) * sizeof(Scissor));
		g_PipelineState.graphics[cmd].scissorsCount = count;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(const Scissor& scissor, ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.scissors[slot] = scissor;
		state.scissorsCount = std::max(g_PipelineState.graphics[cmd].scissorsCount, slot + 1u);
		state.flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(const GPUImage& image, ui32 slot, CommandList cmd)
	{
		graphics_scissor_set(graphics_image_get_scissor(image), slot, cmd);
	}

	Viewport graphics_viewport_get(ui32 slot, CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].viewports[slot];
	}

	Scissor graphics_scissor_get(ui32 slot, CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].scissors[slot];
	}

	void graphics_topology_set(GraphicsTopology topology, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].topology = topology;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_Topology;
	}

	void graphics_stencil_reference_set(ui32 ref, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].stencilReference = ref;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_StencilRef;
	}

	void graphics_line_width_set(float lineWidth, CommandList cmd)
	{
		g_PipelineState.graphics[cmd].lineWidth = lineWidth;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_LineWidth;
	}

	GraphicsPipelineMode graphics_mode_get(CommandList cmd)
	{
		return g_PipelineState.mode[cmd];
	}

	GraphicsTopology graphics_topology_get(CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].topology;
	}

	ui32 graphics_stencil_reference_get(CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].stencilReference;
	}

	float graphics_line_width_get(CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].lineWidth;
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

	void graphics_image_blit(GPUImage& src, GPUImage& dst, GPUImageLayout srcLayout, GPUImageLayout dstLayout, ui32 count, const GPUImageBlit* imageBlit, SamplerFilter filter, CommandList cmd)
	{
		g_Device->ImageBlit(src, dst, srcLayout, dstLayout, count, imageBlit, filter, cmd);
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