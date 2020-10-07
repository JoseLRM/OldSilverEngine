#pragma once

#include "graphics.h"
#include "utils/allocators/InstanceAllocator.h"

#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Green, "[GRAPHICS] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Green, "[GRAPHICS_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[GRAPHICS_ERROR] "#x, __VA_ARGS__)

namespace sv {

	// Primitives internal

	struct Primitive_internal {
		GraphicsPrimitiveType type;
	};

	struct GPUBuffer_internal : public Primitive_internal {

		GPUBufferType	bufferType;
		ResourceUsage	usage;
		ui32			size;
		IndexType		indexType;
		CPUAccessFlags	cpuAccess;

	};

	struct GPUImage_internal : public Primitive_internal {
		ui8					dimension;
		Format				format;
		ui32				width;
		ui32				height;
		ui32				depth;
		ui32				layers;
		ImageTypeFlags		imageType;
	};

	struct Sampler_internal : public Primitive_internal {
	};

	struct Shader_internal : public Primitive_internal {
		ShaderType						shaderType;
		std::vector<ShaderAttribute>	input;
		ShaderMaterialInfo				materialInfo;
	};

	struct RenderPass_internal : public Primitive_internal {
		ui32						depthStencilAttachment;
		std::vector<AttachmentDesc>	attachments;
	};

	struct InputLayoutState_internal : public Primitive_internal {
		InputLayoutStateDesc desc;
	};

	struct BlendState_internal : public Primitive_internal {
		BlendStateDesc desc;
	};

	struct DepthStencilState_internal : public Primitive_internal {
		DepthStencilStateDesc desc;
	};
	
	struct RasterizerState_internal : public Primitive_internal {
		RasterizerStateDesc desc;
	};

	// Pipeline state

	enum GraphicsPipelineState : ui64 {
		GraphicsPipelineState_None = 0,
		GraphicsPipelineState_VertexBuffer = SV_BIT(0),
		GraphicsPipelineState_IndexBuffer = SV_BIT(1),

		GraphicsPipelineState_ConstantBuffer = SV_BIT(2),
		GraphicsPipelineState_ConstantBuffer_VS = SV_BIT(3),
		GraphicsPipelineState_ConstantBuffer_PS = SV_BIT(4),
		GraphicsPipelineState_ConstantBuffer_GS = SV_BIT(5),
		GraphicsPipelineState_ConstantBuffer_HS = SV_BIT(6),
		GraphicsPipelineState_ConstantBuffer_DS = SV_BIT(7),
		GraphicsPipelineState_ConstantBuffer_MS = SV_BIT(8),
		GraphicsPipelineState_ConstantBuffer_TS = SV_BIT(9),

		GraphicsPipelineState_Image = SV_BIT(10),
		GraphicsPipelineState_Image_VS = SV_BIT(11),
		GraphicsPipelineState_Image_PS = SV_BIT(12),
		GraphicsPipelineState_Image_GS = SV_BIT(13),
		GraphicsPipelineState_Image_HS = SV_BIT(14),
		GraphicsPipelineState_Image_DS = SV_BIT(15),
		GraphicsPipelineState_Image_MS = SV_BIT(16),
		GraphicsPipelineState_Image_TS = SV_BIT(17),

		GraphicsPipelineState_Sampler = SV_BIT(18),
		GraphicsPipelineState_Sampler_VS = SV_BIT(19),
		GraphicsPipelineState_Sampler_PS = SV_BIT(20),
		GraphicsPipelineState_Sampler_GS = SV_BIT(21),
		GraphicsPipelineState_Sampler_HS = SV_BIT(22),
		GraphicsPipelineState_Sampler_DS = SV_BIT(23),
		GraphicsPipelineState_Sampler_MS = SV_BIT(24),
		GraphicsPipelineState_Sampler_TS = SV_BIT(25),

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

	typedef ui64 GraphicsPipelineStateFlags;

	struct GraphicsState {
		GPUBuffer_internal*				vertexBuffers[SV_GFX_VERTEX_BUFFER_COUNT];
		ui32							vertexBufferOffsets[SV_GFX_VERTEX_BUFFER_COUNT];
		ui32							vertexBuffersCount;

		GPUBuffer_internal*				indexBuffer;
		ui32							indexBufferOffset;

		GPUBuffer_internal*				constantBuffers[ShaderType_GraphicsCount][SV_GFX_CONSTANT_BUFFER_COUNT];
		ui32							constantBuffersCount[ShaderType_GraphicsCount];

		GPUImage_internal*				images[ShaderType_GraphicsCount][SV_GFX_IMAGE_COUNT];
		ui32							imagesCount[ShaderType_GraphicsCount];

		Sampler_internal*				samplers[ShaderType_GraphicsCount][SV_GFX_SAMPLER_COUNT];
		ui32							samplersCount[ShaderType_GraphicsCount];

		Shader_internal*				vertexShader;
		Shader_internal*				pixelShader;
		Shader_internal*				geometryShader;

		InputLayoutState_internal*		inputLayoutState;
		BlendState_internal*			blendState;
		DepthStencilState_internal*		depthStencilState;
		RasterizerState_internal*		rasterizerState;

		sv::Viewport					viewports[SV_GFX_VIEWPORT_COUNT];
		ui32							viewportsCount;
		sv::Scissor						scissors[SV_GFX_SCISSOR_COUNT];
		ui32							scissorsCount;
		GraphicsTopology				topology;
		ui32							stencilReference;
		float							lineWidth;

