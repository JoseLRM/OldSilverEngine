#pragma once

#include "core.h"
#include "GraphicsDesc.h"
#include "EngineDevice.h"

struct SV_GRAPHICS_INITIALIZATION_DESC {

};

namespace SV {

	class Graphics;

	/////////////////////////////PRIMITIVES///////////////////////
	namespace _internal {

		

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

	//class VertexBuffer		: public _internal::Primitive_internal<_internal::VertexBuffer_internal> {};
	//class IndexBuffer		: public _internal::Primitive_internal<_internal::IndexBuffer_internal> {};
	//class ConstantBuffer	: public _internal::Primitive_internal<_internal::ConstantBuffer_internal> {};
	//class FrameBuffer		: public _internal::Primitive_internal<_internal::FrameBuffer_internal> {};
	//class Shader			: public _internal::Primitive_internal<_internal::Shader_internal> {};
	//class InputLayout		: public _internal::Primitive_internal<_internal::InputLayout_internal> {};
	//class Texture			: public _internal::Primitive_internal<_internal::Texture_internal> {};
	//class Sampler			: public _internal::Primitive_internal<_internal::Sampler_internal> {};
	//class BlendState		: public _internal::Primitive_internal<_internal::BlendState_internal> {};
	//class DepthStencilState	: public _internal::Primitive_internal<_internal::DepthStencilState_internal> {};

	class Graphics : public SV::EngineDevice {
	protected:
		SV::Adapter m_Adapter;
		ui32 m_OutputModeID = 0u;

		bool m_Fullscreen = false;
		SV::uvec2 m_LastWindowSize;
		
		bool Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc);
		bool Close();
		virtual bool _Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) = 0;
		virtual bool _Close() = 0;

		void BeginFrame();
		void EndFrame();

	public:
		friend Engine;
		friend Window;

		Graphics();
		~Graphics();

		// Swap chain
	private:
		void ResizeBackBuffer(ui32 width, ui32 height);
		virtual void _ResizeBackBuffer(ui32 width, ui32 height) = 0;

		virtual void EnableFullscreen() = 0;
		virtual void DisableFullscreen() = 0;

	public:
		void SetFullscreen(bool fullscreen);
		inline bool InFullscreen() const noexcept { return m_Fullscreen; }

		// Adapters
		inline const SV::Adapter& GetAdapter() const noexcept { return m_Adapter; }
		inline ui32 GetOutputModeID() const noexcept { return m_OutputModeID; }

		// Draw calls
		virtual void Draw(ui32 vertexCount, ui32 startVertex, CommandList cmd) = 0;
		virtual void DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList cmd) = 0;
		virtual void DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList cmd) = 0;
		virtual void DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd) = 0;

		// PRIMITIVES
		virtual void Present() = 0;
		virtual CommandList BeginCommandList() = 0;
		virtual void SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList cmd) = 0;
		virtual void SetTopology(SV_GFX_TOPOLOGY topology, CommandList cmd) = 0;

		void ResetState(CommandList cmd);


	};

}