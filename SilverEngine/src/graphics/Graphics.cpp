#include "core.h"

#include "graphics_internal.h"
#include "window.h"

#include "vulkan/graphics_vulkan.h"

namespace sv {

	static PipelineState	g_PipelineState;
	static GraphicsDevice	g_Device;
	
	static std::vector<std::unique_ptr<Adapter>>	g_Adapters;
	static ui32										g_AdapterIndex;

	static bool g_SwapChainImageAcquired = false;

	// Default Primitives

	static GraphicsState		g_DefGraphicsState;

	static InputLayoutState*	g_DefInputLayoutState;
	static BlendState*			g_DefBlendState;
	static DepthStencilState*	g_DefDepthStencilState;
	static RasterizerState*		g_DefRasterizerState;

	Result graphics_initialize(const InitializationGraphicsDesc& desc)
	{
		Result res;

		// Initialize API
		svLog("Trying to initialize vulkan device");
		graphics_vulkan_device_prepare(g_Device);
		res = g_Device.initialize();
		
		if (res != Result_Success) {
			svLogError("Can't initialize vulkan device (Error code: %u)", res);
		}
		else svLog("Vulkan device initialized successfuly");

		// Create default states
		{
			InputLayoutStateDesc desc;
			svCheck(graphics_inputlayoutstate_create(&desc, &g_DefInputLayoutState));
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
			svCheck(graphics_blendstate_create(&desc, &g_DefBlendState));
		}
		{
			DepthStencilStateDesc desc;
			desc.depthTestEnabled = false;
			desc.depthWriteEnabled = false;
			desc.depthCompareOp = CompareOperation_Always;
			desc.stencilTestEnabled = false;
			desc.readMask = 0xFF;
			desc.writeMask = 0xFF;
			svCheck(graphics_depthstencilstate_create(&desc, &g_DefDepthStencilState));
		}
		{
			RasterizerStateDesc desc;
			desc.wireframe = false;
			desc.cullMode = RasterizerCullMode_None;
			desc.clockwise = true;
			svCheck(graphics_rasterizerstate_create(&desc, &g_DefRasterizerState));
		}

		// Graphic State
		g_DefGraphicsState = {};
		g_DefGraphicsState.inputLayoutState = reinterpret_cast<InputLayoutState_internal*>(g_DefInputLayoutState);
		g_DefGraphicsState.blendState = reinterpret_cast<BlendState_internal*>(g_DefBlendState);;
		g_DefGraphicsState.depthStencilState = reinterpret_cast<DepthStencilState_internal*>(g_DefDepthStencilState);;
		g_DefGraphicsState.rasterizerState = reinterpret_cast<RasterizerState_internal*>(g_DefRasterizerState);;
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
		svCheck(graphics_destroy(g_DefBlendState));
		svCheck(graphics_destroy(g_DefDepthStencilState));
		svCheck(graphics_destroy(g_DefInputLayoutState));
		svCheck(graphics_destroy(g_DefRasterizerState));

		if (g_Device.bufferAllocator.get()) {

			std::lock_guard<std::mutex> lock0(g_Device.bufferMutex);
			std::lock_guard<std::mutex> lock1(g_Device.imageMutex);
			std::lock_guard<std::mutex> lock2(g_Device.samplerMutex);
			std::lock_guard<std::mutex> lock3(g_Device.shaderMutex);
			std::lock_guard<std::mutex> lock4(g_Device.renderPassMutex);
			std::lock_guard<std::mutex> lock5(g_Device.inputLayoutStateMutex);
			std::lock_guard<std::mutex> lock6(g_Device.blendStateMutex);
			std::lock_guard<std::mutex> lock7(g_Device.depthStencilStateMutex);
			std::lock_guard<std::mutex> lock8(g_Device.rasterizerStateMutex);

			ui32 count;

			count = g_Device.bufferAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed buffers", count);
			
			count = g_Device.imageAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed images", count);

			count = g_Device.samplerAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed samplers", count);

			count = g_Device.shaderAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed shaders", count);

			count = g_Device.renderPassAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed render passes", count);

			count = g_Device.inputLayoutStateAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed input layout states", count);

			count = g_Device.blendStateAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed blend states", count);

			count = g_Device.depthStencilStateAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed depth stencil states", count);

			count = g_Device.rasterizerStateAllocator->unfreed_count();
			if (count) svLogWarning("There are %u unfreed rasterizer states", count);

			g_Device.bufferAllocator->clear();
			g_Device.imageAllocator->clear();
			g_Device.samplerAllocator->clear();
			g_Device.shaderAllocator->clear();
			g_Device.inputLayoutStateAllocator->clear();
			g_Device.blendStateAllocator->clear();
			g_Device.depthStencilStateAllocator->clear();
			g_Device.rasterizerStateAllocator->clear();

		}

		svCheck(g_Device.close());

		svCheck(graphics_shader_close());

		return Result_Success;
	}

