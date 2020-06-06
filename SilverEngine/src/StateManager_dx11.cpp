#include "core.h"
#include "StateManager_dx11.h"

namespace SV {

	StateManager_dx11::StateManager_dx11()
	{
		Reset();
	}

	void StateManager_dx11::BindRTV(ID3D11RenderTargetView* rtv, ui32 slot)
	{
		if (m_RenderTargets[slot] == nullptr) m_RenderTargetsCount++;
		m_RenderTargets[slot] = rtv;
		m_BindRenderTargets = true;
	}
	void StateManager_dx11::UnbindRTV(ui32 slot)
	{
		if (m_RenderTargets[slot] != nullptr) {
			m_RenderTargets[slot] = nullptr;
			m_RenderTargetsCount--;
			m_BindRenderTargets = true;
		}
	}

	void StateManager_dx11::BindDSV(ID3D11DepthStencilView* dsv)
	{
		m_DepthStencilView = dsv;
		m_BindRenderTargets = true;
	}
	void StateManager_dx11::UnbindDSV()
	{
		BindDSV(nullptr);
	}

	void StateManager_dx11::SetViewport(const D3D11_VIEWPORT& vp, ui32 slot)
	{
		m_Viewports[slot] = vp;
		m_BindViewports = true;
	}

	void StateManager_dx11::BindVB(ID3D11Buffer* b, ui32 slot, ui32 stride, ui32 offset)
	{
		if (m_VertexBuffers[slot] == nullptr) m_VertexBuffersCount++;
		m_VertexBuffers[slot] = b;
		m_VertexBuffersStrides[slot] = stride;
		m_VertexBuffersOffsets[slot] = offset;
		m_BindVertexBuffers = true;
	}

	void StateManager_dx11::UnbindVB(ui32 slot)
	{
		if (m_VertexBuffers[slot] != nullptr) {
			m_VertexBuffers[slot] = nullptr;
			--m_VertexBuffersCount;
			m_BindVertexBuffers = true;
		}
	}

