#pragma once

#include "..//core.h"
#include "graphics_desc.h"
#include "graphics_allocator.h"
#include "graphics_shader_utils.h"

struct SV_GRAPHICS_INITIALIZATION_DESC {

};

namespace _sv {

	struct PipelineState;

	// Primitive Parent
	struct Primitive_internal {
	private:
		SV_GFX_PRIMITIVE m_Type;
	public:

		inline void SetType(SV_GFX_PRIMITIVE type) noexcept { m_Type = type; }
		inline SV_GFX_PRIMITIVE GetType() const noexcept { return m_Type; }
	};
	// Buffer
	struct Buffer_internal : public Primitive_internal {
	private:
		SV_GFX_BUFFER_DESC m_Desc; // TODO: No desc
	public:
		inline SV_GFX_BUFFER_TYPE GetBufferType() const noexcept { return m_Desc.bufferType; }
		inline SV_GFX_USAGE GetUsage() const noexcept { return m_Desc.usage; }
		inline ui32 GetSize() const noexcept { return m_Desc.size; }
		inline SV_GFX_INDEX_TYPE GetIndexType() const noexcept { return m_Desc.indexType; }
		inline sv::CPUAccessFlags GetCPUAccess() const noexcept { return m_Desc.CPUAccess; }

		void SetDescription(const SV_GFX_BUFFER_DESC& d) noexcept { m_Desc = d; }
	};
	// Image
	struct Image_internal : public Primitive_internal {
	private:
		ui8						m_Dimension;
		SV_GFX_FORMAT			m_Format;
		ui32					m_Width;
		ui32					m_Height;
		ui32					m_Depth;
		ui32					m_Layers;
		sv::ImageTypeFlags		m_ImageType;

	public:
		inline ui8 GetDimension() const noexcept { return m_Dimension; }
		inline SV_GFX_FORMAT GetFormat() const noexcept { return m_Format; }
		inline ui32 GetWidth() const noexcept { return m_Width; }
		inline ui32 GetHeight() const noexcept { return m_Height; }
		inline ui32 GetDepth() const noexcept { return m_Depth; }
		inline ui32 GetLayers() const noexcept { return m_Layers; }
		inline sv::ImageTypeFlags GetImageType() const noexcept { return m_ImageType; }
		sv::Viewport GetViewport() const noexcept;
		sv::Scissor GetScissor() const noexcept;

		void SetDescription(const SV_GFX_IMAGE_DESC& d) noexcept;
	};
	// Sampler
	struct Sampler_internal : public Primitive_internal {

	public:
		void SetDescription(const SV_GFX_SAMPLER_DESC& d) noexcept {}

	};
	// Shader
	struct Shader_internal : public Primitive_internal {
	private:
		SV_GFX_SHADER_TYPE m_ShaderType;
		std::string m_FilePath;
	public:
		inline SV_GFX_SHADER_TYPE GetShaderType() const noexcept { return m_ShaderType; }
		inline const char* GetFilePath() const noexcept { return m_FilePath.c_str(); }

		void SetDescription(const SV_GFX_SHADER_DESC& d) noexcept {
			m_ShaderType = d.shaderType;
			m_FilePath = d.filePath;
		}
	};
	// RenderPass
	struct RenderPass_internal : public Primitive_internal {
	private:
		ui32								m_DepthStencilAttachment;
		std::vector<SV_GFX_ATTACHMENT_DESC>	m_Attachments;
	public:
		inline ui32 GetAttachmentsCount() const noexcept { return ui32(m_Attachments.size()); }
		inline bool HasDepthStencilAttachment() const noexcept { return m_DepthStencilAttachment != GetAttachmentsCount(); }
		inline ui32 GetDepthStencilAttachment() const noexcept { return m_DepthStencilAttachment; }
		inline const SV_GFX_ATTACHMENT_DESC* GetAttachments() const noexcept { return m_Attachments.data(); }

		void SetDescription(const SV_GFX_RENDERPASS_DESC& d) noexcept;
	};
	// Graphics Pipeline
	struct GraphicsPipeline_internal : public Primitive_internal {
	private:
		Shader_internal* m_VS;
		Shader_internal* m_PS;
		Shader_internal* m_GS;
		SV_GFX_INPUT_LAYOUT_DESC					m_InputLayout;
		SV_GFX_RASTERIZER_STATE_DESC				m_RasterizerState;
		SV_GFX_BLEND_STATE_DESC						m_BlendState;
		SV_GFX_DEPTHSTENCIL_STATE_DESC				m_DepthStencilState;
		SV_GFX_TOPOLOGY								m_Topology;

