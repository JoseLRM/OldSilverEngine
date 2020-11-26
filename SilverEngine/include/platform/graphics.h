#pragma once

#include "core.h"
#include "simulation/asset_system.h"

namespace sv {

	struct Primitive;
	struct GPUBuffer;
	struct GPUImage;
	struct Sampler;
	struct RenderPass;
	struct Shader;
	struct InputLayoutState;
	struct BlendState;
	struct DepthStencilState;
	struct RasterizerState;

	// Enums

	enum GraphicsAPI {
		GraphicsAPI_Invalid,
		GraphicsAPI_Vulkan,
	};

	enum GraphicsLimit : ui32 {
		GraphicsLimit_CommandList = 32,
		GraphicsLimit_VertexBuffer = 32,
		GraphicsLimit_ConstantBuffer = 32u,
		GraphicsLimit_GPUImage = 32u,
		GraphicsLimit_Sampler = 16u,
		GraphicsLimit_Viewport = 16u,
		GraphicsLimit_Scissor = 16u,
		GraphicsLimit_InputSlot = 16u,
		GraphicsLimit_InputElement = 16u,
		GraphicsLimit_GPUBarrier = 8u,
		GraphicsLimit_AttachmentRT = 8u,
		GraphicsLimit_Attachment = (GraphicsLimit_AttachmentRT + 1u)
	};

	enum GraphicsPrimitiveType : ui8 {
		GraphicsPrimitiveType_Invalid,
		GraphicsPrimitiveType_Image,
		GraphicsPrimitiveType_Sampler,
		GraphicsPrimitiveType_Buffer,
		GraphicsPrimitiveType_Shader,
		GraphicsPrimitiveType_RenderPass,
		GraphicsPrimitiveType_InputLayoutState,
		GraphicsPrimitiveType_BlendState,
		GraphicsPrimitiveType_DepthStencilState,
		GraphicsPrimitiveType_RasterizerState,
	};

	enum GraphicsPipelineMode : ui8 {
		GraphicsPipelineMode_Graphics,
		GraphicsPipelineMode_Compute,
	};

	enum GPUBufferType : ui8 {
		GPUBufferType_Invalid,
		GPUBufferType_Vertex,
		GPUBufferType_Index,
		GPUBufferType_Constant,
	};

	enum ShaderType : ui32 {
		ShaderType_Vertex,
		ShaderType_Pixel,
		ShaderType_Geometry,
		ShaderType_Hull,
		ShaderType_Domain,
		ShaderType_GraphicsCount = 5u,

		ShaderType_Compute,
	};

	enum ShaderAttributeType : ui32 {
		ShaderAttributeType_Unknown,
		ShaderAttributeType_Float,
		ShaderAttributeType_Float2,
		ShaderAttributeType_Float3,
		ShaderAttributeType_Float4,
		ShaderAttributeType_Half,
		ShaderAttributeType_Double,
		ShaderAttributeType_Boolean,
		ShaderAttributeType_UInt32,
		ShaderAttributeType_UInt64,
		ShaderAttributeType_Int32,
		ShaderAttributeType_Int64,
		ShaderAttributeType_Char,
		ShaderAttributeType_Mat3,
		ShaderAttributeType_Mat4,
	};

	struct ShaderAttribute {
		std::string			name;
		ShaderAttributeType type;
		ui32				offset;
	};

	struct ShaderInfo {

		struct ResourceImage {
			std::string name;
			ui32		bindingSlot;
		};

		struct ResourceSampler {
			std::string name;
			ui32		bindingSlot;
		};

		struct ResourceBuffer {
			std::string						name;
			ui32							bindingSlot;
			std::vector<ShaderAttribute>	attributes;
			ui32							size;
		};

		std::vector<ResourceImage>		images;
		std::vector<ResourceSampler>	samplers;
		std::vector<ResourceBuffer>		constantBuffers;
		
		std::vector<ShaderAttribute>	input;
	};

	enum GPUImageType : ui8 {
		GPUImageType_RenderTarget	= SV_BIT(0),
		GPUImageType_ShaderResource = SV_BIT(1),
		GPUImageType_DepthStencil	= SV_BIT(2),
	};
	typedef ui8 ImageTypeFlags;

