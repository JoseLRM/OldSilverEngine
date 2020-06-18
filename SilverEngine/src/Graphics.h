#pragma once

#include "core.h"
#include "GraphicsDesc.h"
#include "EngineDevice.h"

struct SV_GRAPHICS_INITIALIZATION_DESC {

};

namespace SV {

	class Graphics;
	class Graphics_dx11;
	
	// CommandList
	typedef ui32 CommandList;

	/////////////////////////////PRIMITIVES///////////////////////
	namespace _internal {

		class GPUResource {
			SV_GFX_USAGE m_Usage;
			ui32 m_Size;
			SV_GFX_CPU_ACCESS m_CPUAccess;

		public:
			friend Graphics;
			friend Graphics_dx11;

			inline SV_GFX_USAGE GetUsage() const noexcept { return m_Usage; }
			inline ui32 GetSize() const noexcept { return m_Size; }
			inline SV_GFX_CPU_ACCESS GetCPUAccess() const noexcept { return m_CPUAccess; }

		};

		class VertexBuffer_internal : public GPUResource {};
		class IndexBuffer_internal : public GPUResource {};
		class ConstantBuffer_internal : public GPUResource {};

		class FrameBuffer_internal {
			ui32 m_Width = 0;
			ui32 m_Height = 0;
			SV_GFX_FORMAT m_Format;
			
			bool m_TextureUsage;

		public:
			friend Graphics;
			friend Graphics_dx11;

			inline ui32 GetWidth() const noexcept { return m_Width; }
			inline ui32 GetHeight() const noexcept { return m_Height; }
			inline SV_GFX_FORMAT GetFormat() const noexcept { return m_Format; }
			inline bool HasTextureUsage() const noexcept { return m_TextureUsage; }

			virtual ui64 GetResouceID() = 0;

		};

		class Shader_internal {
			SV_GFX_SHADER_TYPE m_ShaderType;

		public:
			friend Graphics;
			friend Graphics_dx11;

			inline SV_GFX_SHADER_TYPE GetType() const noexcept { return m_ShaderType; }

		};

		class InputLayout_internal {
		public:
			friend Graphics;
			friend Graphics_dx11;

		};

		class Texture_internal {
			SV_GFX_FORMAT m_Format;
			SV_GFX_USAGE m_Usage;
			SV_GFX_CPU_ACCESS m_CPUAccess;
			ui32 m_Width;
			ui32 m_Height;
			ui32 m_Size;

		public:
			friend Graphics;
			friend Graphics_dx11;

			inline SV_GFX_FORMAT GetFormat() const noexcept { return m_Format; }
			inline SV_GFX_USAGE GetUsage() const noexcept { return m_Usage; }
			inline SV_GFX_CPU_ACCESS GetCPUAccess() const noexcept { return m_CPUAccess; }
			inline ui32 GetWidth() const noexcept { return m_Width; }
			inline ui32 GetHeight() const noexcept { return m_Height; }
			inline ui32 GetSize() const noexcept { return m_Size; }

			virtual ui64 GetResouceID() = 0;

		};

		class Sampler_internal {
		public:
			friend Graphics;
			friend Graphics_dx11;

		};

		class BlendState_internal {
		public:
			friend Graphics;
			friend Graphics_dx11;

		};

		template<typename T>
		class Primitive_internal {
			std::unique_ptr<T> m_Allocation;
		public:
			inline T* operator->() const noexcept { return m_Allocation.get(); }
			inline bool IsValid() const noexcept { return m_Allocation.get(); }
			inline T* Get() const noexcept { return m_Allocation.get(); }
			inline void Set(std::unique_ptr<T> alloc) noexcept { m_Allocation = std::move(alloc); }

		};

	}
	//////////////////////////////////////////////////////////////

	class VertexBuffer		: public _internal::Primitive_internal<_internal::VertexBuffer_internal> {};
	class IndexBuffer		: public _internal::Primitive_internal<_internal::IndexBuffer_internal> {};
	class ConstantBuffer	: public _internal::Primitive_internal<_internal::ConstantBuffer_internal> {};
	class FrameBuffer		: public _internal::Primitive_internal<_internal::FrameBuffer_internal> {};
	class Shader			: public _internal::Primitive_internal<_internal::Shader_internal> {};
	class InputLayout		: public _internal::Primitive_internal<_internal::InputLayout_internal> {};
	class Texture			: public _internal::Primitive_internal<_internal::Texture_internal> {};
	class Sampler			: public _internal::Primitive_internal<_internal::Sampler_internal> {};
	class BlendState		: public _internal::Primitive_internal<_internal::BlendState_internal> {};