	public:
		inline Shader_internal* GetVertexShader() const noexcept { return m_VS; }
		inline Shader_internal* GetPixelShader() const noexcept { return m_PS; }
		inline Shader_internal* GetGeometryShader() const noexcept { return m_GS; }

		inline const SV_GFX_INPUT_LAYOUT_DESC& GetInputLayout() const noexcept { return m_InputLayout; }
		inline const SV_GFX_RASTERIZER_STATE_DESC& GetRasterizerState() const noexcept { return m_RasterizerState; }
		inline const SV_GFX_BLEND_STATE_DESC& GetBlendState() const noexcept { return m_BlendState; }
		inline const SV_GFX_DEPTHSTENCIL_STATE_DESC& GetDepthStencilState() const noexcept { return m_DepthStencilState; }

		inline SV_GFX_TOPOLOGY GetTopology() const noexcept { return m_Topology; }

		void SetDescription(const SV_GFX_GRAPHICS_PIPELINE_DESC& d) noexcept;
	};

}

namespace sv {

	class Adapter {
	protected:
		ui32 m_Suitability = 0u;

	public:
		inline ui32 GetSuitability() const noexcept { return m_Suitability; }

	};

	struct Primitive {
	protected:
		void* ptr = nullptr;

	public:
		Primitive() = default;
		Primitive(void* ptr) : ptr(ptr) {}

		// Remove copy operator
		Primitive& operator=(const Primitive& other) = delete;
		Primitive(const Primitive& other) = delete;

		// Move operator
		Primitive& operator=(Primitive&& other) noexcept
		{
			ptr = other.ptr;
			other.ptr = nullptr;
			return *this;
		}
		Primitive(Primitive&& other) noexcept { this->operator=(std::move(other)); }

		inline bool IsValid() const noexcept { return ptr != nullptr; }
   		inline void* GetPtr() const noexcept { return ptr; }

		_sv::Primitive_internal* operator->() const noexcept { return reinterpret_cast<_sv::Primitive_internal*>(ptr); }

	};

	struct Buffer : public Primitive {
		_sv::Buffer_internal* operator->() const noexcept { return reinterpret_cast<_sv::Buffer_internal*>(ptr); }
	};
	struct Image : public Primitive {
		_sv::Image_internal* operator->() const noexcept { return reinterpret_cast<_sv::Image_internal*>(ptr); }
	};
	struct Sampler : public Primitive {
		_sv::Sampler_internal* operator->() const noexcept { return reinterpret_cast<_sv::Sampler_internal*>(ptr); }
	};
	struct Shader : public Primitive {
		_sv::Shader_internal* operator->() const noexcept { return reinterpret_cast<_sv::Shader_internal*>(ptr); }
	};
	struct RenderPass : public Primitive {
		_sv::RenderPass_internal* operator->() const noexcept { return reinterpret_cast<_sv::RenderPass_internal*>(ptr); }
	};
	struct GraphicsPipeline : public Primitive {
		_sv::GraphicsPipeline_internal* operator->() const noexcept { return reinterpret_cast<_sv::GraphicsPipeline_internal*>(ptr); }
	};

	typedef ui32 CommandList;

}

namespace _sv {

	class GraphicsDevice {
	public:
		virtual bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) = 0;
		virtual bool Close() = 0;

		virtual sv::CommandList BeginCommandList() = 0;
		virtual sv::CommandList GetLastCommandList() = 0;

		virtual void BeginRenderPass(sv::CommandList cmd) = 0;
		virtual void EndRenderPass(sv::CommandList cmd) = 0;

		virtual void ResizeSwapChain() = 0;
		virtual sv::Image& AcquireSwapChainImage() = 0;
		virtual void WaitGPU() = 0;

		virtual void BeginFrame() = 0;
		virtual void SubmitCommandLists() = 0;
		virtual void Present() = 0;

		virtual void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, sv::CommandList cmd) = 0;
		virtual void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, sv::CommandList cmd) = 0;

		virtual void UpdateBuffer(sv::Buffer& buffer, void* pData, ui32 size, ui32 offset, sv::CommandList cmd) = 0;
		virtual void Barrier(const sv::GPUBarrier* barriers, ui32 count, sv::CommandList cmd) = 0;

	};

	bool graphics_initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc);
	bool graphics_close();

	void graphics_begin();
	void graphics_commandlist_submit();
	void graphics_present();

	void graphics_swapchain_resize();
	sv::Image& graphics_swapchain_acquire_image();

	GraphicsDevice* graphics_device_get() noexcept;
	PrimitiveAllocator& graphics_allocator_get() noexcept;
	PipelineState& graphics_state_get() noexcept;

	void graphics_adapter_add(std::unique_ptr<sv::Adapter>&& adapter);

	size_t graphics_compute_hash_inputlayout(const SV_GFX_INPUT_LAYOUT_DESC* desc);
	size_t graphics_compute_hash_blendstate(const SV_GFX_BLEND_STATE_DESC* desc);
	size_t graphics_compute_hash_rasterizerstate(const SV_GFX_RASTERIZER_STATE_DESC* desc);
	size_t graphics_compute_hash_depthstencilstate(const SV_GFX_DEPTHSTENCIL_STATE_DESC* desc);

}