	enum ResourceUsage : ui8 {
		ResourceUsage_Default,
		ResourceUsage_Static,
		ResourceUsage_Dynamic,
		ResourceUsage_Staging
	};
	
	enum CPUAccess : ui8 {
		CPUAccess_None = 0u,
		CPUAccess_Read = SV_BIT(0),
		CPUAccess_Write = SV_BIT(1),
		CPUAccess_All = CPUAccess_Read | CPUAccess_Write,
	};
	typedef ui8 CPUAccessFlags;

	enum GPUImageLayout {
		GPUImageLayout_Undefined,
		GPUImageLayout_General,
		GPUImageLayout_RenderTarget,
		GPUImageLayout_DepthStencil,
		GPUImageLayout_DepthStencilReadOnly,
		GPUImageLayout_ShaderResource,
		GPUImageLayout_Present,
	};
	
	enum Format {
		Format_Unknown,
		Format_R32G32B32A32_FLOAT,
		Format_R32G32B32A32_UINT,
		Format_R32G32B32A32_SINT,
		Format_R32G32B32_FLOAT,
		Format_R32G32B32_UINT,
		Format_R32G32B32_SINT,
		Format_R16G16B16A16_FLOAT,
		Format_R16G16B16A16_UNORM,
		Format_R16G16B16A16_UINT,
		Format_R16G16B16A16_SNORM,
		Format_R16G16B16A16_SINT,
		Format_R32G32_FLOAT,
		Format_R32G32_UINT,
		Format_R32G32_SINT,
		Format_R8G8B8A8_UNORM,
		Format_R8G8B8A8_SRGB,
		Format_R8G8B8A8_UINT,
		Format_R8G8B8A8_SNORM,
		Format_R8G8B8A8_SINT,
		Format_R16G16_FLOAT,
		Format_R16G16_UNORM,
		Format_R16G16_UINT,
		Format_R16G16_SNORM,
		Format_R16G16_SINT,
		Format_D32_FLOAT,
		Format_R32_FLOAT,
		Format_R32_UINT,
		Format_R32_SINT,
		Format_D24_UNORM_S8_UINT,
		Format_R8G8_UNORM,
		Format_R8G8_UINT,
		Format_R8G8_SNORM,
		Format_R8G8_SINT,
		Format_R16_FLOAT,
		Format_D16_UNORM,
		Format_R16_UNORM,
		Format_R16_UINT,
		Format_R16_SNORM,
		Format_R16_SINT,
		Format_R8_UNORM,
		Format_R8_UINT,
		Format_R8_SNORM,
		Format_R8_SINT,
		Format_BC1_UNORM,
		Format_BC1_SRGB,
		Format_BC2_UNORM,
		Format_BC2_SRGB,
		Format_BC3_UNORM,
		Format_BC3_SRGB,
		Format_BC4_UNORM,
		Format_BC4_SNORM,
		Format_BC5_UNORM,
		Format_BC5_SNORM,
		Format_B8G8R8A8_UNORM,
		Format_B8G8R8A8_SRGB,

	};
	
	enum GraphicsTopology {
		GraphicsTopology_Points,
		GraphicsTopology_Lines,
		GraphicsTopology_LineStrip,
		GraphicsTopology_Triangles,
		GraphicsTopology_TriangleStrip,
	};
	
	enum IndexType : ui8 {
		IndexType_16,
		IndexType_32,
	};
	
	enum RasterizerCullMode {
		RasterizerCullMode_None,
		RasterizerCullMode_Front,
		RasterizerCullMode_Back,
		RasterizerCullMode_FrontAndBack,
	};

	enum AttachmentOperation {
		AttachmentOperation_DontCare,
		AttachmentOperation_Load,
		AttachmentOperation_Store,
		AttachmentOperation_Clear
	};

	enum AttachmentType {
		AttachmentType_RenderTarget,
		AttachmentType_DepthStencil
	};

	enum CompareOperation {
		CompareOperation_Never,
		CompareOperation_Less,
		CompareOperation_Equal,
		CompareOperation_LessOrEqual,
		CompareOperation_Greater,
		CompareOperation_NotEqual,
		CompareOperation_GreaterOrEqual,
		CompareOperation_Always,
	};