		RenderPass_internal*			renderPass;
		GPUImage_internal*				attachments[SV_GFX_ATTACHMENTS_COUNT];
		sv::vec4f						clearColors[SV_GFX_ATTACHMENTS_COUNT];
		std::pair<float, ui32>			clearDepthStencil;

		GraphicsPipelineStateFlags		flags;
	};

	struct ComputeState {

	};

	struct PipelineState {
		GraphicsState			graphics[SV_GFX_COMMAND_LIST_COUNT];
		ComputeState			compute[SV_GFX_COMMAND_LIST_COUNT];
		GraphicsPipelineMode	mode[SV_GFX_COMMAND_LIST_COUNT];
	};

	// GraphicsAPI Device

	typedef Result(*FNP_graphics_api_initialize)();
	typedef Result(*FNP_graphics_api_close)();
	typedef void*(*FNP_graphics_api_get)();

	typedef Result(*FNP_graphics_create)(GraphicsPrimitiveType, const void*, Primitive_internal*);
	typedef Result(*FNP_graphics_destroy)(Primitive_internal*);

	typedef CommandList(*FNP_graphics_api_commandlist_begin)();
	typedef CommandList(*FNP_graphics_api_commandlist_last)();
	typedef ui32(*FNP_graphics_api_commandlist_count)();

	typedef void(*FNP_graphics_api_renderpass_begin)(CommandList);
	typedef void(*FNP_graphics_api_renderpass_end)(CommandList);

	typedef void(*FNP_graphics_api_swapchain_resize)();
	typedef GPUImage*(*FNP_graphics_api_swapchain_acquire_image)();

	typedef void(*FNP_graphics_api_gpu_wait)();

	typedef void(*FNP_graphics_api_frame_begin)();
	typedef void(*FNP_graphics_api_commandlist_submit)();
	typedef void(*FNP_graphics_api_present)();

	typedef void(*FNP_graphics_api_draw)(ui32, ui32, ui32, ui32, CommandList);
	typedef void(*FNP_graphics_api_draw_indexed)(ui32, ui32, ui32, ui32, ui32, CommandList);

	typedef void(*FNP_graphics_api_image_clear)(GPUImage*, GPUImageLayout, GPUImageLayout, const Color4f&, float, ui32, CommandList);
	typedef void(*FNP_graphics_api_image_blit)(GPUImage*, GPUImage*, GPUImageLayout, GPUImageLayout, ui32, const GPUImageBlit*, SamplerFilter, CommandList);
	typedef void(*FNP_graphics_api_buffer_update)(GPUBuffer*, void*, ui32, ui32, CommandList);
	typedef void(*FNP_graphics_api_barrier)(const GPUBarrier*, ui32, CommandList);

	struct GraphicsDevice {

		FNP_graphics_api_initialize	initialize;
		FNP_graphics_api_close		close;
		FNP_graphics_api_get		get;

		FNP_graphics_create		create;
		FNP_graphics_destroy	destroy;

		FNP_graphics_api_commandlist_begin	commandlist_begin;
		FNP_graphics_api_commandlist_last	commandlist_last;
		FNP_graphics_api_commandlist_count	commandlist_count;

		FNP_graphics_api_renderpass_begin	renderpass_begin;
		FNP_graphics_api_renderpass_end		renderpass_end;

		FNP_graphics_api_swapchain_resize			swapchain_resize;
		FNP_graphics_api_swapchain_acquire_image	swapchain_acquire_image;

		FNP_graphics_api_gpu_wait gpu_wait;

		FNP_graphics_api_frame_begin		frame_begin;
		FNP_graphics_api_commandlist_submit commandlist_submit;
		FNP_graphics_api_present			present;

		FNP_graphics_api_draw			draw;
		FNP_graphics_api_draw_indexed	draw_indexed;

		FNP_graphics_api_image_clear	image_clear;
		FNP_graphics_api_image_blit		image_blit;
		FNP_graphics_api_buffer_update	buffer_update;
		FNP_graphics_api_barrier		barrier;

		std::unique_ptr<SizedInstanceAllocator> bufferAllocator;
		std::unique_ptr<SizedInstanceAllocator> imageAllocator;
		std::unique_ptr<SizedInstanceAllocator> samplerAllocator;
		std::unique_ptr<SizedInstanceAllocator> shaderAllocator;
		std::unique_ptr<SizedInstanceAllocator> renderPassAllocator;
		std::unique_ptr<SizedInstanceAllocator> inputLayoutStateAllocator;
		std::unique_ptr<SizedInstanceAllocator> blendStateAllocator;
		std::unique_ptr<SizedInstanceAllocator> depthStencilStateAllocator;
		std::unique_ptr<SizedInstanceAllocator> rasterizerStateAllocator;

		std::mutex bufferMutex;
		std::mutex imageMutex;
		std::mutex samplerMutex;
		std::mutex shaderMutex;
		std::mutex renderPassMutex;
		std::mutex inputLayoutStateMutex;
		std::mutex blendStateMutex;
		std::mutex depthStencilStateMutex;
		std::mutex rasterizerStateMutex;

	};

	Result graphics_initialize(const sv::InitializationGraphicsDesc& desc);
	Result graphics_close();

	void graphics_begin();
	void graphics_commandlist_submit();
	void graphics_present();

	void graphics_swapchain_resize();
	GPUImage* graphics_swapchain_acquire_image();

	void*			graphics_device_get() noexcept;
	PipelineState&	graphics_state_get() noexcept;

	// Adapters

	void graphics_adapter_add(std::unique_ptr<sv::Adapter>&& adapter);

	// Shader utils

	Result graphics_shader_initialize();
	Result graphics_shader_close();

	// Properties

	void graphics_properties_set(const GraphicsProperties& props);

}