	void StateManager_dx11::BindCB(ID3D11Buffer* b, SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_ConstantBuffers[type][slot] == nullptr) m_ConstantBuffersCount[type]++;
		m_ConstantBuffers[type][slot] = b;
		m_BindConstantBuffers[type] = true;
	}
	void StateManager_dx11::UnbindCB(SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_ConstantBuffers[type][slot] != nullptr) {
			m_ConstantBuffers[type][slot] = nullptr;
			m_ConstantBuffersCount[type]--;
			m_BindConstantBuffers[type] = true;
		}
	}

	void StateManager_dx11::BindTex(ID3D11ShaderResourceView* t, SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_Textures[type][slot] == nullptr) m_TexturesCount[type]++;
		m_Textures[type][slot] = t;
		m_BindTextures[type] = true;
	}
	void StateManager_dx11::UnbindTex(SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_Textures[type][slot] != nullptr) {
			m_Textures[type][slot] = nullptr;
			m_TexturesCount[type]--;
			m_BindTextures[type] = true;
		}
	}

	void StateManager_dx11::BindSam(ID3D11SamplerState* s, SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_Samplers[type][slot] == nullptr) m_SamplersCount[type]++;
		m_Samplers[type][slot] = s;
		m_BindSamplers[type] = true;
	}
	void StateManager_dx11::UnbindSam(SV_GFX_SHADER_TYPE type, ui32 slot)
	{
		if (m_Samplers[type][slot] != nullptr) {
			m_Samplers[type][slot] = nullptr;
			m_SamplersCount[type]--;
			m_BindSamplers[type] = true;
		}
	}

	void StateManager_dx11::BindState(ComPtr<ID3D11DeviceContext>& ctx)
	{
		// Bind Render Targets and Depth Stencil State
		if (m_BindRenderTargets) {
			ctx->OMSetRenderTargets(m_RenderTargetsCount, m_RenderTargets, m_DepthStencilView);
			m_BindRenderTargets = false;
		}
		// Bind Viewports
		if (m_BindViewports) {
			ctx->RSSetViewports(m_RenderTargetsCount, m_Viewports);
			m_BindViewports = false;
		}
		// Bind Vertex Buffers
		if (m_BindVertexBuffers) {
			ctx->IASetVertexBuffers(0u, m_VertexBuffersCount, m_VertexBuffers, m_VertexBuffersStrides, m_VertexBuffersOffsets);
			m_BindVertexBuffers = false;
		}
		// Bind Constant Buffers
		if (m_BindConstantBuffers[SV_GFX_SHADER_TYPE_VERTEX]) {
			ctx->VSSetConstantBuffers(0u, m_ConstantBuffersCount[SV_GFX_SHADER_TYPE_VERTEX], m_ConstantBuffers[SV_GFX_SHADER_TYPE_VERTEX]);
			m_BindConstantBuffers[SV_GFX_SHADER_TYPE_VERTEX] = false;
		}
		if (m_BindConstantBuffers[SV_GFX_SHADER_TYPE_PIXEL]) {
			ctx->PSSetConstantBuffers(0u, m_ConstantBuffersCount[SV_GFX_SHADER_TYPE_PIXEL], m_ConstantBuffers[SV_GFX_SHADER_TYPE_PIXEL]);
			m_BindConstantBuffers[SV_GFX_SHADER_TYPE_PIXEL] = false;
		}
		if (m_BindConstantBuffers[SV_GFX_SHADER_TYPE_GEOMETRY]) {
			ctx->GSSetConstantBuffers(0u, m_ConstantBuffersCount[SV_GFX_SHADER_TYPE_GEOMETRY], m_ConstantBuffers[SV_GFX_SHADER_TYPE_GEOMETRY]);
			m_BindConstantBuffers[SV_GFX_SHADER_TYPE_GEOMETRY] = false;
		}
		// Bind Textures
		if (m_BindTextures[SV_GFX_SHADER_TYPE_VERTEX]) {
			ctx->VSSetShaderResources(0u, m_TexturesCount[SV_GFX_SHADER_TYPE_VERTEX], m_Textures[SV_GFX_SHADER_TYPE_VERTEX]);
			m_BindTextures[SV_GFX_SHADER_TYPE_VERTEX] = false;
		}
		if (m_BindTextures[SV_GFX_SHADER_TYPE_PIXEL]) {
			ctx->PSSetShaderResources(0u, m_TexturesCount[SV_GFX_SHADER_TYPE_PIXEL], m_Textures[SV_GFX_SHADER_TYPE_PIXEL]);
			m_BindTextures[SV_GFX_SHADER_TYPE_PIXEL] = false;
		}
		if (m_BindTextures[SV_GFX_SHADER_TYPE_GEOMETRY]) {
			ctx->GSSetShaderResources(0u, m_TexturesCount[SV_GFX_SHADER_TYPE_GEOMETRY], m_Textures[SV_GFX_SHADER_TYPE_GEOMETRY]);
			m_BindTextures[SV_GFX_SHADER_TYPE_GEOMETRY] = false;
		}
		// Bind Samplers
		if (m_BindSamplers[SV_GFX_SHADER_TYPE_VERTEX]) {
			ctx->VSSetSamplers(0u, m_SamplersCount[SV_GFX_SHADER_TYPE_VERTEX], m_Samplers[SV_GFX_SHADER_TYPE_VERTEX]);
			m_BindSamplers[SV_GFX_SHADER_TYPE_VERTEX] = false;
		}
		if (m_BindSamplers[SV_GFX_SHADER_TYPE_PIXEL]) {
			ctx->PSSetSamplers(0u, m_SamplersCount[SV_GFX_SHADER_TYPE_PIXEL], m_Samplers[SV_GFX_SHADER_TYPE_PIXEL]);
			m_BindSamplers[SV_GFX_SHADER_TYPE_PIXEL] = false;
		}
		if (m_BindSamplers[SV_GFX_SHADER_TYPE_GEOMETRY]) {
			ctx->GSSetSamplers(0u, m_SamplersCount[SV_GFX_SHADER_TYPE_GEOMETRY], m_Samplers[SV_GFX_SHADER_TYPE_GEOMETRY]);
			m_BindSamplers[SV_GFX_SHADER_TYPE_GEOMETRY] = false;
		}
	}

	void StateManager_dx11::Reset()
	{
		// Reset Render Targets and Depth Stencil View
		for (ui32 i = 0; i < SV_GFX_RENDER_TARGET_VIEW_COUNT; ++i) {
			m_RenderTargets[i] = nullptr;
		}
		m_DepthStencilView = nullptr;

		m_RenderTargetsCount = 0u;
		m_BindRenderTargets = false;

		// Reset Viewport
		m_BindViewports = false;

		// Reset Vertex Buffer
		for (ui32 i = 0; i < SV_GFX_VERTEX_BUFFER_COUNT; ++i) {
			m_VertexBuffers[i] = nullptr;
		}
		m_BindVertexBuffers = false;
		m_VertexBuffersCount = 0u;

		// Reset Constant Buffer
		for (ui32 i = 0; i < 3; ++i) {
			for (ui32 j = 0; j < SV_GFX_CONSTANT_BUFFER_COUNT; ++j) {
				m_ConstantBuffers[i][j] = nullptr;
			}
		}
		for (ui32 i = 0; i < 3; ++i) {
			m_ConstantBuffersCount[i] = 0u;
			m_BindConstantBuffers[i] = false;
		}

		// Reset Textures
		for (ui32 i = 0; i < 3; ++i) {
			for (ui32 j = 0; j < SV_GFX_TEXTURE_COUNT; ++j) {
				m_Textures[i][j] = nullptr;
			}
		}
		for (ui32 i = 0; i < 3; ++i) {
			m_TexturesCount[i] = 0u;
			m_BindTextures[i] = false;
		}

		// Reset Samplers
		for (ui32 i = 0; i < 3; ++i) {
			for (ui32 j = 0; j < SV_GFX_SAMPLER_COUNT; ++j) {
				m_Samplers[i][j] = nullptr;
			}
		}
		for (ui32 i = 0; i < 3; ++i) {
			m_SamplersCount[i] = 0u;
			m_BindSamplers[i] = false;
		}
	}

}