	enum StencilOperation {
		StencilOperation_Keep,
		StencilOperation_Zero,
		StencilOperation_Replace,
		StencilOperation_IncrementAndClamp,
		StencilOperation_DecrementAndClamp,
		StencilOperation_Invert,
		StencilOperation_IncrementAndWrap,
		StencilOperation_DecrementAndWrap
	};

	enum BlendOperation {
		BlendOperation_Add,
		BlendOperation_Substract,
		BlendOperation_ReverseSubstract,
		BlendOperation_Min,
		BlendOperation_Max,
	};

	enum ColorComponent {
		ColorComponent_None = 0,
		ColorComponent_R = SV_BIT(0),
		ColorComponent_G = SV_BIT(1),
		ColorComponent_B = SV_BIT(2),
		ColorComponent_A = SV_BIT(3),
		ColorComponent_RGB = (ColorComponent_R | ColorComponent_G | ColorComponent_B),
		ColorComponent_All = (ColorComponent_RGB | ColorComponent_A),
	};
	typedef ui8 ColorComponentFlags;

	enum BlendFactor {
		BlendFactor_Zero,
		BlendFactor_One,
		BlendFactor_SrcColor,
		BlendFactor_OneMinusSrcColor,
		BlendFactor_DstColor,
		BlendFactor_OneMinusDstColor,
		BlendFactor_SrcAlpha,
		BlendFactor_OneMinusSrcAlpha,
		BlendFactor_DstAlpha,
		BlendFactor_OneMinusDstAlpha,
		BlendFactor_ConstantColor,
		BlendFactor_OneMinusConstantColor,
		BlendFactor_ConstantAlpha,
		BlendFactor_OneMinusConstantAlpha,
		BlendFactor_SrcAlphaSaturate,
		BlendFactor_Src1Color,
		BlendFactor_OneMinusSrc1Color,
		BlendFactor_Src1Alpha,
		BlendFactor_OneMinusSrc1Alpha,
	};

	enum SamplerFilter {
		SamplerFilter_Nearest,
		SamplerFilter_Linear,
	};

	enum SamplerAddressMode {
		SamplerAddressMode_Wrap,
		SamplerAddressMode_Mirror,
		SamplerAddressMode_Clamp,
		SamplerAddressMode_Border
	};
	
	enum SamplerBorderColor {
	};
	
	enum BarrierType : ui8 {
		BarrierType_Image
	};

	struct Viewport {
		float x, y, width, height, minDepth, maxDepth;
	};

	struct Scissor {
		ui32 x, y, width, height;
	};

	struct GPUBarrier {
		union {
			struct {
				GPUImage* pImage;
				GPUImageLayout oldLayout;
				GPUImageLayout newLayout;
			} image;
		};
		BarrierType type;

		inline static GPUBarrier Image(GPUImage* image, GPUImageLayout oldLayout, GPUImageLayout newLayout)
		{
			GPUBarrier barrier;
			barrier.image.pImage = image;
			barrier.image.oldLayout = oldLayout;
			barrier.image.newLayout = newLayout;
			barrier.type = BarrierType_Image;
			return barrier;
		}
	};

	struct GPUImageRegion {
		vec3u offset;
		vec3u size;
	};

	struct GPUImageBlit {
		GPUImageRegion srcRegion;
		GPUImageRegion dstRegion;
	};

	struct ShaderCompileDesc {
		GraphicsAPI											api;
		ShaderType											shaderType;
		ui32												majorVersion;
		ui32												minorVersion;
		const char*											entryPoint;
		std::vector<std::pair<const char*, const char*>>	macros;
	};

	// Primitive Descriptors

	struct GPUBufferDesc {
		GPUBufferType	bufferType;
		ResourceUsage	usage;
		CPUAccessFlags	CPUAccess;
		ui32			size;
		IndexType		indexType;
		void*			pData;
	};

