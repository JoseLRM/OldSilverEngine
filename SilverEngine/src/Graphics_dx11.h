#pragma once

#include "core.h"
#include "StateManager_dx11.h"
#include "Graphics.h"

namespace SV {

	class DirectX11Device;

	///////////////////////////////VERTEX BUFFER///////////////////////////////////
	class VertexBuffer_dx11 : public SV::_internal::VertexBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;

	public:
		bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) override;
		void _Release() override;

		void _Update(void* data, ui32 size, CommandList& cmd) override;

		void _Bind(ui32 slot, ui32 stride, ui32 offset, CommandList& cmd) override;
		void _Unbind(CommandList& cmd) override;

	};
	///////////////////////////////INDEX BUFFER///////////////////////////////////
	class IndexBuffer_dx11 : public SV::_internal::IndexBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;

	public:
		bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) override;
		void _Release() override;

		void _Bind(SV_GFX_FORMAT format, ui32 offset, CommandList& cmd) override;
		void _Unbind(CommandList& cmd) override;

	};

	///////////////////////////////CONSTANT BUFFER///////////////////////////////////
	class ConstantBuffer_dx11 : public SV::_internal::ConstantBuffer_internal {
		ComPtr<ID3D11Buffer> m_Buffer;

	public:
		bool _Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device) override;
		void _Release() override;

		void _Bind(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList& cmd) override;
		void _Unbind(SV_GFX_SHADER_TYPE type, CommandList& cmd) override;

	};

	///////////////////////////////FRAME BUFFER///////////////////////////////////
	class FrameBuffer_dx11 : public SV::_internal::FrameBuffer_internal {
		ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
		ComPtr<ID3D11Texture2D> m_Resource;
		ComPtr<ID3D11ShaderResourceView> m_ShaderResouceView;

	public:
		friend DirectX11Device;

		bool _Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::Graphics& device) override;
		void _Release() override;

		bool _Resize(ui32 width, ui32 height, SV::Graphics& device) override;

		void _Clear(SV::Color4f color, CommandList& cmd) override;
		void _Bind(ui32 slot, CommandList& cmd) override;
		void _Unbind(CommandList& cmd) override;
		void _BindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) override;

		inline ComPtr<ID3D11RenderTargetView>& GetRTV() noexcept { return m_RenderTargetView; }
		inline ComPtr<ID3D11Texture2D>& GetResource() noexcept { return m_Resource; }

	};

	/////////////////////////////////SHADER//////////////////////////////////////////
	class Shader_dx11 : public SV::_internal::Shader_internal {
		void* m_Shader = nullptr;
		ComPtr<ID3DBlob> m_Blob;

	public:
		bool _Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::Graphics& device) override;
		void _Release() override;

		void _Bind(CommandList& cmd) override;
		void _Unbind(CommandList& cmd) override;

		inline ComPtr<ID3DBlob>& GetBlob() noexcept { return m_Blob; }

	};

	/////////////////////////////////INPUT LAYOUT//////////////////////////////////////////
	class InputLayout_dx11 : public SV::_internal::InputLayout_internal {
		ComPtr<ID3D11InputLayout> m_InputLayout;

	public:
		bool _Create(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, SV::Graphics& device) override;
		void _Release() override;

		void _Bind(CommandList& cmd) override;
		void _Unbind(CommandList& cmd) override;
		
	};

	/////////////////////////////////TEXTURE//////////////////////////////////////////
	class Texture_dx11 : public SV::_internal::Texture_internal {
		ComPtr<ID3D11Texture2D> m_Texture;
		ComPtr<ID3D11ShaderResourceView> m_ShaderResouceView;

	public:
		bool _Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::Graphics& device) override;
		void _Release() override;
		void _Update(void* data, ui32 size, CommandList& cmd) override;
		void _Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) override;
		void _Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) override;
	
	};

	/////////////////////////////////SAMPLER//////////////////////////////////////////
	class Sampler_dx11 : public SV::_internal::Sampler_internal {
		ComPtr<ID3D11SamplerState> m_SamplerState;

	public:
		bool _Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Graphics& device) override;
		void _Release() override;
		void _Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) override;
		void _Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd) override;

	};

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

		FrameBuffer mainFrameBuffer;

		SV::StateManager_dx11 stateManager[SV_GFX_COMMAND_LIST_COUNT];

		///////////////////////////Methods/////////////////////////
		void CreateBackBuffer(const SV::Adapter::OutputMode& outputMode, ui32 width, ui32 height);
		bool _Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc) override;
		bool _Close() override;

		void Present() override;
		CommandList BeginCommandList() override;
		void SetViewport(ui32 slot, float x, float y, float w, float h, float n, float f, SV::CommandList& cmd) override;

		void ResizeBackBuffer(ui32 width, ui32 height);

		void SetTopology(SV_GFX_TOPOLOGY topology, CommandList& cmd) override;

		void EnableFullscreen() override;
		void DisableFullscreen() override;

		SV::FrameBuffer& GetMainFrameBuffer() override;

		// Draw calls
		void Draw(ui32 vertexCount, ui32 startVertex, CommandList& cmd) override;
		void DrawIndexed(ui32 indexCount, ui32 startIndex, ui32 startVertex, CommandList& cmd) override;
		void DrawInstanced(ui32 verticesPerInstance, ui32 instances, ui32 startVertex, ui32 startInstance, CommandList& cmd) override;
		void DrawIndexedInstanced(ui32 indicesPerInstance, ui32 instances, ui32 startIndex, ui32 startVertex, ui32 startInstance, CommandList& cmd) override;

		///////////////////////////Primitive Creation///////////////////////////////////
		void ValidateVertexBuffer(VertexBuffer* buffer)		override { buffer->Set(std::make_unique<VertexBuffer_dx11>()); }
		void ValidateIndexBuffer(IndexBuffer* buffer)		override { buffer->Set(std::make_unique<IndexBuffer_dx11>()); }
		void ValidateConstantBuffer(ConstantBuffer* buffer)	override { buffer->Set(std::make_unique<ConstantBuffer_dx11>()); }
		void ValidateFrameBuffer(FrameBuffer* buffer)		override { buffer->Set(std::make_unique<FrameBuffer_dx11>()); }
		void ValidateShader(Shader* shader)					override { shader->Set(std::make_unique<Shader_dx11>()); }
		void ValidateInputLayout(InputLayout* il)			override { il->Set(std::make_unique<InputLayout_dx11>()); }
		void ValidateTexture(Texture* tex)					override { tex->Set(std::make_unique<Texture_dx11>()); }
		void ValidateSampler(Sampler* sam)					override { sam->Set(std::make_unique<Sampler_dx11>()); }

	};

}