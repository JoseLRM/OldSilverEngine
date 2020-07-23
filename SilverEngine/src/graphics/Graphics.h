#pragma once

#include "..//core.h"
#include "GraphicsDesc.h"
#include "PrimitiveAllocator.h"
#include "ShaderCompiler.h"

struct SV_GRAPHICS_INITIALIZATION_DESC {

};

namespace SV {

	class Adapter {
	protected:
		ui32 m_Suitability = 0u;

	public:
		inline ui32 GetSuitability() const noexcept { return m_Suitability; }

	};

	/////////////////////////////PRIMITIVES///////////////////////
	namespace _internal {

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
			inline CPUAccessFlags GetCPUAccess() const noexcept { return m_Desc.CPUAccess; }

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
		public:
			inline ui8 GetDimension() const noexcept { return m_Dimension; }
			inline SV_GFX_FORMAT GetFormat() const noexcept { return m_Format; }
			inline ui32 GetWidth() const noexcept { return m_Width; }
			inline ui32 GetHeight() const noexcept { return m_Height; }
			inline ui32 GetDepth() const noexcept { return m_Depth; }
			inline ui32 GetLayers() const noexcept { return m_Layers; }
			SV_GFX_VIEWPORT GetViewport() const noexcept;
			SV_GFX_SCISSOR GetScissor() const noexcept;

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
			Shader_internal*							m_VS;
			Shader_internal*							m_PS;
			Shader_internal*							m_GS;
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
	//////////////////////////////////////////////////////////////
	
	struct Primitive { 
	protected:
		void* ptr				= nullptr;
		
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

		_internal::Primitive_internal* operator->() const noexcept { return reinterpret_cast<_internal::Primitive_internal*>(ptr); }

	};

	struct Buffer				: public Primitive {
		_internal::Buffer_internal* operator->() const noexcept { return reinterpret_cast<_internal::Buffer_internal*>(ptr); }
	};
	struct Image				: public Primitive {
		_internal::Image_internal* operator->() const noexcept { return reinterpret_cast<_internal::Image_internal*>(ptr); }
	};
	struct Sampler : public Primitive {
		_internal::Sampler_internal* operator->() const noexcept { return reinterpret_cast<_internal::Sampler_internal*>(ptr); }
	};
	struct Shader				: public Primitive {
		_internal::Shader_internal* operator->() const noexcept { return reinterpret_cast<_internal::Shader_internal*>(ptr); }
	};
	struct RenderPass			: public Primitive {
		_internal::RenderPass_internal* operator->() const noexcept { return reinterpret_cast<_internal::RenderPass_internal*>(ptr); }
	};
	struct GraphicsPipeline		: public Primitive {
		_internal::GraphicsPipeline_internal* operator->() const noexcept { return reinterpret_cast<_internal::GraphicsPipeline_internal*>(ptr); }
	};

	typedef ui32 CommandList;

	namespace Graphics {

		namespace _internal {

			// API Interface
			class GraphicsDevice {
			public:
				virtual bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) = 0;
				virtual bool Close() = 0;

				virtual CommandList BeginCommandList() = 0;

				virtual void ResizeSwapChain() = 0;
				virtual SV::Image& GetSwapChainBackBuffer() = 0;

				virtual void BeginFrame() = 0;
				virtual void SubmitCommandLists() = 0;
				virtual void Present() = 0;

				virtual void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, SV::CommandList cmd) = 0;
				virtual void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, SV::CommandList cmd) = 0;