	struct GPUImageDesc {
		void*			pData;
		ui32			size;
		Format			format;
		GPUImageLayout	layout;
		ImageTypeFlags	type;
		ResourceUsage	usage;
		CPUAccessFlags	CPUAccess;
		ui8				dimension;
		ui32			width;
		ui32			height;
		ui32			depth;
		ui32			layers;
	};

	struct SamplerDesc {
		SamplerAddressMode addressModeU;
		SamplerAddressMode addressModeV;
		SamplerAddressMode addressModeW;
		SamplerFilter		minFilter;
		SamplerFilter		magFilter;
	};

	struct ShaderDesc {
		void*		pBinData;
		size_t		binDataSize;
		ShaderType	shaderType;
	};

	struct AttachmentDesc {
		AttachmentOperation	loadOp;
		AttachmentOperation storeOp;
		AttachmentOperation stencilLoadOp;
		AttachmentOperation stencilStoreOp;
		Format				format;
		GPUImageLayout		initialLayout;
		GPUImageLayout		layout;
		GPUImageLayout		finalLayout;
		AttachmentType		type;
	};

	struct RenderPassDesc {
		std::vector<AttachmentDesc> attachments;
	};

	struct InputElementDesc {
		const char* name;
		ui32		index;
		ui32		inputSlot;
		ui32		offset;
		Format		format;
	};

	struct InputSlotDesc {
		ui32 slot;
		ui32 stride;
		bool instanced;
	};

	struct InputLayoutStateDesc {
		std::vector<InputSlotDesc>		slots;
		std::vector<InputElementDesc>	elements;
	};

	struct BlendAttachmentDesc {
		bool				blendEnabled;
		BlendFactor			srcColorBlendFactor;
		BlendFactor			dstColorBlendFactor;
		BlendOperation		colorBlendOp;
		BlendFactor			srcAlphaBlendFactor;
		BlendFactor			dstAlphaBlendFactor;
		BlendOperation		alphaBlendOp;
		ColorComponentFlags colorWriteMask;
	};

	struct BlendStateDesc {
		std::vector<BlendAttachmentDesc>	attachments;
		vec4f								blendConstants;
	};

	struct RasterizerStateDesc {
		bool				wireframe;
		RasterizerCullMode	cullMode;
		bool				clockwise;
	};

	struct StencilStateDesc {
		StencilOperation   failOp;
		StencilOperation   passOp;
		StencilOperation   depthFailOp;
		CompareOperation   compareOp;
	};

	struct DepthStencilStateDesc {
		bool					depthTestEnabled;
		bool					depthWriteEnabled;
		CompareOperation		depthCompareOp;
		bool					stencilTestEnabled;
		ui32					readMask;
		ui32					writeMask;
		StencilStateDesc		front;
		StencilStateDesc		back;
	};

	// Format functions

	bool graphics_format_has_stencil(Format format);
	constexpr ui32 graphics_format_size(Format format);

	// Adapters

	class Adapter {
	protected:
		ui32 m_Suitability = 0u;

	public:
		inline ui32 GetSuitability() const noexcept { return m_Suitability; }

	};

	// Primitives

	SV_DEFINE_HANDLE(Primitive);

	struct GPUBuffer : public Primitive { GPUBuffer() = delete; ~GPUBuffer() = delete; };
	struct GPUImage : public Primitive { GPUImage() = delete; ~GPUImage() = delete; };
	struct Sampler : public Primitive { Sampler() = delete; ~Sampler() = delete; };
	struct Shader : public Primitive { Shader() = delete; ~Shader() = delete; };
	struct RenderPass : public Primitive { RenderPass() = delete; ~RenderPass() = delete; };
	struct InputLayoutState : public Primitive { InputLayoutState() = delete; ~InputLayoutState() = delete; };
	struct BlendState : public Primitive { BlendState() = delete; ~BlendState() = delete; };
	struct DepthStencilState : public Primitive { DepthStencilState() = delete; ~DepthStencilState() = delete; };
	struct RasterizerState : public Primitive { RasterizerState() = delete; ~RasterizerState() = delete; };

	typedef ui32 CommandList;

	void graphics_present(GPUImage* image, const GPUImageRegion& region, GPUImageLayout layout, CommandList cmd);

	GraphicsAPI graphics_api_get();

	// Adapers

