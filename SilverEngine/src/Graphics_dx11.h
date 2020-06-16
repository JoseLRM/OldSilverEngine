#pragma once

#include "core.h"
#include "Graphics.h"
#include "Dx11.h"

namespace SV {

	class DirectX11Device;

	class ViewportManager {
		D3D11_VIEWPORT m_Viewports[SV_GFX_VIEWPORT_COUNT];
		bool m_Bind;

	public:
		ViewportManager() { Reset(); }

		inline void SetViewport(ui32 slot, const D3D11_VIEWPORT& vp)
		{
			m_Viewports[slot] = vp;
			m_Bind = true;
		}
		inline void Bind(ID3D11DeviceContext* ctx)
		{
			if (!m_Bind) return;

			ctx->RSSetViewports(SV_GFX_VIEWPORT_COUNT, m_Viewports);

			m_Bind = false;
		}
		inline void Reset() { svZeroMemory(m_Viewports, sizeof(D3D11_VIEWPORT) * SV_GFX_VIEWPORT_COUNT); m_Bind = true; }
	};

	///////////////////////////////VERTEX BUFFER///////////////////////////////////
	struct VertexBuffer_dx11 : public SV::_internal::VertexBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;
	};
	///////////////////////////////INDEX BUFFER///////////////////////////////////
	struct IndexBuffer_dx11 : public SV::_internal::IndexBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;
	};

	///////////////////////////////CONSTANT BUFFER///////////////////////////////////
	struct ConstantBuffer_dx11 : public SV::_internal::ConstantBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;
	};

	///////////////////////////////FRAME BUFFER///////////////////////////////////
	struct FrameBuffer_dx11 : public SV::_internal::FrameBuffer_internal {
		ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
		ComPtr<ID3D11Texture2D> m_Resource;
		ComPtr<ID3D11ShaderResourceView> m_ShaderResouceView;
	};

	/////////////////////////////////SHADER//////////////////////////////////////////
	struct Shader_dx11 : public SV::_internal::Shader_internal {
		void* m_Shader = nullptr;
		ComPtr<ID3DBlob> m_Blob;
	};

	/////////////////////////////////INPUT LAYOUT//////////////////////////////////////////
	struct InputLayout_dx11 : public SV::_internal::InputLayout_internal {
		ComPtr<ID3D11InputLayout> m_InputLayout;
	};

	/////////////////////////////////TEXTURE//////////////////////////////////////////
	struct Texture_dx11 : public SV::_internal::Texture_internal {
		ComPtr<ID3D11Texture2D> m_Texture;
		ComPtr<ID3D11ShaderResourceView> m_ShaderResouceView;
#ifdef SV_IMGUI
		ImTextureID GetImGuiTexture() override
		{
			return m_ShaderResouceView.Get();
		}
#endif
	};

	/////////////////////////////////SAMPLER//////////////////////////////////////////
	struct Sampler_dx11 : public SV::_internal::Sampler_internal {
		ComPtr<ID3D11SamplerState> m_SamplerState;
	};

	/////////////////////////////////SAMPLER//////////////////////////////////////////
	struct BlendState_dx11 : public SV::_internal::BlendState_internal {
		ComPtr<ID3D11BlendState> m_BlendState;
	};

	//TO INTERNAL METHODS
	inline VertexBuffer_dx11& ToInternal(VertexBuffer& vb) {
		return *reinterpret_cast<VertexBuffer_dx11*>(vb.Get());
	}
	inline IndexBuffer_dx11& ToInternal(IndexBuffer& ib) {
		return *reinterpret_cast<IndexBuffer_dx11*>(ib.Get());
	}
	inline ConstantBuffer_dx11& ToInternal(ConstantBuffer& cb) {
		return *reinterpret_cast<ConstantBuffer_dx11*>(cb.Get());
	}
	inline FrameBuffer_dx11& ToInternal(FrameBuffer& fb) {
		return *reinterpret_cast<FrameBuffer_dx11*>(fb.Get());
	}
	inline Shader_dx11& ToInternal(Shader& s) {
		return *reinterpret_cast<Shader_dx11*>(s.Get());
	}
	inline InputLayout_dx11& ToInternal(InputLayout& il) {
		return *reinterpret_cast<InputLayout_dx11*>(il.Get());
	}
	inline Texture_dx11& ToInternal(Texture& t) {
		return *reinterpret_cast<Texture_dx11*>(t.Get());
	}
	inline Sampler_dx11& ToInternal(Sampler& s) {
		return *reinterpret_cast<Sampler_dx11*>(s.Get());
	}
	inline BlendState_dx11& ToInternal(BlendState& bs) {
		return *reinterpret_cast<BlendState_dx11*>(bs.Get());
	}

	/////////////////////////////////DEVICE///////////////////////////////////////////////
	class DirectX11Device : public SV::Graphics {
	public:
		///////////////////Internal allocation/////////////////////////
		ComPtr<ID3D11Device> device;
		ComPtr<IDXGISwapChain> swapChain;

		ComPtr<ID3D11DeviceContext> immediateContext;
		SV::safe_queue<ui32, SV_GFX_COMMAND_LIST_COUNT> activeCommandLists;
		SV::safe_queue<ui32, SV_GFX_COMMAND_LIST_COUNT> freeCommandLists;
		std::atomic<ui32> commandListCount = 0;
		ComPtr<ID3D11DeviceContext>	deferredContext[SV_GFX_COMMAND_LIST_COUNT];

		ViewportManager viewports[SV_GFX_COMMAND_LIST_COUNT];

		FrameBuffer mainFrameBuffer;

		///////////////////////////Methods/////////////////////////
		void CreateBackBuffer(const SV::Adapter::OutputMode& outputMode, ui32 width, ui32 height);
		bool _Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) override;
		bool _Close() override;

		void Present() override;
		CommandList BeginCommandList() override;
		void SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList cmd) override;

		void ResizeBackBuffer(ui32 width, ui32 height);

		void SetTopology(SV_GFX_TOPOLOGY topology, CommandList cmd) override;

		void EnableFullscreen() override;
		void DisableFullscreen() override;

		SV::FrameBuffer& GetMainFrameBuffer() override;

		// Draw calls
		void Draw(ui32 vertexCount, ui32 startVertex, CommandList cmd) override;
		void DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList cmd) override;
		void DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList cmd) override;
		void DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList cmd) override;

		///////////////////////////Primitives///////////////////////////////////
		// Allocate Instances
		std::unique_ptr<_internal::VertexBuffer_internal> _AllocateVertexBuffer() override { return std::make_unique<VertexBuffer_dx11>(); }
		std::unique_ptr<_internal::IndexBuffer_internal> _AllocateIndexBuffer() override { return std::make_unique<IndexBuffer_dx11>(); }
		std::unique_ptr<_internal::ConstantBuffer_internal> _AllocateConstantBuffer() override { return std::make_unique<ConstantBuffer_dx11>(); }
		std::unique_ptr<_internal::FrameBuffer_internal> _AllocateFrameBuffer() override { return std::make_unique<FrameBuffer_dx11>(); }
		std::unique_ptr<_internal::Shader_internal> _AllocateShader() override { return std::make_unique<Shader_dx11>(); }
		std::unique_ptr<_internal::InputLayout_internal> _AllocateInputLayout() override { return std::make_unique<InputLayout_dx11>(); }
		std::unique_ptr<_internal::Texture_internal> _AllocateTexture() override { return std::make_unique<Texture_dx11>(); }
		std::unique_ptr<_internal::Sampler_internal> _AllocateSampler() override { return std::make_unique<Sampler_dx11>(); }
		std::unique_ptr<_internal::BlendState_internal> _AllocateBlendState() override { return std::make_unique<BlendState_dx11>(); }

		void _ResetState(CommandList cmd) override;

		// VertexBuffer
		bool _CreateVertexBuffer(void* data, VertexBuffer& vb) override;
		void _ReleaseVertexBuffer(VertexBuffer& vb) override;

		void _UpdateVertexBuffer(void* data, ui32 size, VertexBuffer& vb, CommandList cmd) override;

		void _BindVertexBuffer(ui32 slot, ui32 stride, ui32 offset, VertexBuffer& vb, CommandList cmd) override;
		void _BindVertexBuffers(ui32 slot, ui32 count, const ui32* strides, const ui32* offset, VertexBuffer** vbs, CommandList cmd) override;

		void _UnbindVertexBuffer(ui32 slot, CommandList cmd) override;
		void _UnbindVertexBuffers(CommandList cmd) override;

		// IndexBuffer
		bool _CreateIndexBuffer(void* data, IndexBuffer& ib) override;
		void _ReleaseIndexBuffer(IndexBuffer& ib) override;

		void _UpdateIndexBuffer(void* data, ui32 size, IndexBuffer& ib, CommandList cmd) override;

		void _BindIndexBuffer(SV_GFX_FORMAT format, ui32 offset, IndexBuffer& ib, CommandList cmd) override;
		void _UnbindIndexBuffer(CommandList cmd) override;

		// ConstantBuffer
		bool _CreateConstantBuffer(void* data, ConstantBuffer& cb) override;
		void _ReleaseConstantBuffer(ConstantBuffer& cb) override;

		void _UpdateConstantBuffer(void* data, ui32 size, ConstantBuffer& cb, CommandList cmd) override;

		void _BindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, ConstantBuffer& cb, CommandList cmd) override;
		void _BindConstantBuffers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, ConstantBuffer** cbs, CommandList cmd) override;

		void _UnbindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindConstantBuffers(SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindConstantBuffers(CommandList cmd) override;

		// FrameBuffers
		bool _CreateFrameBuffer(FrameBuffer& fb) override;
		void _ReleaseFrameBuffer(FrameBuffer& fb) override;

		void _ClearFrameBuffer(SV::Color4f color, FrameBuffer& fb, CommandList cmd) override;

		void _BindFrameBuffer(FrameBuffer& fb, CommandList cmd) override;
		void _BindFrameBuffers(ui32 count, FrameBuffer** fbs, CommandList cmd) override;

		void _BindFrameBufferAsTexture(ui32 slot, SV_GFX_SHADER_TYPE type, FrameBuffer& fb, CommandList cmd) override;

		void _UnbindFrameBuffers(CommandList cmd) override;

		// Shaders
		bool _CreateShader(const char* filePath, Shader& s) override;
		void _ReleaseShader(Shader& s) override;
		void _BindShader(Shader& s, CommandList cmd) override;
		void _UnbindShader(SV_GFX_SHADER_TYPE type, CommandList cmd) override;

		// Input Layout
		bool _CreateInputLayout(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, InputLayout& il) override;
		void _ReleaseInputLayout(InputLayout& il) override;

		void _BindInputLayout(InputLayout& il, CommandList cmd) override;
		void _UnbindInputLayout(CommandList cmd) override;

		// Texture
		bool _CreateTexture(void* data, Texture& t) override;
		void _ReleaseTexture(Texture& t) override;

		void _UpdateTexture(void* data, ui32 size, Texture& t, CommandList cmd) override;

		void _BindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, Texture& t, CommandList cmd) override;
		void _BindTextures(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Texture** ts, CommandList cmd) override;

		void _UnbindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindTextures(SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindTextures(CommandList cmd) override;

		// Sampler
		bool _CreateSampler(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Sampler& s) override;
		void _ReleaseSampler(Sampler& s) override;

		void _BindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, Sampler& s, CommandList cmd) override;
		void _BindSamplers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Sampler** ss, CommandList cmd) override;

		void _UnbindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindSamplers(SV_GFX_SHADER_TYPE type, CommandList cmd) override;
		void _UnbindSamplers(CommandList cmd) override;

		// BlendState
		bool _CreateBlendState(const SV_GFX_BLEND_STATE_DESC* desc, BlendState& bs) override;
		void _ReleaseBlendState(BlendState& bs) override;

		void _BindBlendState(ui32 sampleMask, const float* blendFactors, BlendState& bs, CommandList cmd) override;
		void _UnbindBlendState(CommandList cmd) override;

	};

}