	void graphics_begin()
	{
		g_SwapChainImageAcquired = false;
		g_Device.frame_begin();
	}
	void graphics_commandlist_submit()
	{
		g_Device.commandlist_submit();
	}
	void graphics_present()
	{
		if (!g_SwapChainImageAcquired) svLogWarning("Must acquire swaphchain image once per frame");
		g_Device.present();
	}

	void graphics_swapchain_resize()
	{
		g_Device.swapchain_resize();
	}

	GPUImage* graphics_swapchain_acquire_image()
	{
		if (g_SwapChainImageAcquired) svLogWarning("Must acquire swapchain image once per frame");
		g_SwapChainImageAcquired = true;
		return g_Device.swapchain_acquire_image();
	}

	PipelineState& graphics_state_get() noexcept
	{
		return g_PipelineState;
	}
	void* graphics_device_get() noexcept
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

	Result graphics_buffer_create(const GPUBufferDesc* desc, GPUBuffer** buffer)
	{
#ifdef SV_DEBUG
		if (desc->usage == ResourceUsage_Static && desc->CPUAccess & CPUAccess_Write) {
			SV_THROW("GRAPHICS_ERROR", "Buffer with static usage can't have CPU access");
			return Result_InvalidUsage;
		}
#endif
		
		// Allocate memory
		g_Device.bufferMutex.lock();
		*buffer = (GPUBuffer*)g_Device.bufferAllocator->alloc();
		g_Device.bufferMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Buffer, desc, (Primitive_internal*)*buffer));

		// Set parameters
		GPUBuffer_internal* p = reinterpret_cast<GPUBuffer_internal*>(*buffer);
		p->type = GraphicsPrimitiveType_Buffer;
		p->bufferType = desc->bufferType;
		p->usage = desc->usage;
		p->size = desc->size;
		p->indexType = desc->indexType;
		p->cpuAccess = desc->CPUAccess;

		return Result_Success;
	}

	Result graphics_shader_create(const ShaderDesc* desc, Shader** shader)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.shaderMutex.lock();
		*shader = (Shader*)g_Device.shaderAllocator->alloc();
		g_Device.shaderMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Shader, desc, (Primitive_internal*)* shader));

		// Set parameters
		Shader_internal* p = reinterpret_cast<Shader_internal*>(*shader);
		p->type = GraphicsPrimitiveType_Shader;
		p->shaderType = desc->shaderType;

		return Result_Success;
	}

	Result graphics_image_create(const GPUImageDesc* desc, GPUImage** image)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.imageMutex.lock();
		*image = (GPUImage*)g_Device.imageAllocator->alloc();
		g_Device.imageMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Image, desc, (Primitive_internal*)* image));

		// Set parameters
		GPUImage_internal* p = reinterpret_cast<GPUImage_internal*>(*image);
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

	Result graphics_sampler_create(const SamplerDesc* desc, Sampler** sampler)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.samplerMutex.lock();
		*sampler = (Sampler*)g_Device.samplerAllocator->alloc();
		g_Device.samplerMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Sampler, desc, (Primitive_internal*)* sampler));

		// Set parameters
		Sampler_internal* p = reinterpret_cast<Sampler_internal*>(*sampler);
		p->type = GraphicsPrimitiveType_Sampler;

		return Result_Success;
	}

	Result graphics_renderpass_create(const RenderPassDesc* desc, RenderPass** renderPass)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.renderPassMutex.lock();
		*renderPass = (RenderPass*)g_Device.renderPassAllocator->alloc();
		g_Device.renderPassMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_RenderPass, desc, (Primitive_internal*)* renderPass));

		// Set parameters
		RenderPass_internal* p = reinterpret_cast<RenderPass_internal*>(*renderPass);
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

	Result graphics_inputlayoutstate_create(const InputLayoutStateDesc* desc, InputLayoutState** inputLayoutState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.inputLayoutStateMutex.lock();
		*inputLayoutState = (InputLayoutState*)g_Device.inputLayoutStateAllocator->alloc();
		g_Device.inputLayoutStateMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_InputLayoutState, desc, (Primitive_internal*)* inputLayoutState));

		// Set parameters
		InputLayoutState_internal* p = reinterpret_cast<InputLayoutState_internal*>(*inputLayoutState);
		p->type = GraphicsPrimitiveType_InputLayoutState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_blendstate_create(const BlendStateDesc* desc, BlendState** blendState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.blendStateMutex.lock();
		*blendState = (BlendState*)g_Device.blendStateAllocator->alloc();
		g_Device.blendStateMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_BlendState, desc, (Primitive_internal*)* blendState));

		// Set parameters
		BlendState_internal* p = reinterpret_cast<BlendState_internal*>(*blendState);
		p->type = GraphicsPrimitiveType_BlendState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_depthstencilstate_create(const DepthStencilStateDesc* desc, DepthStencilState** depthStencilState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.depthStencilStateMutex.lock();
		*depthStencilState = (DepthStencilState*)g_Device.depthStencilStateAllocator->alloc();
		g_Device.depthStencilStateMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_DepthStencilState, desc, (Primitive_internal*)* depthStencilState));

		// Set parameters
		DepthStencilState_internal* p = reinterpret_cast<DepthStencilState_internal*>(*depthStencilState);
		p->type = GraphicsPrimitiveType_DepthStencilState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_rasterizerstate_create(const RasterizerStateDesc* desc, RasterizerState** rasterizerState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		g_Device.rasterizerStateMutex.lock();
		*rasterizerState = (RasterizerState*)g_Device.rasterizerStateAllocator->alloc();
		g_Device.rasterizerStateMutex.unlock();

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_DepthStencilState, desc, (Primitive_internal*)* rasterizerState));

		// Set parameters
		RasterizerState_internal* p = reinterpret_cast<RasterizerState_internal*>(*rasterizerState);
		p->type = GraphicsPrimitiveType_RasterizerState;
		p->desc = *desc;

		return Result_Success;
	}

	Result graphics_destroy(Primitive* primitive)
	{
		if (primitive == nullptr) return Result_Success;

		Primitive_internal* p = reinterpret_cast<Primitive_internal*>(primitive);
		Result res = g_Device.destroy(p);
		
		switch (p->type)
		{
		case GraphicsPrimitiveType_Image:
			g_Device.imageMutex.lock();
			g_Device.imageAllocator->free(p);
			g_Device.imageMutex.unlock();
			break;
		case GraphicsPrimitiveType_Sampler:
			g_Device.samplerMutex.lock();
			g_Device.samplerAllocator->free(p);
			g_Device.samplerMutex.unlock();
			break;
		case GraphicsPrimitiveType_Buffer:
			g_Device.bufferMutex.lock();
			g_Device.bufferAllocator->free(p);
			g_Device.bufferMutex.unlock();
			break;
		case GraphicsPrimitiveType_Shader:
			g_Device.shaderMutex.lock();
			g_Device.shaderAllocator->free(p);
			g_Device.shaderMutex.unlock();
			break;
		case GraphicsPrimitiveType_RenderPass:
			g_Device.renderPassMutex.lock();
			g_Device.renderPassAllocator->free(p);
			g_Device.renderPassMutex.unlock();
			break;
		case GraphicsPrimitiveType_InputLayoutState:
			g_Device.inputLayoutStateMutex.lock();
			g_Device.inputLayoutStateAllocator->free(p);
			g_Device.inputLayoutStateMutex.unlock();
			break;
		case GraphicsPrimitiveType_BlendState:
			g_Device.blendStateMutex.lock();
			g_Device.blendStateAllocator->free(p);
			g_Device.blendStateMutex.unlock();
			break;
		case GraphicsPrimitiveType_DepthStencilState:
			g_Device.depthStencilStateMutex.lock();
			g_Device.depthStencilStateAllocator->free(p);
			g_Device.depthStencilStateMutex.unlock();
			break;
		case GraphicsPrimitiveType_RasterizerState:
			g_Device.rasterizerStateMutex.lock();
			g_Device.rasterizerStateAllocator->free(p);
			g_Device.rasterizerStateMutex.unlock();
			break;
		}

		return Result_Success;
	}

	CommandList graphics_commandlist_begin()
	{
		CommandList cmd = g_Device.commandlist_begin();

		g_PipelineState.mode[cmd] = GraphicsPipelineMode_Graphics;
		g_PipelineState.graphics[cmd] = g_DefGraphicsState;

		return cmd;
	}

	CommandList graphics_commandlist_last()
	{
		return g_Device.commandlist_last();
	}

	CommandList graphics_commandlist_get()
	{
		return (graphics_commandlist_count() != 0u) ? graphics_commandlist_last() : graphics_commandlist_begin();
	}

	ui32 graphics_commandlist_count()
	{
		return g_Device.commandlist_count();
	}

	void graphics_gpu_wait()
	{
		g_Device.gpu_wait();
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
		graphics_vertexbuffer_unbind_commandlist(cmd);
		graphics_indexbuffer_unbind(cmd);
		graphics_constantbuffer_unbind_commandlist(cmd);
		graphics_image_unbind_commandlist(cmd);
		graphics_sampler_unbind_commandlist(cmd);
	}

	void graphics_vertexbuffer_bind_array(GPUBuffer** buffers, ui32* offsets, ui32 count, ui32 beginSlot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, beginSlot + count);

		memcpy(state.vertexBuffers + beginSlot, buffers, sizeof(GPUBuffer*) * count);
		memcpy(state.vertexBufferOffsets + beginSlot, offsets, sizeof(ui32) * count);
		state.flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_bind(GPUBuffer* buffer, ui32 offset, ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, slot + 1u);
		state.vertexBuffers[slot] = reinterpret_cast<GPUBuffer_internal*>(buffer);
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

	void graphics_vertexbuffer_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.vertexBuffers, state.vertexBuffersCount * sizeof(GPUBuffer_internal*));
		state.vertexBuffersCount = 0u;

		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_indexbuffer_bind(GPUBuffer* buffer, ui32 offset, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.indexBuffer = reinterpret_cast<GPUBuffer_internal*>(buffer);
		state.indexBufferOffset = offset;
		state.flags |= GraphicsPipelineState_IndexBuffer;
	}

	void graphics_indexbuffer_unbind(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.indexBuffer = nullptr;
		state.flags |= GraphicsPipelineState_IndexBuffer;
	}

	void graphics_constantbuffer_bind_array(GPUBuffer** buffers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffersCount[shaderType] = std::max(state.constantBuffersCount[shaderType], beginSlot + count);

		memcpy(state.constantBuffers, buffers, sizeof(GPUBuffer*) * count);
		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_bind(GPUBuffer* buffer, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffers[shaderType][slot] = reinterpret_cast<GPUBuffer_internal*>(buffer);
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

	void graphics_constantbuffer_unbind_shader(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.constantBuffers[shaderType], state.constantBuffersCount[shaderType] * sizeof(GPUBuffer_internal*));
		state.constantBuffersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.constantBuffersCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_constantbuffer((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_ConstantBuffer;
	}

	void graphics_image_bind_array(GPUImage** images, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.imagesCount[shaderType] = std::max(state.imagesCount[shaderType], beginSlot + count);

		memcpy(state.images, images, sizeof(GPUImage*) * count);
		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_bind(GPUImage* image, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.images[shaderType][slot] = reinterpret_cast<GPUImage_internal*>(image);
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

	void graphics_image_unbind_shader(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.images[shaderType], state.imagesCount[shaderType] * sizeof(GPUImage_internal*));
		state.imagesCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}
	
	void graphics_image_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (ui32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.imagesCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_image((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_Image;
	}

	void graphics_sampler_bind_array(Sampler** samplers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplersCount[shaderType] = std::max(state.samplersCount[shaderType], beginSlot + count);

		memcpy(state.samplers, samplers, sizeof(Sampler*) * count);
		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_bind(Sampler* sampler, ui32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplers[shaderType][slot] = reinterpret_cast<Sampler_internal*>(sampler);
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

	void graphics_sampler_unbind_shader(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		svZeroMemory(state.samplers[shaderType], state.samplersCount[shaderType] * sizeof(Sampler_internal*));
		state.samplersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind_commandlist(CommandList cmd)
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
		
		graphics_shader_unbind_commandlist(cmd);
		graphics_inputlayoutstate_unbind(cmd);
		graphics_blendstate_unbind(cmd);
		graphics_depthstencilstate_unbind(cmd);
		graphics_rasterizerstate_unbind(cmd);

		graphics_stencil_reference_set(0u, cmd);
		graphics_line_width_set(1.f, cmd);
	}

	void graphics_shader_bind(Shader* shader_, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		Shader_internal* shader = reinterpret_cast<Shader_internal*>(shader_);

		switch (shader->shaderType)
		{
		case ShaderType_Vertex:
			state.vertexShader = shader;
			state.flags |= GraphicsPipelineState_Shader_VS;
			break;
		case ShaderType_Pixel:
			state.pixelShader = shader;
			state.flags |= GraphicsPipelineState_Shader_PS;
			break;
		case ShaderType_Geometry:
			state.geometryShader = shader;
			state.flags |= GraphicsPipelineState_Shader_GS;
			break;
		}

		state.flags |= GraphicsPipelineState_Shader;
	}

	void graphics_inputlayoutstate_bind(InputLayoutState* inputLayoutState, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.inputLayoutState = reinterpret_cast<InputLayoutState_internal*>(inputLayoutState);
		state.flags |= GraphicsPipelineState_InputLayoutState;
	}

	void graphics_blendstate_bind(BlendState* blendState, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.blendState = reinterpret_cast<BlendState_internal*>(blendState);
		state.flags |= GraphicsPipelineState_BlendState;
	}

	void graphics_depthstencilstate_bind(DepthStencilState* depthStencilState, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.depthStencilState = reinterpret_cast<DepthStencilState_internal*>(depthStencilState);
		state.flags |= GraphicsPipelineState_DepthStencilState;
	}

	void graphics_rasterizerstate_bind(RasterizerState* rasterizerState, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.rasterizerState = reinterpret_cast<RasterizerState_internal*>(rasterizerState);
		state.flags |= GraphicsPipelineState_RasterizerState;
	}

	void graphics_shader_unbind(ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		switch (shaderType)
		{
		case ShaderType_Vertex:
			state.vertexShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_VS;
			break;
		case ShaderType_Pixel:
			state.pixelShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_PS;
			break;
		case ShaderType_Geometry:
			state.geometryShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_GS;
			break;
		}

		state.flags |= GraphicsPipelineState_Shader;
	}

	void graphics_shader_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		if (state.vertexShader) {
			state.vertexShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_VS;
		}
		if (state.pixelShader) {
			state.pixelShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_PS;
		}
		if (state.geometryShader) {
			state.geometryShader = nullptr;
			state.flags |= GraphicsPipelineState_Shader_GS;
		}

		state.flags |= GraphicsPipelineState_Shader;
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
		if (pipeline.pVertexShader) graphics_shader_bind(pipeline.pVertexShader, cmd);
		if (pipeline.pPixelShader) graphics_shader_bind(pipeline.pPixelShader, cmd);
		if (pipeline.pGeometryShader) graphics_shader_bind(pipeline.pGeometryShader, cmd);

		// Bind states
		graphics_inputlayoutstate_bind(pipeline.pInputLayoutState ? pipeline.pInputLayoutState : g_DefInputLayoutState, cmd);
		graphics_blendstate_bind(pipeline.pBlendState ? pipeline.pBlendState : g_DefBlendState, cmd);
		graphics_depthstencilstate_bind(pipeline.pDepthStencilState ? pipeline.pDepthStencilState : g_DefDepthStencilState, cmd);
		graphics_rasterizerstate_bind(pipeline.pRasterizerState ? pipeline.pRasterizerState : g_DefRasterizerState, cmd);

		graphics_mode_set(GraphicsPipelineMode_Graphics, cmd);
		graphics_stencil_reference_set(pipeline.stencilRef, cmd);
		graphics_topology_set(pipeline.topology, cmd);
		graphics_line_width_set(pipeline.lineWidth, cmd);
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

	void graphics_viewport_set(GPUImage* image, ui32 slot, CommandList cmd)
	{
		graphics_viewport_set(graphics_image_get_viewport(image), slot, cmd);
	}

	void graphics_scissor_set(const Scissor* scissors, ui32 count, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		SV_ASSERT(count < SV_GFX_SCISSOR_COUNT);
		memcpy(state.scissors, scissors, size_t(count) * sizeof(Scissor));
		state.scissorsCount = count;
		state.flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(const Scissor& scissor, ui32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.scissors[slot] = scissor;
		state.scissorsCount = std::max(g_PipelineState.graphics[cmd].scissorsCount, slot + 1u);
		state.flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(GPUImage* image, ui32 slot, CommandList cmd)
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
		auto& state = g_PipelineState.graphics[cmd];
		state.topology = topology;
		state.flags |= GraphicsPipelineState_Topology;
	}

	void graphics_stencil_reference_set(ui32 ref, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.stencilReference = ref;
		state.flags |= GraphicsPipelineState_StencilRef;
	}

	void graphics_line_width_set(float lineWidth, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.lineWidth = lineWidth;
		state.flags |= GraphicsPipelineState_LineWidth;
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

	void graphics_renderpass_begin(RenderPass* renderPass, GPUImage** attachments, const Color4f* colors, float depth, ui32 stencil, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		RenderPass_internal* rp = reinterpret_cast<RenderPass_internal*>(renderPass);
		state.renderPass = rp;

		ui32 attCount = ui32(rp->attachments.size());
		memcpy(state.attachments, attachments, sizeof(GPUImage*) * attCount);
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;

		if (colors != nullptr)
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, rp->attachments.size() * sizeof(vec4f));
		g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);

		g_Device.renderpass_begin(cmd);
	}
	void graphics_renderpass_end(CommandList cmd)
	{
		g_Device.renderpass_end(cmd);
		g_PipelineState.graphics[cmd].renderPass = nullptr;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;
	}

	////////////////////////////////////////// DRAW CALLS /////////////////////////////////////////

	void graphics_draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		g_Device.draw(vertexCount, instanceCount, startVertex, startInstance, cmd);
	}
	void graphics_draw_indexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd)
	{
		g_Device.draw_indexed(indexCount, instanceCount, startIndex, startVertex, startInstance, cmd);
	}

	////////////////////////////////////////// MEMORY /////////////////////////////////////////

	void graphics_buffer_update(GPUBuffer* buffer, void* pData, ui32 size, ui32 offset, CommandList cmd)
	{
		g_Device.buffer_update(buffer, pData, size, offset, cmd);
	}

	void graphics_barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd)
	{
		g_Device.barrier(barriers, count, cmd);
	}

	void graphics_image_blit(GPUImage* src, GPUImage* dst, GPUImageLayout srcLayout, GPUImageLayout dstLayout, ui32 count, const GPUImageBlit* imageBlit, SamplerFilter filter, CommandList cmd)
	{
		g_Device.image_blit(src, dst, srcLayout, dstLayout, count, imageBlit, filter, cmd);
	}

	void graphics_image_clear(GPUImage* image, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, ui32 stencil, CommandList cmd)
	{
		g_Device.image_clear(image, oldLayout, newLayout, clearColor, depth, stencil, cmd);
	}

	const ShaderMaterialInfo* graphics_shader_materialinfo_get(Shader* shader_)
	{
		SV_ASSERT(shader_);
		const Shader_internal& shader = *reinterpret_cast<Shader_internal*>(shader_);
		return &shader.materialInfo;
	}

	ui32 graphics_shader_attribute_size(ShaderAttributeType type)
	{
		switch (type)
		{
		case sv::ShaderAttributeType_Boolean:
		case sv::ShaderAttributeType_Char:
			return 1u;

		case sv::ShaderAttributeType_Half:
			return 2u;

		case sv::ShaderAttributeType_Float:
		case sv::ShaderAttributeType_UInt32:
		case sv::ShaderAttributeType_Int32:
			return 4u;

		case sv::ShaderAttributeType_Float2:
		case sv::ShaderAttributeType_Double:
		case sv::ShaderAttributeType_UInt64:
		case sv::ShaderAttributeType_Int64:
			return 8u;

		case sv::ShaderAttributeType_Float3:
			return 12u;

		case sv::ShaderAttributeType_Float4:
			return 16u;
		
		case sv::ShaderAttributeType_Mat3:
			return 36u;

		case sv::ShaderAttributeType_Mat4:
			return 64u;

		case sv::ShaderAttributeType_Texture:
			return sizeof(void*);

		default:
			return 0u;
		}
	}

	void graphics_shader_input_get(Shader* shader_, ShaderAttribute const** inputAttr, ui32* count)
	{
		SV_ASSERT(shader_);
		const Shader_internal& shader = *reinterpret_cast<Shader_internal*>(shader_);

		*inputAttr = shader.input.data();
		*count = ui32(shader.input.size());
	}

	ui32 graphics_image_get_width(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->width;
	}
	ui32 graphics_image_get_height(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->height;
	}
	ui32 graphics_image_get_depth(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->depth;
	}
	ui32 graphics_image_get_layers(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->layers;
	}
	ui32 graphics_image_get_dimension(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->dimension;
	}
	Format graphics_image_get_format(GPUImage* image)
	{
		return reinterpret_cast<GPUImage_internal*>(image)->format;
	}
	Viewport graphics_image_get_viewport(GPUImage* image)
	{
		GPUImage_internal& p = *reinterpret_cast<GPUImage_internal*>(image);
		return { 0.f, 0.f, float(p.width), float(p.height), 0.f, 1.f };
	}
	Scissor graphics_image_get_scissor(GPUImage* image)
	{
		GPUImage_internal& p = *reinterpret_cast<GPUImage_internal*>(image);
		return { 0.f, 0.f, float(p.width), float(p.height) };
	}

}