	const std::vector<std::unique_ptr<Adapter>>& graphics_adapter_get_list() noexcept;
	Adapter* graphics_adapter_get() noexcept;
	void graphics_adapter_set(ui32 index);

	// Hash functions

	size_t graphics_compute_hash_inputlayoutstate(const InputLayoutStateDesc* desc);
	size_t graphics_compute_hash_blendstate(const BlendStateDesc* desc);
	size_t graphics_compute_hash_rasterizerstate(const RasterizerStateDesc* desc);
	size_t graphics_compute_hash_depthstencilstate(const DepthStencilStateDesc* desc);

	// Primitives

	Result graphics_buffer_create(const GPUBufferDesc* desc, GPUBuffer** buffer);
	Result graphics_shader_create(const ShaderDesc* desc, Shader** shader);
	Result graphics_image_create(const GPUImageDesc* desc, GPUImage** image);
	Result graphics_sampler_create(const SamplerDesc* desc, Sampler** sampler);
	Result graphics_renderpass_create(const RenderPassDesc* desc, RenderPass** renderPass);
	Result graphics_inputlayoutstate_create(const InputLayoutStateDesc* desc, InputLayoutState** inputLayoutState);
	Result graphics_blendstate_create(const BlendStateDesc* desc, BlendState** blendState);
	Result graphics_depthstencilstate_create(const DepthStencilStateDesc* desc, DepthStencilState** depthStencilState);
	Result graphics_rasterizerstate_create(const RasterizerStateDesc* desc, RasterizerState** rasterizerState);

	Result graphics_destroy(Primitive* primitive);

	// CommandList functions

	CommandList graphics_commandlist_begin();
	CommandList graphics_commandlist_last();
	CommandList graphics_commandlist_get();
	ui32		graphics_commandlist_count();

	void graphics_gpu_wait();

	// Resource functions

	void graphics_resources_unbind(CommandList cmd);

	void graphics_vertexbuffer_bind_array(GPUBuffer** buffers, ui32* offsets, ui32 count, ui32 beginSlot, CommandList cmd);
	void graphics_vertexbuffer_bind(GPUBuffer* buffer, ui32 offset, ui32 slot, CommandList cmd);
	void graphics_vertexbuffer_unbind(ui32 slot, CommandList cmd);
	void graphics_vertexbuffer_unbind_commandlist(CommandList cmd);
	
	void graphics_indexbuffer_bind(GPUBuffer* buffer, ui32 offset, CommandList cmd);
	void graphics_indexbuffer_unbind(CommandList cmd);

