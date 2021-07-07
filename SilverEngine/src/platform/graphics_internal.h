#pragma once

#include "platform/graphics.h"
#include "utils/allocators.h"

// TEMP TODO
#include <mutex>
#include <functional>

namespace sv {

    // Primitives internal

    struct Primitive_internal {
		GraphicsPrimitiveType type;

#if SV_GFX
		std::string name;
#endif

    };

    struct GPUBuffer_internal : public Primitive_internal {
		GPUBufferInfo info;
    };

    struct GPUImage_internal : public Primitive_internal {
		GPUImageInfo info;
    };

    struct Sampler_internal : public Primitive_internal {
		SamplerInfo info;
    };

    struct Shader_internal : public Primitive_internal {
		ShaderInfo info;
    };

    struct RenderPass_internal : public Primitive_internal {
		RenderPassInfo info;
    };

    struct InputLayoutState_internal : public Primitive_internal {
		InputLayoutStateInfo info;
    };

    struct BlendState_internal : public Primitive_internal {
		BlendStateInfo info;
    };

    struct DepthStencilState_internal : public Primitive_internal {
		DepthStencilStateInfo info;
    };
	
    struct RasterizerState_internal : public Primitive_internal {
		RasterizerStateInfo info;
    };

    // Pipeline state

    enum GraphicsPipelineState : u64 {
		GraphicsPipelineState_None = 0,
		GraphicsPipelineState_VertexBuffer = SV_BIT(0),
		GraphicsPipelineState_IndexBuffer = SV_BIT(1),

		GraphicsPipelineState_ConstantBuffer = SV_BIT(2),
		GraphicsPipelineState_ShaderResource = SV_BIT(2),
		GraphicsPipelineState_UnorderedAccessView = SV_BIT(2),
		GraphicsPipelineState_Sampler = SV_BIT(2),
		
		GraphicsPipelineState_Resource_VS = SV_BIT(3),
		GraphicsPipelineState_Resource_PS = SV_BIT(4),
		GraphicsPipelineState_Resource_GS = SV_BIT(5),
		GraphicsPipelineState_Resource_HS = SV_BIT(6),
		GraphicsPipelineState_Resource_DS = SV_BIT(7),
		GraphicsPipelineState_Resource_MS = SV_BIT(8),
		GraphicsPipelineState_Resource_TS = SV_BIT(9),

		GraphicsPipelineState_Shader	= SV_BIT(26),
		GraphicsPipelineState_Shader_VS = SV_BIT(27),
		GraphicsPipelineState_Shader_PS = SV_BIT(28),
		GraphicsPipelineState_Shader_GS = SV_BIT(29),
		GraphicsPipelineState_Shader_HS = SV_BIT(30),
		GraphicsPipelineState_Shader_DS = SV_BIT(31),
		GraphicsPipelineState_Shader_MS = SV_BIT(32),
		GraphicsPipelineState_Shader_TS = SV_BIT(33),

		GraphicsPipelineState_RenderPass = SV_BIT(34),

		GraphicsPipelineState_InputLayoutState	= SV_BIT(35),
		GraphicsPipelineState_BlendState		= SV_BIT(36),
		GraphicsPipelineState_DepthStencilState = SV_BIT(37),
		GraphicsPipelineState_RasterizerState	= SV_BIT(38),

		GraphicsPipelineState_Viewport		= SV_BIT(39),
		GraphicsPipelineState_Scissor		= SV_BIT(40),
		GraphicsPipelineState_Topology		= SV_BIT(41),
		GraphicsPipelineState_StencilRef	= SV_BIT(42),
		GraphicsPipelineState_LineWidth		= SV_BIT(43),
    };

    struct GraphicsState {
		GPUBuffer_internal*				vertexBuffers[GraphicsLimit_VertexBuffer];
		u32								vertexBufferOffsets[GraphicsLimit_VertexBuffer];
		u32								vertexBuffersCount;

		GPUBuffer_internal*				indexBuffer;
		u32								indexBufferOffset;

		void*				            shader_resources[ShaderType_GraphicsCount][GraphicsLimit_ShaderResource];
		u32								shader_resource_count[ShaderType_GraphicsCount];

		GPUBuffer_internal*				constant_buffers[ShaderType_GraphicsCount][GraphicsLimit_ConstantBuffer];
		u32								constant_buffer_count[ShaderType_GraphicsCount];
		
		void*				            unordered_access_views[ShaderType_GraphicsCount][GraphicsLimit_UnorderedAccessView];
		u32								unordered_access_view_count[ShaderType_GraphicsCount];

		Sampler_internal*				samplers[ShaderType_GraphicsCount][GraphicsLimit_Sampler];
		u32								samplersCount[ShaderType_GraphicsCount];

		Shader_internal*				vertexShader;
		Shader_internal*				pixelShader;
		Shader_internal*				geometryShader;

		InputLayoutState_internal*		inputLayoutState;
		BlendState_internal*			blendState;
		DepthStencilState_internal*		depthStencilState;
		RasterizerState_internal*		rasterizerState;

		Viewport					viewports[GraphicsLimit_Viewport];
		u32								viewportsCount;
		Scissor						scissors[GraphicsLimit_Scissor];
		u32								scissorsCount;
		GraphicsTopology				topology;
		u32								stencilReference;
		float							lineWidth;

		RenderPass_internal*			renderPass;
		GPUImage_internal*				attachments[GraphicsLimit_Attachment];
		v4_f32						clearColors[GraphicsLimit_Attachment];
		std::pair<float, u32>			clearDepthStencil;

		u64 flags;
    };

    struct ComputeState {
		Shader_internal* compute_shader;