				virtual void UpdateBuffer(SV::Buffer& buffer, void* pData, ui32 size, ui32 offset, SV::CommandList cmd) = 0;

			};

		}

		namespace _internal {
			bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc);
			bool Close();

			void BeginFrame();
			void SubmitCommandLists();
			void Present();

			SV::Graphics::_internal::GraphicsDevice* GetDevice() noexcept;
			SV::_internal::PrimitiveAllocator& GetPrimitiveAllocator() noexcept;
			SV::_internal::PipelineState& GetPipelineState() noexcept;

			void AddAdapter(std::unique_ptr<Adapter>&& adapter);

			size_t ComputeInputLayoutHash(const SV_GFX_INPUT_LAYOUT_DESC* desc);
			size_t ComputeBlendStateHash(const SV_GFX_BLEND_STATE_DESC* desc);
			size_t ComputeRasterizerStateHash(const SV_GFX_RASTERIZER_STATE_DESC* desc);
			size_t ComputeDepthStencilStateHash(const SV_GFX_DEPTHSTENCIL_STATE_DESC* desc);
		}

		// ADAPTERS

		const std::vector<std::unique_ptr<SV::Adapter>>& GetAdapters() noexcept;
		SV::Adapter* GetAdapter() noexcept;
		void SetAdapter(ui32 index);

		// Primitives

		bool CreateBuffer(const SV_GFX_BUFFER_DESC* desc, SV::Buffer& buffer);
		bool CreateShader(const SV_GFX_SHADER_DESC* desc, SV::Shader& shader);
		bool CreateImage(const SV_GFX_IMAGE_DESC* desc, SV::Image& buffer);
		bool CreateSampler(const SV_GFX_SAMPLER_DESC* desc, SV::Sampler& sampler);
		bool CreateRenderPass(const SV_GFX_RENDERPASS_DESC* desc, SV::RenderPass& renderPass);
		bool CreateGraphicsPipeline(const SV_GFX_GRAPHICS_PIPELINE_DESC* desc, SV::GraphicsPipeline& graphicsPipeline);

		bool Destroy(SV::Primitive& primitive);

		SV::CommandList BeginCommandList();

		// State Methods

		void ResetState();

		void BindVertexBuffers(SV::Buffer** buffers, ui32* offsets, ui32* strides, ui32 count, SV::CommandList cmd);
		void BindIndexBuffer(SV::Buffer& buffer, ui32 offset, SV::CommandList cmd);
		void BindConstantBuffers(SV::Buffer** buffers, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd);
		void BindImages(SV::Image** images, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd);
		void BindSamplers(SV::Sampler** samplers, ui32 count, SV_GFX_SHADER_TYPE shaderType, SV::CommandList cmd);

		void BindRenderPass(SV::RenderPass& renderPass, SV::Image** attachments, SV::CommandList cmd);
		void BindGraphicsPipeline(SV::GraphicsPipeline& pipeline, SV::CommandList cmd);

		void SetPipelineMode(SV_GFX_PIPELINE_MODE mode, SV::CommandList cmd);
		void SetViewports(const SV_GFX_VIEWPORT* viewports, ui32 count, SV::CommandList cmd);
		void SetScissors(const SV_GFX_SCISSOR* scissors, ui32 count, SV::CommandList cmd);
		void SetClearColors(const SV::vec4* colors, ui32 count, SV::CommandList cmd);
		void SetClearDepthStencil(float depth, ui32 stencil, SV::CommandList cmd);

		// Draw Calls

		void Draw(ui32 vertexCount, ui32 instanceCount, ui32 startVertex, ui32 startInstance, SV::CommandList cmd);
		void DrawIndexed(ui32 indexCount, ui32 instanceCount, ui32 startIndex, ui32 startVertex, ui32 startInstance, SV::CommandList cmd);

		// SwapChain

		void ResizeSwapChain();
		SV::Image& GetSwapChainBackBuffer();

		// Buffer

		void UpdateBuffer(SV::Buffer& buffer, void* pData, ui32 size, ui32 offset, SV::CommandList cmd);

		// Defaults
		namespace Default {

			constexpr SV_GFX_ATTACHMENT_DESC RenderPassAttDesc_RenderTarget(SV_GFX_FORMAT format)
			{
				return { SV_GFX_LOAD_OP_CLEAR, SV_GFX_STORE_OP_STORE, SV_GFX_LOAD_OP_DONT_CARE, SV_GFX_STORE_OP_DONT_CARE,
					format, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_IMAGE_LAYOUT_RENDER_TARGET, SV_GFX_ATTACHMENT_TYPE_RENDER_TARGET };
			}
			constexpr SV_GFX_ATTACHMENT_DESC RenderPassAttDesc_DepthStencil(SV_GFX_FORMAT format)
			{
				return { SV_GFX_LOAD_OP_DONT_CARE, SV_GFX_STORE_OP_DONT_CARE, SV_GFX_LOAD_OP_LOAD, SV_GFX_STORE_OP_STORE,
					format, SV_GFX_IMAGE_LAYOUT_UNDEFINED, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, SV_GFX_IMAGE_LAYOUT_DEPTH_STENCIL, SV_GFX_ATTACHMENT_TYPE_DEPTH_STENCIL };
			}

			constexpr SV_GFX_RASTERIZER_STATE_DESC RasterizerStateDesc_BackCulling()
			{
				return { false, 1.f, SV_GFX_CULL_BACK, true };
			}

			constexpr SV_GFX_BLEND_ATTACHMENT_DESC BlendAttachment_Opaque()
			{
				return { false, SV_GFX_BLEND_FACTOR_ZERO, SV_GFX_BLEND_FACTOR_ZERO, SV_GFX_BLEND_OP_ADD, 
				SV_GFX_BLEND_FACTOR_ZERO ,SV_GFX_BLEND_FACTOR_ZERO ,SV_GFX_BLEND_OP_ADD, SV_GFX_COLOR_COMPONENT_ALL };
			}

		}
	}

}