	class Graphics : public SV::EngineDevice {

		SV::Adapter m_Adapter;
		ui32 m_OutputModeID = 0u;

		bool m_Fullscreen = false;
		
		bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc);
		bool Close();
		virtual bool _Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) = 0;
		virtual bool _Close() = 0;

#ifdef SV_IMGUI
		virtual void ResizeBackBuffer(ui32 width, ui32 height) = 0;
#endif

		void BeginFrame();
		void EndFrame();

	public:
		friend Engine;
		friend Window;

		Graphics();
		~Graphics();

		void SetFullscreen(bool fullscreen);
		inline bool InFullscreen() const noexcept { return m_Fullscreen; }

		inline const SV::Adapter& GetAdapter() const noexcept { return m_Adapter; }
		inline ui32 GetOutputModeID() const noexcept { return m_OutputModeID; }

		virtual void Present() = 0;
		virtual CommandList BeginCommandList() = 0;
		virtual void SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList cmd) = 0;
		virtual void SetTopology(SV_GFX_TOPOLOGY topology, CommandList cmd) = 0;

		virtual SV::FrameBuffer& GetMainFrameBuffer() = 0;

		// Draw calls
		virtual void Draw(ui32 vertexCount, ui32 startVertex, CommandList cmd) = 0;
		virtual void DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList cmd) = 0;
		virtual void DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList cmd) = 0;
		virtual void DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd) = 0;

		// PRIMITIVES
		void ResetState(CommandList cmd);

		// VertexBuffer
		bool CreateVertexBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, VertexBuffer& vb);
		void ReleaseVertexBuffer(VertexBuffer& vb);

		void UpdateVertexBuffer(void* data, ui32 size, VertexBuffer& vb, CommandList cmd);

		void BindVertexBuffer(ui32 slot, ui32 stride, ui32 offset, VertexBuffer& vb, CommandList cmd);
		void BindVertexBuffers(ui32 slot, ui32 count, const ui32* strides, const ui32* offset, VertexBuffer** vbs, CommandList cmd);

		void UnbindVertexBuffer(ui32 slot, CommandList cmd);
		void UnbindVertexBuffers(CommandList cmd);

		// IndexBuffer
		bool CreateIndexBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, IndexBuffer& ib);
		void ReleaseIndexBuffer(IndexBuffer& ib);

		void UpdateIndexBuffer(void* data, ui32 size, IndexBuffer& ib, CommandList cmd);

		void BindIndexBuffer(SV_GFX_FORMAT format, ui32 offset, IndexBuffer& ib, CommandList cmd);
		void UnbindIndexBuffer(CommandList cmd);

		// ConstantBuffer
		bool CreateConstantBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, ConstantBuffer& cb);
		void ReleaseConstantBuffer(ConstantBuffer& cb);

		void UpdateConstantBuffer(void* data, ui32 size, ConstantBuffer& cb, CommandList cmd);

		void BindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, ConstantBuffer& cb, CommandList cmd);
		void BindConstantBuffers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, ConstantBuffer** cbs, CommandList cmd);

		void UnbindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindConstantBuffers(SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindConstantBuffers(CommandList cmd);

		// FrameBuffers
		bool CreateFrameBuffer(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, FrameBuffer& fb);
		void ReleaseFrameBuffer(FrameBuffer& fb);

		bool ResizeFrameBuffer(ui32 width, ui32 height, FrameBuffer& fb);

		void ClearFrameBuffer(SV::Color4f color, FrameBuffer& fb, CommandList cmd);

		void BindFrameBuffer(FrameBuffer& fb, CommandList cmd);
		void BindFrameBuffers(ui32 count, FrameBuffer** fbs, CommandList cmd);

		void BindFrameBufferAsTexture(ui32 slot, SV_GFX_SHADER_TYPE type, FrameBuffer& fb, CommandList cmd);
		
		void UnbindFrameBuffers(CommandList cmd);

		// Shaders
		bool CreateShader(SV_GFX_SHADER_TYPE type, const char* filePath, Shader& s);
		void ReleaseShader(Shader& s);

		void BindShader(Shader& s, CommandList cmd);
		void UnbindShader(SV_GFX_SHADER_TYPE type, CommandList cmd);

		// Input Layout
		bool CreateInputLayout(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, InputLayout& il);
		void ReleaseInputLayout(InputLayout& il);

		void BindInputLayout(InputLayout& il, CommandList cmd);
		void UnbindInputLayout(CommandList cmd);

		// Texture
		bool CreateTexture(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, Texture& t);
		void ReleaseTexture(Texture& t);
		
		bool ResizeTexture(void* data, ui32 width, ui32 height, Texture& t);

		void UpdateTexture(void* data, ui32 size, Texture& t, CommandList cmd);

		void BindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, Texture& t, CommandList cmd);
		void BindTextures(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Texture** ts, CommandList cmd);

		void UnbindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindTextures(SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindTextures(CommandList cmd);

		// Sampler
		bool CreateSampler(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Sampler& s);
		void ReleaseSampler(Sampler& s);

		void BindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, Sampler& s, CommandList cmd);
		void BindSamplers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Sampler** ss, CommandList cmd);

		void UnbindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindSamplers(SV_GFX_SHADER_TYPE type, CommandList cmd);
		void UnbindSamplers(CommandList cmd);
		
		// BlendState
		bool CreateBlendState(const SV_GFX_BLEND_STATE_DESC* desc, BlendState& bs);
		void ReleaseBlendState(BlendState& bs);

		void BindBlendState(ui32 sampleMask, const float* blendFactors, BlendState& bs, CommandList cmd);
		void BindBlendState(ui32 sampleMask, BlendState& bs, CommandList cmd);
		void BindBlendState(const float* blendFactors, BlendState& bs, CommandList cmd);
		void BindBlendState(BlendState& bs, CommandList cmd);

		void UnbindBlendState(CommandList cmd);

		// INTERNAL PRIMITIVES
	private:
		// Allocate Instances
		virtual std::unique_ptr<_internal::VertexBuffer_internal> _AllocateVertexBuffer() = 0;
		virtual std::unique_ptr<_internal::IndexBuffer_internal> _AllocateIndexBuffer() = 0;
		virtual std::unique_ptr<_internal::ConstantBuffer_internal> _AllocateConstantBuffer() = 0;
		virtual std::unique_ptr<_internal::FrameBuffer_internal> _AllocateFrameBuffer() = 0;
		virtual std::unique_ptr<_internal::Shader_internal> _AllocateShader() = 0;
		virtual std::unique_ptr<_internal::InputLayout_internal> _AllocateInputLayout() = 0;
		virtual std::unique_ptr<_internal::Texture_internal> _AllocateTexture() = 0;
		virtual std::unique_ptr<_internal::Sampler_internal> _AllocateSampler() = 0;
		virtual std::unique_ptr<_internal::BlendState_internal> _AllocateBlendState() = 0;

		virtual void _ResetState(CommandList cmd) = 0;

		// Internal VertexBuffer
		virtual bool _CreateVertexBuffer(void* data, VertexBuffer& vb) = 0;
		virtual void _ReleaseVertexBuffer(VertexBuffer& vb) = 0;

		virtual void _UpdateVertexBuffer(void* data, ui32 size, VertexBuffer& vb, CommandList cmd) = 0;

		virtual void _BindVertexBuffer(ui32 slot, ui32 stride, ui32 offset, VertexBuffer& vb, CommandList cmd) = 0;
		virtual void _BindVertexBuffers(ui32 slot, ui32 count, const ui32* strides, const ui32* offset, VertexBuffer** vbs, CommandList cmd) = 0;

		virtual void _UnbindVertexBuffer(ui32 slot, CommandList cmd) = 0;
		virtual void _UnbindVertexBuffers(CommandList cmd) = 0;

		// Internal IndexBuffer
		virtual bool _CreateIndexBuffer(void* data, IndexBuffer& ib) = 0;
		virtual void _ReleaseIndexBuffer(IndexBuffer& ib) = 0;

		virtual void _UpdateIndexBuffer(void* data, ui32 size, IndexBuffer& ib, CommandList cmd) = 0;

		virtual void _BindIndexBuffer(SV_GFX_FORMAT format, ui32 offset, IndexBuffer& ib, CommandList cmd) = 0;
		virtual void _UnbindIndexBuffer(CommandList cmd) = 0;

		// Internal ConstantBuffer
		virtual bool _CreateConstantBuffer(void* data, ConstantBuffer& cb) = 0;
		virtual void _ReleaseConstantBuffer(ConstantBuffer& cb) = 0;

		virtual void _UpdateConstantBuffer(void* data, ui32 size, ConstantBuffer& cb, CommandList cmd) = 0;

		virtual void _BindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, ConstantBuffer& cb, CommandList cmd) = 0;
		virtual void _BindConstantBuffers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, ConstantBuffer** cbs, CommandList cmd) = 0;

		virtual void _UnbindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindConstantBuffers(SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindConstantBuffers(CommandList cmd) = 0;
		
		// Internal FrameBuffers
		virtual bool _CreateFrameBuffer(FrameBuffer& fb) = 0;
		virtual void _ReleaseFrameBuffer(FrameBuffer& fb) = 0;

		virtual void _ClearFrameBuffer(SV::Color4f color, FrameBuffer& fb, CommandList cmd) = 0;

		virtual void _BindFrameBuffer(FrameBuffer& fb, CommandList cmd) = 0;
		virtual void _BindFrameBuffers(ui32 count, FrameBuffer** fbs, CommandList cmd) = 0;

		virtual void _BindFrameBufferAsTexture(ui32 slot, SV_GFX_SHADER_TYPE type, FrameBuffer& fb, CommandList cmd) = 0;

		virtual void _UnbindFrameBuffers(CommandList cmd) = 0;

		// Internal Shaders
		virtual bool _CreateShader(const char* filePath, Shader& s) = 0;
		virtual void _ReleaseShader(Shader& s) = 0;

		virtual void _BindShader(Shader& s, CommandList cmd) = 0;
		virtual void _UnbindShader(SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;

		// Internal Input Layout
		virtual bool _CreateInputLayout(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, InputLayout& il) = 0;
		virtual void _ReleaseInputLayout(InputLayout& il) = 0;
 
		virtual void _BindInputLayout(InputLayout& il, CommandList cmd) = 0;
		virtual void _UnbindInputLayout(CommandList cmd) = 0;

		// Internal Texture
		virtual bool _CreateTexture(void* data, Texture& t) = 0;
		virtual void _ReleaseTexture(Texture& t) = 0;

		virtual void _UpdateTexture(void* data, ui32 size, Texture& t, CommandList cmd) = 0;

		virtual void _BindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, Texture& t, CommandList cmd) = 0;
		virtual void _BindTextures(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Texture** ts, CommandList cmd) = 0;

		virtual void _UnbindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindTextures(SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindTextures(CommandList cmd) = 0;

		// Internal Sampler
		virtual bool _CreateSampler(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Sampler& s) = 0;
		virtual void _ReleaseSampler(Sampler& s) = 0;

		virtual void _BindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, Sampler& s, CommandList cmd) = 0;
		virtual void _BindSamplers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Sampler** ss, CommandList cmd) = 0;

		virtual void _UnbindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindSamplers(SV_GFX_SHADER_TYPE type, CommandList cmd) = 0;
		virtual void _UnbindSamplers(CommandList cmd) = 0;

		// Internal BlendState
		virtual bool _CreateBlendState(const SV_GFX_BLEND_STATE_DESC* desc, BlendState& bs) = 0;
		virtual void _ReleaseBlendState(BlendState& bs) = 0;

		virtual void _BindBlendState(ui32 sampleMask, const float* blendFactors, BlendState& bs, CommandList cmd) = 0;
		virtual void _UnbindBlendState(CommandList cmd) = 0;

	private:
		virtual void EnableFullscreen() = 0;
		virtual void DisableFullscreen() = 0;

	};

}