namespace sv {

	// ADAPTERS

	const std::vector<std::unique_ptr<Adapter>>& graphics_adapter_get_list() noexcept;
	Adapter* graphics_adapter_get() noexcept;
	void graphics_adapter_set(ui32 index);

	// Primitives

	bool graphics_buffer_create(const SV_GFX_BUFFER_DESC* desc, Buffer& buffer);
	bool graphics_shader_create(const SV_GFX_SHADER_DESC* desc, Shader& shader);
	bool graphics_image_create(const SV_GFX_IMAGE_DESC* desc, Image& image);
	bool graphics_sampler_create(const SV_GFX_SAMPLER_DESC* desc, Sampler& sampler);
	bool graphics_renderpass_create(const SV_GFX_RENDERPASS_DESC* desc, RenderPass& renderPass);
	bool graphics_pipeline_create(const SV_GFX_GRAPHICS_PIPELINE_DESC* desc, GraphicsPipeline& graphicsPipeline);

	bool graphics_destroy(Primitive& primitive);

	CommandList graphics_commandlist_begin();
	CommandList graphics_commandlist_last();

	void graphics_gpu_wait();

	// State Methods

	void graphics_state_reset();

	void graphics_vertexbuffer_bind(Buffer** buffers, ui32* offsets, ui32* strides, ui32 count, CommandList cmd);
	void graphics_indexbuffer_bind(Buffer& buffer, ui32 offset, CommandList cmd);
	void graphics_constantbuffer_bind(Buffer** buffers, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd);
	void graphics_image_bind(Image** images, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd);
	void graphics_sampler_bind(Sampler** samplers, ui32 count, SV_GFX_SHADER_TYPE shaderType, CommandList cmd);
	void graphics_pipeline_bind(GraphicsPipeline& pipeline, CommandList cmd);

	void graphics_renderpass_begin(RenderPass& renderPass, Image** attachments, const Color4f* colors, float depth, ui32 stencil, CommandList cmd);
	void graphics_renderpass_end(CommandList cmd);

	void graphics_set_pipeline_mode(SV_GFX_PIPELINE_MODE mode, CommandList cmd);
	void graphics_set_viewports(const Viewport* viewports, ui32 count, CommandList cmd);
	void graphics_set_scissors(const Scissor* scissors, ui32 count, CommandList cmd);

	// Draw Calls

	void graphics_draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, CommandList cmd);
	void graphics_draw_indexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd);

	// Memory

	void graphics_buffer_update(Buffer& buffer, void* pData, ui32 size, ui32 offset, CommandList cmd);
	void graphics_barrier(const GPUBarrier* barriers, ui32 count, CommandList cmd);

	// Defaults

	constexpr SV_GFX_ATTACHMENT_DESC graphics_renderpass_default_rendertarget(SV_GFX_FORMAT format)
	{
		return { SV_GFX_LOAD_OP_CLEAR, SV_GFX_STORE_OP_STORE, SV_GFX_LOAD_OP_DONT_CARE, SV_GFX_STORE_OP_DONT_CARE,
			format, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET };
	}
	constexpr SV_GFX_ATTACHMENT_DESC graphics_renderpass_default_depthstencil(SV_GFX_FORMAT format)
	{
		return { SV_GFX_LOAD_OP_DONT_CARE, SV_GFX_STORE_OP_DONT_CARE, SV_GFX_LOAD_OP_LOAD, SV_GFX_STORE_OP_STORE,
			format, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL };
	}

	constexpr SV_GFX_RASTERIZER_STATE_DESC graphics_rasterizerstate_default_backculling()
	{
		return { false, 1.f, SV_GFX_CULL_BACK, true };
	}

	constexpr SV_GFX_BLEND_ATTACHMENT_DESC graphics_blendstate_default_opaque()
	{
		return { false, SV_GFX_BLEND_FACTOR_ZERO, SV_GFX_BLEND_FACTOR_ZERO, SV_GFX_BLEND_OP_ADD, 
		SV_GFX_BLEND_FACTOR_ZERO ,SV_GFX_BLEND_FACTOR_ZERO ,SV_GFX_BLEND_OP_ADD, SV_GFX_COLOR_COMPONENT_ALL };
	}

}