	void graphics_constantbuffer_bind_array(GPUBuffer** buffers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd);
	void graphics_constantbuffer_bind(GPUBuffer* buffer, ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_constantbuffer_unbind(ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_constantbuffer_unbind_shader(ShaderType shaderType, CommandList cmd);
	void graphics_constantbuffer_unbind_commandlist(CommandList cmd);

	void graphics_image_bind_array(GPUImage** images, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd);
	void graphics_image_bind(GPUImage* image, ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_image_unbind(ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_image_unbind_shader(ShaderType shaderType, CommandList cmd);
	void graphics_image_unbind_commandlist(CommandList cmd);

	void graphics_sampler_bind_array(Sampler** samplers, ui32 count, ui32 beginSlot, ShaderType shaderType, CommandList cmd);
	void graphics_sampler_bind(Sampler* sampler, ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_sampler_unbind(ui32 slot, ShaderType shaderType, CommandList cmd);
	void graphics_sampler_unbind_shader(ShaderType shaderType, CommandList cmd);
	void graphics_sampler_unbind_commandlist(CommandList cmd);

	// State functions

	void graphics_state_unbind(CommandList cmd);

	void graphics_shader_bind(Shader* shader, CommandList cmd);
	void graphics_inputlayoutstate_bind(InputLayoutState* inputLayoutState, CommandList cmd);
	void graphics_blendstate_bind(BlendState* blendState, CommandList cmd);
	void graphics_depthstencilstate_bind(DepthStencilState* depthStencilState, CommandList cmd);
	void graphics_rasterizerstate_bind(RasterizerState* rasterizerState, CommandList cmd);

	void graphics_shader_unbind(ShaderType shaderType, CommandList cmd);
	void graphics_shader_unbind_commandlist(CommandList cmd);
	void graphics_inputlayoutstate_unbind(CommandList cmd);
	void graphics_blendstate_unbind(CommandList cmd);
	void graphics_depthstencilstate_unbind(CommandList cmd);
	void graphics_rasterizerstate_unbind(CommandList cmd);

	void graphics_mode_set(GraphicsPipelineMode mode, CommandList cmd);
	void graphics_topology_set(GraphicsTopology topology, CommandList cmd);
	void graphics_stencil_reference_set(ui32 ref, CommandList cmd);
	void graphics_line_width_set(float lineWidth, CommandList cmd);

	GraphicsPipelineMode	graphics_mode_get(CommandList cmd);
	GraphicsTopology		graphics_topology_get(CommandList cmd);
	ui32					graphics_stencil_reference_get(CommandList cmd);
	float					graphics_line_width_get(CommandList cmd);

	void graphics_viewport_set(const Viewport* viewports, ui32 count, CommandList cmd);
	void graphics_viewport_set(const Viewport& viewport, ui32 slot, CommandList cmd);
	void graphics_viewport_set(GPUImage* image, ui32 slot, CommandList cmd);
	void graphics_scissor_set(const Scissor* scissors, ui32 count, CommandList cmd);
	void graphics_scissor_set(const Scissor& scissor, ui32 slot, CommandList cmd);
	void graphics_scissor_set(GPUImage* image, ui32 slot, CommandList cmd);

	Viewport graphics_viewport_get(ui32 slot, CommandList cmd);
	Scissor	 graphics_scissor_get(ui32 slot, CommandList cmd);

	// RenderPass functions

	void graphics_renderpass_begin(RenderPass* renderPass, GPUImage** attachments, const Color4f* colors, float depth, ui32 stencil, CommandList cmd);
	void graphics_renderpass_end(CommandList cmd);

	// Draw Calls

	void graphics_draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd);
	void graphics_draw_indexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd);

	// Memory

	void graphics_buffer_update(GPUBuffer* buffer, void* pData, ui32 size, ui32 offset, CommandList cmd);
	void graphics_barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd);
	void graphics_image_blit(GPUImage* src, GPUImage* dst, GPUImageLayout srcLayout, GPUImageLayout dstLayout, ui32 count, const GPUImageBlit* imageBlit, SamplerFilter filter, CommandList cmd);
	void graphics_image_clear(GPUImage* image, GPUImageLayout oldLayout, GPUImageLayout newLayout, const Color4f& clearColor, float depth, ui32 stencil, CommandList cmd); // Not use if necessary, renderpasses have best performance!!

	// Shader utils

	Result graphics_shader_compile_string(const ShaderCompileDesc* desc, const char* str, ui32 size, std::vector<ui8>& data);
	Result graphics_shader_compile_file(const ShaderCompileDesc* desc, const char* srcPath, std::vector<ui8>& data);

	const ShaderInfo* graphics_shader_info_get(Shader* shader);

	Result graphics_shader_include_write(const char* name, const char* str);

	ui32 graphics_shader_attribute_size(ShaderAttributeType type);

	ShaderType graphics_shader_type(const Shader* shader);

	// Primitive getters

	ui32		graphics_image_get_width(GPUImage* image);
	ui32		graphics_image_get_height(GPUImage* image);
	ui32		graphics_image_get_depth(GPUImage* image);
	ui32		graphics_image_get_layers(GPUImage* image);
	ui32		graphics_image_get_dimension(GPUImage* image);
	Format		graphics_image_get_format(GPUImage* image);
	Viewport	graphics_image_get_viewport(GPUImage* image);
	Scissor		graphics_image_get_scissor(GPUImage* image);

	// Assets

	class TextureAsset : public Asset {
	public:
		inline GPUImage* get() const noexcept { GPUImage** ptr = reinterpret_cast<GPUImage**>(m_Ref.get()); return ptr ? *ptr : nullptr; }
	};

	// Properties

	struct GraphicsProperties {
	};

	GraphicsProperties graphics_properties_get();

}