		void*               shader_resources[GraphicsLimit_ShaderResource];
		u32		            shader_resource_count;

		GPUBuffer_internal*	constant_buffers[GraphicsLimit_ConstantBuffer];
		u32					constant_buffer_count;
		
		void*			    unordered_access_views[GraphicsLimit_UnorderedAccessView];
		u32					unordered_access_view_count;

		bool update_resources;
    };

    struct PipelineState {
		GraphicsState			graphics[GraphicsLimit_CommandList];
		ComputeState			compute[GraphicsLimit_CommandList];

		GPUImage* present_image;
		GPUImageLayout present_image_layout;
    };

    // GraphicsAPI Device

    typedef bool(*FNP_graphics_api_initialize)();
    typedef bool(*FNP_graphics_api_close)();
    typedef void*(*FNP_graphics_api_get)();

    typedef bool(*FNP_graphics_create)(GraphicsPrimitiveType, const void*, Primitive_internal*);
    typedef bool(*FNP_graphics_destroy)(Primitive_internal*);

    typedef CommandList(*FNP_graphics_api_commandlist_begin)();
    typedef CommandList(*FNP_graphics_api_commandlist_last)();
    typedef u32(*FNP_graphics_api_commandlist_count)();

    typedef void(*FNP_graphics_api_renderpass_begin)(CommandList);
    typedef void(*FNP_graphics_api_renderpass_end)(CommandList);

    typedef void(*FNP_graphics_api_swapchain_resize)();

    typedef void(*FNP_graphics_api_gpu_wait)();

    typedef void(*FNP_graphics_api_frame_begin)();
    typedef void(*FNP_graphics_api_frame_end)();

    typedef void(*FNP_graphics_api_draw)(u32, u32, u32, u32, CommandList);
    typedef void(*FNP_graphics_api_draw_indexed)(u32, u32, u32, u32, u32, CommandList);
	typedef void(*FNP_graphics_api_dispatch)(u32, u32, u32, CommandList);

    typedef void(*FNP_graphics_api_image_clear)(GPUImage*, GPUImageLayout, GPUImageLayout, Color, float, u32, CommandList);
    typedef void(*FNP_graphics_api_image_blit)(GPUImage*, GPUImage*, GPUImageLayout, GPUImageLayout, u32, const GPUImageBlit*, SamplerFilter, CommandList);
    typedef void(*FNP_graphics_api_buffer_update)(GPUBuffer*, GPUBufferState, const void*, u32, u32, CommandList);
    typedef void(*FNP_graphics_api_barrier)(const GPUBarrier*, u32, CommandList);

    typedef void(*FNP_graphics_api_event_begin)(const char*, CommandList);
    typedef void(*FNP_graphics_api_event_mark)(const char*, CommandList);
    typedef void(*FNP_graphics_api_event_end)(CommandList);

    struct GraphicsDevice {

		FNP_graphics_api_initialize	initialize;
		FNP_graphics_api_close		close;
		FNP_graphics_api_get		get;
	
		FNP_graphics_create    create;
		FNP_graphics_destroy   destroy;

		FNP_graphics_api_commandlist_begin	commandlist_begin;
		FNP_graphics_api_commandlist_last	commandlist_last;
		FNP_graphics_api_commandlist_count	commandlist_count;

		FNP_graphics_api_renderpass_begin	renderpass_begin;
		FNP_graphics_api_renderpass_end		renderpass_end;

		FNP_graphics_api_swapchain_resize swapchain_resize;

		FNP_graphics_api_gpu_wait gpu_wait;

		FNP_graphics_api_frame_begin		frame_begin;
		FNP_graphics_api_frame_begin		frame_end;

		FNP_graphics_api_draw		  draw;
		FNP_graphics_api_draw_indexed draw_indexed;
		FNP_graphics_api_dispatch     dispatch;
		
		FNP_graphics_api_image_clear	image_clear;
		FNP_graphics_api_image_blit	image_blit;
		FNP_graphics_api_buffer_update	buffer_update;
		FNP_graphics_api_barrier	barrier;

		FNP_graphics_api_event_begin	event_begin;
		FNP_graphics_api_event_mark	event_mark;
		FNP_graphics_api_event_end	event_end;

		// TODO
		std::unique_ptr<SizedInstanceAllocator> bufferAllocator;
		std::mutex								bufferMutex;

		std::unique_ptr<SizedInstanceAllocator> imageAllocator;
		std::mutex								imageMutex;

		std::unique_ptr<SizedInstanceAllocator> samplerAllocator;
		std::mutex								samplerMutex;

		std::unique_ptr<SizedInstanceAllocator> shaderAllocator;
		std::mutex								shaderMutex;

		std::unique_ptr<SizedInstanceAllocator> renderPassAllocator;
		std::mutex								renderPassMutex;

		std::unique_ptr<SizedInstanceAllocator> inputLayoutStateAllocator;
		std::mutex								inputLayoutStateMutex;

		std::unique_ptr<SizedInstanceAllocator> blendStateAllocator;
		std::mutex								blendStateMutex;

		std::unique_ptr<SizedInstanceAllocator> depthStencilStateAllocator;
		std::mutex								depthStencilStateMutex;

		std::unique_ptr<SizedInstanceAllocator> rasterizerStateAllocator;
		std::mutex								rasterizerStateMutex;

		GraphicsAPI api = GraphicsAPI_Invalid;

    };

    void graphics_swapchain_resize();

    void*		graphics_internaldevice_get() noexcept;
    GraphicsDevice*	graphics_device_get() noexcept;
    PipelineState&	graphics_state_get() noexcept;

    // Shader utils

    bool graphics_shader_initialize();
    bool graphics_shader_close();

}
