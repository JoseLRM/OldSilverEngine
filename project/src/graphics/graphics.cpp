#include "SilverEngine/core.h"

#include "graphics_internal.h"
#include "window/window_internal.h"

#include "vulkan/graphics_vulkan.h"
#include "..\..\include\SilverEngine\graphics.h"

namespace sv {

	static PipelineState		g_PipelineState;
	static GraphicsDevice		g_Device;
	GraphicsProperties	graphics_properties;
	
	// Default Primitives

	static GraphicsState		g_DefGraphicsState;

	static InputLayoutState*	g_DefInputLayoutState;
	static BlendState*			g_DefBlendState;
	static DepthStencilState*	g_DefDepthStencilState;
	static RasterizerState*		g_DefRasterizerState;

	Result graphics_initialize()
	{
		Result res;

		// Initialize API
		SV_LOG_INFO("Trying to initialize vulkan device");
		graphics_vulkan_device_prepare(g_Device);
		res = g_Device.initialize();
		
		if (res != Result_Success) {
			SV_LOG_ERROR("Can't initialize vulkan device (Error code: %u)", res);
		}
		else SV_LOG_INFO("Vulkan device initialized successfuly");

		// Create default states
		{
			InputLayoutStateDesc desc;
			svCheck(graphics_inputlayoutstate_create(&desc, &g_DefInputLayoutState));
		}
		{
			BlendAttachmentDesc att;
			att.blendEnabled = false;
			att.srcColorBlendFactor = BlendFactor_One;
			att.dstColorBlendFactor = BlendFactor_One;
			att.colorBlendOp = BlendOperation_Add;
			att.srcAlphaBlendFactor = BlendFactor_One;
			att.dstAlphaBlendFactor = BlendFactor_One;
			att.alphaBlendOp = BlendOperation_Add;
			att.colorWriteMask = ColorComponent_All;

			BlendStateDesc desc;
			desc.attachmentCount = 1u;
			desc.pAttachments = &att;
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
		for (u32 i = 0; i < GraphicsLimit_Viewport; ++i) {
			g_DefGraphicsState.viewports[i] = { 0.f, 0.f, 100.f, 100.f, 0.f, 1.f };
		}
		g_DefGraphicsState.viewportsCount = GraphicsLimit_Viewport;
		for (u32 i = 0; i < GraphicsLimit_Scissor; ++i) {
			g_DefGraphicsState.scissors[i] = { 0u, 0u, 100u, 100u };
		}
		g_DefGraphicsState.scissorsCount = GraphicsLimit_Scissor;
		g_DefGraphicsState.topology = GraphicsTopology_Triangles;
		g_DefGraphicsState.stencilReference = 0u;
		g_DefGraphicsState.lineWidth = 1.f;
		for (u32 i = 0; i < GraphicsLimit_Attachment; ++i) {
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

	static inline void destroyUnusedPrimitive(Primitive_internal& primitive)
	{
#ifdef SV_ENABLE_GFX_VALIDATION
		if (primitive.name.size())
			SV_LOG_WARNING("Destroing '%s'", primitive.name.c_str());
#endif

		g_Device.destroy(&primitive);
	}

	Result graphics_close()
	{
		svCheck(graphics_destroy(g_DefBlendState));
		svCheck(graphics_destroy(g_DefDepthStencilState));
		svCheck(graphics_destroy(g_DefInputLayoutState));
		svCheck(graphics_destroy(g_DefRasterizerState));

		if (g_Device.bufferAllocator.get()) {

			std::scoped_lock lock(g_Device.bufferMutex, g_Device.imageMutex, g_Device.samplerMutex, 
				g_Device.shaderMutex, g_Device.renderPassMutex, g_Device.inputLayoutStateMutex, g_Device.blendStateMutex,
				g_Device.depthStencilStateMutex, g_Device.rasterizerStateMutex, g_Device.swapChainMutex);

			u32 count;

			count = g_Device.bufferAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed buffers", count);

				for (auto& pool : *(g_Device.bufferAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}
			
			count = g_Device.imageAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed images", count);
				
				for (auto& pool : *(g_Device.imageAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.samplerAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed samplers", count);

				for (auto& pool : *(g_Device.samplerAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.shaderAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed shaders", count);

				for (auto& pool : *(g_Device.shaderAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.renderPassAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed render passes", count);

				for (auto& pool : *(g_Device.renderPassAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.inputLayoutStateAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed input layout states", count);

				for (auto& pool : *(g_Device.inputLayoutStateAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.blendStateAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed blend states", count);

				for (auto& pool : *(g_Device.blendStateAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.depthStencilStateAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed depth stencil states", count);

				for (auto& pool : *(g_Device.depthStencilStateAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.rasterizerStateAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed rasterizer states", count);

				for (auto& pool : *(g_Device.rasterizerStateAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			count = g_Device.swapChainAllocator->unfreed_count();
			if (count) {
				SV_LOG_WARNING("There are %u unfreed swapchains", count);

				for (auto& pool : *(g_Device.swapChainAllocator.get())) {
					for (void* p : pool) {
						destroyUnusedPrimitive(*reinterpret_cast<Primitive_internal*>(p));
					}
				}
			}

			g_Device.bufferAllocator->clear();
			g_Device.imageAllocator->clear();
			g_Device.samplerAllocator->clear();
			g_Device.shaderAllocator->clear();
			g_Device.inputLayoutStateAllocator->clear();
			g_Device.blendStateAllocator->clear();
			g_Device.depthStencilStateAllocator->clear();
			g_Device.rasterizerStateAllocator->clear();
			g_Device.swapChainAllocator->clear();

		}

		g_Device.api = GraphicsAPI_Invalid;
		svCheck(g_Device.close());

		svCheck(graphics_shader_close());

		return Result_Success;
	}

	void graphics_begin()
	{
		SV_PROFILER_SCALAR_SET("Draw Calls", 0);

		g_Device.frame_begin();
	}

	void graphics_end()
	{
		g_Device.frame_end();
	}

	void graphics_present(Window* window, GPUImage* image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd)
	{
		SwapChain_internal* sc = reinterpret_cast<Window_internal*>(window)->swap_chain;
		g_Device.present(sc, image, region, layout, cmd);
	}

	void graphics_present(Window* window, GPUImage* image, GPUImageLayout layout, CommandList cmd)
	{
		SwapChain_internal* sc = reinterpret_cast<Window_internal*>(window)->swap_chain;
		GPUImage_internal& img = *reinterpret_cast<GPUImage_internal*>(image);

		GPUImageRegion reg;
		reg.offset0 = { 0, 0, 0 };
		reg.offset1 = { i32(img.info.width), i32(img.info.height), 1 };

		g_Device.present(sc, image, reg, layout, cmd);
	}

	void graphics_present(Window* window, CommandList cmd)
	{
		SwapChain_internal* sc = reinterpret_cast<Window_internal*>(window)->swap_chain;
		GPUImageRegion reg;
		g_Device.present(sc, nullptr, reg, (GPUImageLayout)0, cmd);
	}

	void graphics_swapchain_resize(SwapChain_internal* sc)
	{
		g_Device.swapchain_resize(sc);
	}

	PipelineState& graphics_state_get() noexcept
	{
		return g_PipelineState;
	}
	void* graphics_internaldevice_get() noexcept
	{
		return g_Device.get();
	}
	GraphicsDevice* graphics_device_get() noexcept
	{
		return &g_Device;
	}

	/////////////////////////////////////// HASH FUNCTIONS ///////////////////////////////////////

	size_t graphics_compute_hash_inputlayoutstate(const InputLayoutStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const InputLayoutStateDesc& desc = *d;
		size_t hash = 0u;
		sv::hash_combine(hash, desc.slotCount);

		for (u32 i = 0; i < desc.slotCount; ++i) {
			const InputSlotDesc& slot = desc.pSlots[i];
			sv::hash_combine(hash, slot.slot);
			sv::hash_combine(hash, slot.stride);
			sv::hash_combine(hash, u64(slot.instanced));
		}

		sv::hash_combine(hash, desc.elementCount);

		for (u32 i = 0; i < desc.elementCount; ++i) {
			const InputElementDesc& element = desc.pElements[i];
			sv::hash_combine(hash, u64(element.format));
			sv::hash_combine(hash, element.inputSlot);
			sv::hash_combine(hash, element.offset);
			sv::hash_combine(hash, sv::hash_string(element.name));
		}

		return hash;
	}
	size_t graphics_compute_hash_blendstate(const BlendStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const BlendStateDesc& desc = *d;
		size_t hash = 0u;
		sv::hash_combine(hash, u64(double(desc.blendConstants.x) * 2550.0));
		sv::hash_combine(hash, u64(double(desc.blendConstants.y) * 2550.0));
		sv::hash_combine(hash, u64(double(desc.blendConstants.z) * 2550.0));
		sv::hash_combine(hash, u64(double(desc.blendConstants.w) * 2550.0));
		sv::hash_combine(hash, desc.attachmentCount);

		for (u32 i = 0; i < desc.attachmentCount; ++i)
		{
			const BlendAttachmentDesc& att = desc.pAttachments[i];
			sv::hash_combine(hash, att.alphaBlendOp);
			sv::hash_combine(hash, att.blendEnabled ? 1u : 0u);
			sv::hash_combine(hash, att.colorBlendOp);
			sv::hash_combine(hash, att.colorWriteMask);
			sv::hash_combine(hash, att.dstAlphaBlendFactor);
			sv::hash_combine(hash, att.dstColorBlendFactor);
			sv::hash_combine(hash, att.srcAlphaBlendFactor);
			sv::hash_combine(hash, att.srcColorBlendFactor);
		}

		return hash;
	}
	size_t graphics_compute_hash_rasterizerstate(const RasterizerStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const RasterizerStateDesc& desc = *d;
		size_t hash = 0u;
		sv::hash_combine(hash, desc.clockwise ? 1u : 0u);
		sv::hash_combine(hash, desc.cullMode);
		sv::hash_combine(hash, desc.wireframe ? 1u : 0u);

		return hash;
	}
	size_t graphics_compute_hash_depthstencilstate(const DepthStencilStateDesc* d)
	{
		if (d == nullptr) return 0u;

		const DepthStencilStateDesc& desc = *d;
		size_t hash = 0u;
		sv::hash_combine(hash, desc.depthCompareOp);
		sv::hash_combine(hash, desc.depthTestEnabled ? 1u : 0u);
		sv::hash_combine(hash, desc.depthWriteEnabled ? 1u : 0u);
		sv::hash_combine(hash, desc.stencilTestEnabled ? 1u : 0u);
		sv::hash_combine(hash, desc.readMask);
		sv::hash_combine(hash, desc.writeMask);
		sv::hash_combine(hash, desc.front.compareOp);
		sv::hash_combine(hash, desc.front.depthFailOp);
		sv::hash_combine(hash, desc.front.failOp);
		sv::hash_combine(hash, desc.front.passOp);
		sv::hash_combine(hash, desc.back.compareOp);
		sv::hash_combine(hash, desc.back.depthFailOp);
		sv::hash_combine(hash, desc.back.failOp);
		sv::hash_combine(hash, desc.back.passOp);

		return hash;
	}

	bool graphics_format_has_stencil(Format format)
	{
		if (format == Format_D24_UNORM_S8_UINT)
			return true;
		else return false;
	}
	constexpr u32 graphics_format_size(Format format)
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
			SV_LOG_INFO("Unknown format size");
			return 0u;
		}
	}

	GraphicsAPI graphics_api_get()
	{
		return GraphicsAPI_Vulkan;
	}

	////////////////////////////////////////// PRIMITIVES /////////////////////////////////////////

	Result graphics_buffer_create(const GPUBufferDesc* desc, GPUBuffer** buffer)
	{
#ifdef SV_DEBUG
		if (desc->usage == ResourceUsage_Static && desc->CPUAccess & CPUAccess_Write) {
			SV_LOG_ERROR("Buffer with static usage can't have CPU access");
			return Result_InvalidUsage;
		}
#endif
		
		// Allocate memory
		{
			std::scoped_lock lock(g_Device.bufferMutex);
			*buffer = (GPUBuffer*)g_Device.bufferAllocator->alloc();
		}
		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Buffer, desc, (Primitive_internal*)*buffer));

		// Set parameters
		GPUBuffer_internal* p = reinterpret_cast<GPUBuffer_internal*>(*buffer);
		p->type = GraphicsPrimitiveType_Buffer;
		p->info.bufferType = desc->bufferType;
		p->info.usage = desc->usage;
		p->info.size = desc->size;
		p->info.indexType = desc->indexType;
		p->info.CPUAccess = desc->CPUAccess;

		return Result_Success;
	}

	Result graphics_shader_create(const ShaderDesc* desc, Shader** shader)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.shaderMutex);
			*shader = (Shader*)g_Device.shaderAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Shader, desc, (Primitive_internal*)* shader));

		// Set parameters
		Shader_internal* p = reinterpret_cast<Shader_internal*>(*shader);
		p->type = GraphicsPrimitiveType_Shader;
		p->info.shader_type = desc->shaderType;

		return Result_Success;
	}

	Result graphics_image_create(const GPUImageDesc* desc, GPUImage** image)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.imageMutex);
			*image = (GPUImage*)g_Device.imageAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_Image, desc, (Primitive_internal*)* image));

		// Set parameters
		GPUImage_internal* p = reinterpret_cast<GPUImage_internal*>(*image);
		p->type = GraphicsPrimitiveType_Image;
		p->info.dimension = desc->dimension;
		p->info.format = desc->format;
		p->info.width = desc->width;
		p->info.height = desc->height;
		p->info.depth = desc->depth;
		p->info.layers = desc->layers;
		p->info.type = desc->type;

		return Result_Success;
	}

	Result graphics_sampler_create(const SamplerDesc* desc, Sampler** sampler)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.samplerMutex);
			*sampler = (Sampler*)g_Device.samplerAllocator->alloc();
		}

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
		{
			std::scoped_lock lock(g_Device.renderPassMutex);
			*renderPass = (RenderPass*)g_Device.renderPassAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_RenderPass, desc, (Primitive_internal*)* renderPass));

		// Set parameters
		RenderPass_internal* p = reinterpret_cast<RenderPass_internal*>(*renderPass);
		p->type = GraphicsPrimitiveType_RenderPass;
		p->info.depthstencil_attachment_index = desc->attachmentCount;
		for (u32 i = 0; i < desc->attachmentCount; ++i) {
			if (desc->pAttachments[i].type == AttachmentType_DepthStencil) {
				p->info.depthstencil_attachment_index = i;
				break;
			}
		}

		p->info.attachments.resize(desc->attachmentCount);
		for (u32 i = 0; i < desc->attachmentCount; ++i) {

			p->info.attachments[i] = desc->pAttachments[i];
		}

		return Result_Success;
	}

	Result graphics_inputlayoutstate_create(const InputLayoutStateDesc* desc, InputLayoutState** inputLayoutState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.inputLayoutStateMutex);
			*inputLayoutState = (InputLayoutState*)g_Device.inputLayoutStateAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_InputLayoutState, desc, (Primitive_internal*)* inputLayoutState));

		// Set parameters
		InputLayoutState_internal* p = reinterpret_cast<InputLayoutState_internal*>(*inputLayoutState);
		p->type = GraphicsPrimitiveType_InputLayoutState;
		
		p->info.slots.resize(desc->slotCount);
		p->info.elements.resize(desc->elementCount);

		foreach(i, desc->slotCount) {
			p->info.slots[i] = desc->pSlots[i];
		}
		foreach(i, desc->elementCount) {
			p->info.elements[i] = desc->pElements[i];
		}

		return Result_Success;
	}

	Result graphics_blendstate_create(const BlendStateDesc* desc, BlendState** blendState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.blendStateMutex);
			*blendState = (BlendState*)g_Device.blendStateAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_BlendState, desc, (Primitive_internal*)* blendState));

		// Set parameters
		BlendState_internal* p = reinterpret_cast<BlendState_internal*>(*blendState);
		p->type = GraphicsPrimitiveType_BlendState;
		
		p->info.blendConstants = desc->blendConstants;
		p->info.attachments.resize(desc->attachmentCount);

		foreach(i, desc->attachmentCount) {
			p->info.attachments[i] = desc->pAttachments[i];
		}

		return Result_Success;
	}

	Result graphics_depthstencilstate_create(const DepthStencilStateDesc* desc, DepthStencilState** depthStencilState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.depthStencilStateMutex);
			*depthStencilState = (DepthStencilState*)g_Device.depthStencilStateAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_DepthStencilState, desc, (Primitive_internal*)* depthStencilState));

		// Set parameters
		DepthStencilState_internal* p = reinterpret_cast<DepthStencilState_internal*>(*depthStencilState);
		p->type = GraphicsPrimitiveType_DepthStencilState;
		memcpy(&p->info, desc, sizeof(DepthStencilStateInfo));

		return Result_Success;
	}

	Result graphics_rasterizerstate_create(const RasterizerStateDesc* desc, RasterizerState** rasterizerState)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.rasterizerStateMutex);
			*rasterizerState = (RasterizerState*)g_Device.rasterizerStateAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_DepthStencilState, desc, (Primitive_internal*)* rasterizerState));

		// Set parameters
		RasterizerState_internal* p = reinterpret_cast<RasterizerState_internal*>(*rasterizerState);
		p->type = GraphicsPrimitiveType_RasterizerState;
		memcpy(&p->info, desc, sizeof(RasterizerStateInfo));

		return Result_Success;
	}

	Result graphics_swapchain_create(Window* window, SwapChain_internal** swapChain)
	{
#ifdef SV_DEBUG
#endif

		// Allocate memory
		{
			std::scoped_lock lock(g_Device.swapChainMutex);
			*swapChain = (SwapChain_internal*)g_Device.swapChainAllocator->alloc();
		}

		// Create API primitive
		svCheck(g_Device.create(GraphicsPrimitiveType_SwapChain, window, (Primitive_internal*)*swapChain));

		// Set parameters
		SwapChain_internal* p = reinterpret_cast<SwapChain_internal*>(*swapChain);
		p->type = GraphicsPrimitiveType_SwapChain;
		p->window = window;

		return Result_Success;
	}

	Result graphics_destroy(Primitive* primitive)
	{
		if (primitive == nullptr) return Result_Success;

		if (g_Device.api == GraphicsAPI_Invalid)
		{
			SV_LOG_ERROR("Trying to destroy a graphics primitive after the system is closed");
			return Result_InvalidUsage;
		}

		Primitive_internal* p = reinterpret_cast<Primitive_internal*>(primitive);
		Result res = g_Device.destroy(p);
		
		switch (p->type)
		{
		case GraphicsPrimitiveType_Image:
		{
			std::scoped_lock lock(g_Device.imageMutex);
			g_Device.imageAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_Sampler:
		{
			std::scoped_lock lock(g_Device.samplerMutex);
			g_Device.samplerAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_Buffer:
		{
			std::scoped_lock lock(g_Device.bufferMutex);
			g_Device.bufferAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_Shader:
		{
			std::scoped_lock lock(g_Device.shaderMutex);
			g_Device.shaderAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_RenderPass:
		{
			std::scoped_lock lock(g_Device.renderPassMutex);
			g_Device.renderPassAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_InputLayoutState:
		{
			std::scoped_lock lock(g_Device.inputLayoutStateMutex);
			g_Device.inputLayoutStateAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_BlendState:
		{
			std::scoped_lock lock(g_Device.blendStateMutex);
			g_Device.blendStateAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_DepthStencilState:
		{
			std::scoped_lock lock(g_Device.depthStencilStateMutex);
			g_Device.depthStencilStateAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_RasterizerState:
		{
			std::scoped_lock lock(g_Device.rasterizerStateMutex);
			g_Device.rasterizerStateAllocator->free(p);
			break;
		}
		case GraphicsPrimitiveType_SwapChain:
		{
			std::scoped_lock lock(g_Device.swapChainMutex);
			g_Device.swapChainAllocator->free(p);
			break;
		}
		}

		return Result_Success;
	}

	Result graphics_destroy_struct(void* data, size_t size)
	{
		SV_ASSERT(size % sizeof(void*) == 0u);

		u8* it = (u8*)data;
		u8* end = it + size;

		Result res = Result_Success;

		while (it != end) {

			Primitive*& primitive = *reinterpret_cast<Primitive**>(it);
			Result res0 = graphics_destroy(primitive);
			primitive = nullptr;

			if (result_fail(res0)) {
				res = res0;
			}

			it += sizeof(void*);
		}

		return res;
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

	u32 graphics_commandlist_count()
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
		default:
			return GraphicsPipelineState(0);
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
		default:
			return GraphicsPipelineState(0);
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
		default:
			return GraphicsPipelineState(0);
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

	void graphics_vertexbuffer_bind_array(GPUBuffer** buffers, u32* offsets, u32 count, u32 beginSlot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, beginSlot + count);

		memcpy(state.vertexBuffers + beginSlot, buffers, sizeof(GPUBuffer*) * count);
		memcpy(state.vertexBufferOffsets + beginSlot, offsets, sizeof(u32) * count);
		state.flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_bind(GPUBuffer* buffer, u32 offset, u32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.vertexBuffersCount = std::max(state.vertexBuffersCount, slot + 1u);
		state.vertexBuffers[slot] = reinterpret_cast<GPUBuffer_internal*>(buffer);
		state.vertexBufferOffsets[slot] = offset;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_vertexbuffer_unbind(u32 slot, CommandList cmd)
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

		SV_ZERO_MEMORY(state.vertexBuffers, state.vertexBuffersCount * sizeof(GPUBuffer_internal*));
		state.vertexBuffersCount = 0u;

		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_VertexBuffer;
	}

	void graphics_indexbuffer_bind(GPUBuffer* buffer, u32 offset, CommandList cmd)
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

	void graphics_constantbuffer_bind_array(GPUBuffer** buffers, u32 count, u32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffersCount[shaderType] = std::max(state.constantBuffersCount[shaderType], beginSlot + count);

		memcpy(state.constantBuffers, buffers, sizeof(GPUBuffer*) * count);
		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_bind(GPUBuffer* buffer, u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffers[shaderType][slot] = reinterpret_cast<GPUBuffer_internal*>(buffer);
		state.constantBuffersCount[shaderType] = std::max(state.constantBuffersCount[shaderType], slot + 1u);

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind(u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.constantBuffers[shaderType][slot] = nullptr;

		// Compute Constant Buffer Count
		u32& count = state.constantBuffersCount[shaderType];

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

		SV_ZERO_MEMORY(state.constantBuffers[shaderType], state.constantBuffersCount[shaderType] * sizeof(GPUBuffer_internal*));
		state.constantBuffersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_ConstantBuffer;
		state.flags |= graphics_pipelinestateflags_constantbuffer(shaderType);
	}

	void graphics_constantbuffer_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (u32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.constantBuffersCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_constantbuffer((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_ConstantBuffer;
	}

	void graphics_image_bind_array(GPUImage** images, u32 count, u32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.imagesCount[shaderType] = std::max(state.imagesCount[shaderType], beginSlot + count);

		memcpy(state.images[shaderType] + beginSlot, images, sizeof(GPUImage*) * count);
		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_bind(GPUImage* image, u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.images[shaderType][slot] = reinterpret_cast<GPUImage_internal*>(image);
		state.imagesCount[shaderType] = std::max(state.imagesCount[shaderType], slot + 1u);
		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}

	void graphics_image_unbind(u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.images[shaderType][slot] = nullptr;

		// Compute Images Count
		u32& count = state.imagesCount[shaderType];

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

		SV_ZERO_MEMORY(state.images[shaderType], state.imagesCount[shaderType] * sizeof(GPUImage_internal*));
		state.imagesCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Image;
		state.flags |= graphics_pipelinestateflags_image(shaderType);
	}
	
	void graphics_image_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (u32 i = 0; i < ShaderType_GraphicsCount; ++i) {
			state.imagesCount[i] = 0u;
			state.flags |= graphics_pipelinestateflags_image((ShaderType)i);
		}
		state.flags |= GraphicsPipelineState_Image;
	}

	void graphics_sampler_bind_array(Sampler** samplers, u32 count, u32 beginSlot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplersCount[shaderType] = std::max(state.samplersCount[shaderType], beginSlot + count);

		memcpy(state.samplers, samplers, sizeof(Sampler*) * count);
		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_bind(Sampler* sampler, u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplers[shaderType][slot] = reinterpret_cast<Sampler_internal*>(sampler);
		state.samplersCount[shaderType] = std::max(state.samplersCount[shaderType], slot + 1u);
		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind(u32 slot, ShaderType shaderType, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.samplers[shaderType][slot] = nullptr;

		// Compute Images Count
		u32& count = state.samplersCount[shaderType];

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

		SV_ZERO_MEMORY(state.samplers[shaderType], state.samplersCount[shaderType] * sizeof(Sampler_internal*));
		state.samplersCount[shaderType] = 0u;

		state.flags |= GraphicsPipelineState_Sampler;
		state.flags |= graphics_pipelinestateflags_sampler(shaderType);
	}

	void graphics_sampler_unbind_commandlist(CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		for (u32 i = 0; i < ShaderType_GraphicsCount; ++i) {
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

		switch (shader->info.shader_type)
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

	void graphics_mode_set(GraphicsPipelineMode mode, CommandList cmd)
	{
		g_PipelineState.mode[cmd] = mode;
	}

	void graphics_viewport_set(const Viewport* viewports, u32 count, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		SV_ASSERT(count < GraphicsLimit_Viewport);
		memcpy(state.viewports, viewports, size_t(count) * sizeof(Viewport));
		state.viewportsCount = count;
		state.flags |= GraphicsPipelineState_Viewport;
	}

	void graphics_viewport_set(const Viewport& viewport, u32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.viewports[slot] = viewport;
		state.viewportsCount = std::max(g_PipelineState.graphics[cmd].viewportsCount, slot + 1u);
		state.flags |= GraphicsPipelineState_Viewport;
	}

	void graphics_viewport_set(GPUImage* image, u32 slot, CommandList cmd)
	{
		graphics_viewport_set(graphics_image_viewport(image), slot, cmd);
	}

	void graphics_scissor_set(const Scissor* scissors, u32 count, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		SV_ASSERT(count < GraphicsLimit_Scissor);
		memcpy(state.scissors, scissors, size_t(count) * sizeof(Scissor));
		state.scissorsCount = count;
		state.flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(const Scissor& scissor, u32 slot, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		state.scissors[slot] = scissor;
		state.scissorsCount = std::max(g_PipelineState.graphics[cmd].scissorsCount, slot + 1u);
		state.flags |= GraphicsPipelineState_Scissor;
	}

	void graphics_scissor_set(GPUImage* image, u32 slot, CommandList cmd)
	{
		graphics_scissor_set(graphics_image_scissor(image), slot, cmd);
	}

	Viewport graphics_viewport_get(u32 slot, CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].viewports[slot];
	}

	Scissor graphics_scissor_get(u32 slot, CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].scissors[slot];
	}

	const GPUBufferInfo& graphics_buffer_info(const GPUBuffer* buffer)
	{
		const GPUBuffer_internal& buf = *reinterpret_cast<const GPUBuffer_internal*>(buffer);
		return buf.info;
	}

	const GPUImageInfo& graphics_image_info(const GPUImage* image)
	{
		const GPUImage_internal& img = *reinterpret_cast<const GPUImage_internal*>(image);
		return img.info;
	}

	const ShaderInfo& graphics_shader_info(const Shader* shader)
	{
		const Shader_internal& s = *reinterpret_cast<const Shader_internal*>(shader);
		return s.info;
	}

	const RenderPassInfo& graphics_renderpass_info(const RenderPass* renderpass)
	{
		const RenderPass_internal& rp = *reinterpret_cast<const RenderPass_internal*>(renderpass);
		return rp.info;
	}

	const SamplerInfo& graphics_sampler_info(const Sampler* sampler)
	{
		const Sampler_internal& sam = *reinterpret_cast<const Sampler_internal*>(sampler);
		return sam.info;
	}

	const InputLayoutStateInfo& graphics_inputlayoutstate_info(const InputLayoutState* ils)
	{
		const InputLayoutState_internal& i = *reinterpret_cast<const InputLayoutState_internal*>(ils);
		return i.info;
	}

	const BlendStateInfo& graphics_blendstate_info(const BlendState* blendstate)
	{
		const BlendState_internal& bs = *reinterpret_cast<const BlendState_internal*>(blendstate);
		return bs.info;
	}

	const RasterizerStateInfo& graphics_rasterizerstate_info(const RasterizerState* rasterizer)
	{
		const RasterizerState_internal& rs = *reinterpret_cast<const RasterizerState_internal*>(rasterizer);
		return rs.info;
	}

	const DepthStencilStateInfo& graphics_depthstencilstate_info(const DepthStencilState* dss)
	{
		const DepthStencilState_internal& d = *reinterpret_cast<const DepthStencilState_internal*>(dss);
		return d.info;
	}

	GPUImage* graphics_image_get(u32 slot, ShaderType shader_type, CommandList cmd)
	{
		return reinterpret_cast<GPUImage*>(g_PipelineState.graphics[cmd].images[shader_type][slot]);
	}

	void graphics_topology_set(GraphicsTopology topology, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];
		state.topology = topology;
		state.flags |= GraphicsPipelineState_Topology;
	}

	void graphics_stencil_reference_set(u32 ref, CommandList cmd)
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

	u32 graphics_stencil_reference_get(CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].stencilReference;
	}

	float graphics_line_width_get(CommandList cmd)
	{
		return g_PipelineState.graphics[cmd].lineWidth;
	}

	void graphics_renderpass_begin(RenderPass* renderPass, GPUImage** attachments, const Color4f* colors, float depth, u32 stencil, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		RenderPass_internal* rp = reinterpret_cast<RenderPass_internal*>(renderPass);
		state.renderPass = rp;

		u32 attCount = u32(rp->info.attachments.size());
		memcpy(state.attachments, attachments, sizeof(GPUImage*) * attCount);
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;

		if (colors != nullptr)
			memcpy(g_PipelineState.graphics[cmd].clearColors, colors, rp->info.attachments.size() * sizeof(v4_f32));
		g_PipelineState.graphics[cmd].clearDepthStencil = std::make_pair(depth, stencil);

		g_Device.renderpass_begin(cmd);
	}
	void graphics_renderpass_begin(RenderPass* renderPass, GPUImage** attachments, CommandList cmd)
	{
		auto& state = g_PipelineState.graphics[cmd];

		RenderPass_internal* rp = reinterpret_cast<RenderPass_internal*>(renderPass);
		state.renderPass = rp;

		u32 attCount = u32(rp->info.attachments.size());
		memcpy(state.attachments, attachments, sizeof(GPUImage*) * attCount);
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;

		g_Device.renderpass_begin(cmd);
	}
	void graphics_renderpass_end(CommandList cmd)
	{
		g_Device.renderpass_end(cmd);
		g_PipelineState.graphics[cmd].renderPass = nullptr;
		g_PipelineState.graphics[cmd].flags |= GraphicsPipelineState_RenderPass;
	}

	////////////////////////////////////////// DRAW CALLS /////////////////////////////////////////

	void graphics_draw(u32 vertexCount, u32 instanceCount, u32 startVertex, u32 startInstance, CommandList cmd)
	{
		SV_PROFILER_SCALAR_ADD("Draw Calls", 1);
		g_Device.draw(vertexCount, instanceCount, startVertex, startInstance, cmd);
	}
	void graphics_draw_indexed(u32 indexCount, u32 instanceCount, u32 startIndex, u32 startVertex, u32 startInstance, CommandList cmd)
	{
		SV_PROFILER_SCALAR_ADD("Draw Calls", 1);
		g_Device.draw_indexed(indexCount, instanceCount, startIndex, startVertex, startInstance, cmd);
	}

	////////////////////////////////////////// MEMORY /////////////////////////////////////////

	void graphics_buffer_update(GPUBuffer* buffer, const void* pData, u32 size, u32 offset, CommandList cmd)
	{
		g_Device.buffer_update(buffer, pData, size, offset, cmd);
	}

	void graphics_barrier(const GPUBarrier* barriers, u32 count, CommandList cmd)
	{
		g_Device.barrier(barriers, count, cmd);
	}

	void graphics_image_blit(GPUImage* src, GPUImage* dst, GPUImageLayout srcLayout, GPUImageLayout dstLayout, u32 count, const GPUImageBlit* imageBlit, SamplerFilter filter, CommandList cmd)
	{
		g_Device.image_blit(src, dst, srcLayout, dstLayout, count, imageBlit, filter, cmd);
	}

	void graphics_image_clear(GPUImage* image, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, u32 stencil, CommandList cmd)
	{
		g_Device.image_clear(image, oldLayout, newLayout, clearColor, depth, stencil, cmd);
	}

	Result graphics_shader_include_write(const char* name, const char* str)
	{
		std::string filePath = "library/shader_utils/";
		filePath += name;
		filePath += ".hlsl";

#ifndef SV_ENABLE_GFX_VALIDATION
		if (std::filesystem::exists(filePath)) return Result_Success;
#endif

		return file_write_text(filePath.c_str(), str, strlen(str));
	}

	u32 graphics_shader_attribute_size(ShaderAttributeType type)
	{
		switch (type)
		{
		case sv::ShaderAttributeType_Boolean:
		case sv::ShaderAttributeType_Char:
			return 4u;

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

		default:
			return 0u;
		}
	}

	Viewport graphics_image_viewport(const GPUImage* image)
	{
		const GPUImage_internal& p = *reinterpret_cast<const GPUImage_internal*>(image);
		return { 0.f, 0.f, f32(p.info.width), f32(p.info.height), 0.f, 1.f };
	}
	Scissor graphics_image_scissor(const GPUImage* image)
	{
		const GPUImage_internal& p = *reinterpret_cast<const GPUImage_internal*>(image);
		return { 0u, 0u, p.info.width, p.info.height };
	}

	// DEBUG

#ifdef SV_ENABLE_GFX_VALIDATION

	void graphics_event_begin(const char* name, CommandList cmd)
	{
		g_Device.event_begin(name, cmd);
	}
	void graphics_event_mark(const char* name, CommandList cmd)
	{
		g_Device.event_mark(name, cmd);
	}
	void graphics_event_end(CommandList cmd)
	{
		g_Device.event_end(cmd);
	}

	void graphics_name_set(Primitive* primitive_, const char* name)
	{
		if (primitive_ == nullptr) return;
		Primitive_internal& primitive = *reinterpret_cast<Primitive_internal*>(primitive_);
		primitive.name = name;
	}

#endif

}