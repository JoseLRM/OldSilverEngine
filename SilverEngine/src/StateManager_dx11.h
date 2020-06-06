#pragma once

#include "Dx11.h"
#include "core.h"

namespace SV {
	
	// Binding classes
	class StateManager_dx11 {
		// RenderTargets
		ID3D11RenderTargetView* m_RenderTargets[SV_GFX_RENDER_TARGET_VIEW_COUNT];
		ID3D11DepthStencilView* m_DepthStencilView;
		ui32 m_RenderTargetsCount;
		bool m_BindRenderTargets;

		// Viewports
		D3D11_VIEWPORT m_Viewports[SV_GFX_VIEWPORT_COUNT];
		bool m_BindViewports;

		// Vertex Buffers
		ID3D11Buffer* m_VertexBuffers[SV_GFX_VERTEX_BUFFER_COUNT];
		UINT m_VertexBuffersStrides[SV_GFX_VERTEX_BUFFER_COUNT];
		UINT m_VertexBuffersOffsets[SV_GFX_VERTEX_BUFFER_COUNT];
		ui32 m_VertexBuffersCount;
		bool m_BindVertexBuffers;

		// Constant Buffers
		ID3D11Buffer* m_ConstantBuffers[3][SV_GFX_CONSTANT_BUFFER_COUNT];
		ui32 m_ConstantBuffersCount[3];
		bool m_BindConstantBuffers[3];

		// Textures
		ID3D11ShaderResourceView* m_Textures[3][SV_GFX_TEXTURE_COUNT];
		ui32 m_TexturesCount[3];
		bool m_BindTextures[3];

		// Sampler
		ID3D11SamplerState* m_Samplers[3][SV_GFX_SAMPLER_COUNT];
		ui32 m_SamplersCount[3];
		bool m_BindSamplers[3];

	public:
		StateManager_dx11();

		void BindRTV(ID3D11RenderTargetView* rtv, ui32 slot);
		void UnbindRTV(ui32 slot);
		
		void BindDSV(ID3D11DepthStencilView* dsv);
		void UnbindDSV();

		void SetViewport(const D3D11_VIEWPORT& vp, ui32 slot);

		void BindVB(ID3D11Buffer* b, ui32 slot, ui32 stride, ui32 offset);
		void UnbindVB(ui32 slot);

		void BindCB(ID3D11Buffer* b, SV_GFX_SHADER_TYPE type, ui32 slot);
		void UnbindCB(SV_GFX_SHADER_TYPE type, ui32 slot);

		void BindTex(ID3D11ShaderResourceView* t, SV_GFX_SHADER_TYPE type, ui32 slot);
		void UnbindTex(SV_GFX_SHADER_TYPE type, ui32 slot);

		void BindSam(ID3D11SamplerState* s, SV_GFX_SHADER_TYPE type, ui32 slot);
		void UnbindSam(SV_GFX_SHADER_TYPE type, ui32 slot);

		void BindState(ComPtr<ID3D11DeviceContext>& ctx);

		void Reset();

	};

}