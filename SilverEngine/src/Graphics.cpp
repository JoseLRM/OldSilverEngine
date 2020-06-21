#include "core.h"

#include "Graphics.h"

namespace SV {

	Graphics::Graphics()
	{}
	Graphics::~Graphics()
	{}

	bool Graphics::Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		// Select adapter
		{
			auto& adapters = SV::User::GetAdapters();
			bool find = false;

			ui32 res = 0u;

			for (ui32 i = 0; i < adapters.size(); ++i) {
				const SV::Adapter& adapter = adapters[i];

				if (adapter.modes.empty()) continue;

				for (ui32 j = 0; j < adapter.modes.size(); ++j) {
					const SV::Adapter::OutputMode& outputMode = adapter.modes[j];

					ui32 outputRes = uvec2(outputMode.width, outputMode.height).Mag();
					if (outputRes > res) {
						m_OutputModeID = j;
						m_Adapter = adapter;
						outputRes = res;
						find = true;
					}
				}
			}

			if (!find) {
				SV::LogE("Adapter not found");
				return false;
			}
		}

		// Initialize API
		if (!_Initialize(desc)) {
			SV::LogE("Can't initialize GraphicsAPI");
			return false;
		}

		return true;
	}

	bool Graphics::Close()
	{
		if (!_Close()) {
			SV::LogE("Can't close GraphicsDevice");
		}

		return true;
	}

	void Graphics::BeginFrame()
	{
#ifdef SV_IMGUI
		SVImGui::_internal::BeginFrame();
#endif
	}
	void Graphics::EndFrame()
	{
		Present();
	}

	void Graphics::SetFullscreen(bool fullscreen)
	{
		if (m_Fullscreen == fullscreen) return;

		m_Fullscreen = fullscreen;

		if (fullscreen) {
			EnableFullscreen();
		}
		else {
			DisableFullscreen();
		}
	}

#define SVAssertValidation(x) SV_ASSERT(x.IsValid())

	void Graphics::ResetState(CommandList cmd)
	{
		_ResetState(cmd);
	}

	// VERTEX BUFFER
	bool Graphics::CreateVertexBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, VertexBuffer& vb)
	{
		if (vb.IsValid()) ReleaseVertexBuffer(vb);
		else vb.Set(_AllocateVertexBuffer());
		vb->m_Size = size;
		vb->m_Usage = usage;
		vb->m_CPUAccess = cpuAccess;
		return _CreateVertexBuffer(data, vb);
	}
	void Graphics::ReleaseVertexBuffer(VertexBuffer& vb)
	{
		if(vb.IsValid()) _ReleaseVertexBuffer(vb);
	}
	void Graphics::UpdateVertexBuffer(void* data, ui32 size, VertexBuffer& vb, CommandList cmd)
	{
		SVAssertValidation(vb);
		_UpdateVertexBuffer(data, size, vb, cmd);
	}
	void Graphics::BindVertexBuffer(ui32 slot, ui32 stride, ui32 offset, VertexBuffer& vb, CommandList cmd)
	{
		SVAssertValidation(vb);
		SV_ASSERT(slot < SV_GFX_VERTEX_BUFFER_COUNT);
		_BindVertexBuffer(slot, stride, offset, vb, cmd);
	}
	void Graphics::BindVertexBuffers(ui32 slot, ui32 count, const ui32* strides, const ui32* offset, VertexBuffer** vbs, CommandList cmd)
	{
		_BindVertexBuffers(slot, count, strides, offset, vbs, cmd);
	}
	void Graphics::UnbindVertexBuffer(ui32 slot, CommandList cmd)
	{
		_UnbindVertexBuffer(slot, cmd);
	}
	void Graphics::UnbindVertexBuffers(CommandList cmd)
	{
		_UnbindVertexBuffers(cmd);
	}

	// INDEX BUFFER
	bool Graphics::CreateIndexBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, IndexBuffer& ib)
	{
		if (ib.IsValid()) ReleaseIndexBuffer(ib);
		else ib.Set(_AllocateIndexBuffer());
		ib->m_Size = size;
		ib->m_Usage = usage;
		ib->m_CPUAccess = cpuAccess;
		return _CreateIndexBuffer(data, ib);
	}
	void Graphics::ReleaseIndexBuffer(IndexBuffer& ib)
	{
		if (ib.IsValid()) _ReleaseIndexBuffer(ib);
	}
	void Graphics::UpdateIndexBuffer(void* data, ui32 size, IndexBuffer& ib, CommandList cmd)
	{
		SVAssertValidation(ib);
		_UpdateIndexBuffer(data, size, ib, cmd);
	}
	void Graphics::BindIndexBuffer(SV_GFX_FORMAT format, ui32 offset, IndexBuffer& ib, CommandList cmd)
	{
		SVAssertValidation(ib);
		_BindIndexBuffer(format, offset, ib, cmd);
	}
	void Graphics::UnbindIndexBuffer(CommandList cmd)
	{
		_UnbindIndexBuffer(cmd);
	}

	// CONSTANT BUFFER
	bool Graphics::CreateConstantBuffer(ui32 size, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, void* data, ConstantBuffer& cb)
	{
		if (cb.IsValid()) ReleaseConstantBuffer(cb);
		else cb.Set(_AllocateConstantBuffer());
		cb->m_Size = size;
		cb->m_Usage = usage;
		cb->m_CPUAccess = cpuAccess;
		return _CreateConstantBuffer(data, cb);
	}
	void Graphics::ReleaseConstantBuffer(ConstantBuffer& cb)
	{
		if (cb.IsValid()) _ReleaseConstantBuffer(cb);
	}
	void Graphics::UpdateConstantBuffer(void* data, ui32 size, ConstantBuffer& cb, CommandList cmd)
	{
		SVAssertValidation(cb);
		_UpdateConstantBuffer(data, size, cb, cmd);
	}
	void Graphics::BindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, ConstantBuffer& cb, CommandList cmd)
	{
		SV_ASSERT(slot < SV_GFX_CONSTANT_BUFFER_COUNT);
		SVAssertValidation(cb);
		_BindConstantBuffer(slot, type, cb, cmd);
	}
	void Graphics::BindConstantBuffers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, ConstantBuffer** cbs, CommandList cmd)
	{
		_BindConstantBuffers(slot, type, count, cbs, cmd);
	}
	void Graphics::UnbindConstantBuffer(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindConstantBuffer(slot, type, cmd);
	}
	void Graphics::UnbindConstantBuffers(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindConstantBuffers(type, cmd);
	}
	void Graphics::UnbindConstantBuffers(CommandList cmd)
	{
		_UnbindConstantBuffers(cmd);
	}

	// FRAME BUFFER
	bool Graphics::CreateFrameBuffer(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, FrameBuffer& fb)
	{
		if (fb.IsValid()) ReleaseFrameBuffer(fb);
		else fb.Set(_AllocateFrameBuffer());

		fb->m_Width = width;
		fb->m_Height = height;
		fb->m_Format = format;
		fb->m_TextureUsage = textureUsage;

		return _CreateFrameBuffer(fb);
	}
	void Graphics::ReleaseFrameBuffer(FrameBuffer& fb)
	{
		if(fb.IsValid()) _ReleaseFrameBuffer(fb);
	}
	bool Graphics::ResizeFrameBuffer(ui32 width, ui32 height, FrameBuffer& fb)
	{
		SVAssertValidation(fb);
		return CreateFrameBuffer(width, height, fb->m_Format, fb->m_TextureUsage, fb);
	}
	void Graphics::ClearFrameBuffer(SV::Color4f color, FrameBuffer& fb, CommandList cmd)
	{
		SVAssertValidation(fb);
		_ClearFrameBuffer(color, fb, cmd);
	}
	void Graphics::BindFrameBuffer(FrameBuffer& fb, Texture* dsv, CommandList cmd)
	{
		SVAssertValidation(fb);
		SV_ASSERT(dsv ? dsv->IsValid() : true);
		_BindFrameBuffer(fb, dsv, cmd);
	}
	void Graphics::BindFrameBuffers(ui32 count, FrameBuffer** fbs, Texture* dsv, CommandList cmd)
	{
		SV_ASSERT(count <= SV_GFX_RENDER_TARGET_VIEW_COUNT);
		SV_ASSERT(dsv ? dsv->IsValid() : true);
		_BindFrameBuffers(count, fbs, dsv, cmd);
	}
	void Graphics::BindFrameBufferAsTexture(ui32 slot, SV_GFX_SHADER_TYPE type, FrameBuffer& fb, CommandList cmd)
	{
		SVAssertValidation(fb);
		SV_ASSERT(fb->m_TextureUsage);
		_BindFrameBufferAsTexture(slot, type, fb, cmd);
	}
	void Graphics::UnbindFrameBuffers(CommandList cmd)
	{
		_UnbindFrameBuffers(cmd);
	}

	// SHADER
	bool Graphics::CreateShader(SV_GFX_SHADER_TYPE type, const char* filePath, Shader& s)
	{
		if (s.IsValid()) ReleaseShader(s);
		else s.Set(_AllocateShader());

		s->m_ShaderType = type;

		return _CreateShader(filePath, s);
	}
	void Graphics::ReleaseShader(Shader& s)
	{
		if (s.IsValid()) _ReleaseShader(s);
	}
	void Graphics::BindShader(Shader& s, CommandList cmd)
	{
		SVAssertValidation(s);
		_BindShader(s, cmd);
	}
	void Graphics::UnbindShader(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindShader(type, cmd);
	}

	// INPUT LAYOUT
	bool Graphics::CreateInputLayout(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, InputLayout& il)
	{
		if (il.IsValid()) ReleaseInputLayout(il);
		else il.Set(_AllocateInputLayout());

		return _CreateInputLayout(desc, count, vs, il);
	}
	void Graphics::ReleaseInputLayout(InputLayout& il)
	{
		if (il.IsValid()) _ReleaseInputLayout(il);
	}
	void Graphics::BindInputLayout(InputLayout& il, CommandList cmd)
	{
		SVAssertValidation(il);
		_BindInputLayout(il, cmd);
	}
	void Graphics::UnbindInputLayout(CommandList cmd)
	{
		_UnbindInputLayout(cmd);
	}

	// TEXTURE
	bool Graphics::CreateTexture(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, Texture& t)
	{
		if (t.IsValid()) ReleaseTexture(t);
		else t.Set(_AllocateTexture());

		t->m_Width = width;
		t->m_Height = height;
		t->m_Format = format;
		t->m_Usage = usage;
		t->m_CPUAccess = cpuAccess;
		t->m_IsDepthStencilView = false;

		return _CreateTexture(data, t);
	}
	bool Graphics::CreateTextureDSV(ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, SV_GFX_CPU_ACCESS cpuAccess, Texture& t)
	{
		if (t.IsValid()) ReleaseTexture(t);
		else t.Set(_AllocateTexture());

		t->m_Width = width;
		t->m_Height = height;
		t->m_Format = format;
		t->m_Usage = usage;
		t->m_CPUAccess = cpuAccess;
		t->m_IsDepthStencilView = true;

		return _CreateTexture(nullptr, t);
	}
	void Graphics::ReleaseTexture(Texture& t)
	{
		if (t.IsValid()) _ReleaseTexture(t);
	}
	bool Graphics::ResizeTexture(void* data, ui32 width, ui32 height, Texture& t)
	{
		SVAssertValidation(t);
		
		bool result;
		if (t->IsDepthStencilView()) {
			result = CreateTextureDSV(width, height, t->m_Format, t->m_Usage, t->m_CPUAccess, t);
		}
		else {
			result = CreateTexture(data, width, height, t->m_Format, t->m_Usage, t->m_CPUAccess, t);
		}
		return result;
	}
	void Graphics::ClearTextureDSV(float depth, ui8 stencil, Texture& t, CommandList cmd)
	{
		SVAssertValidation(t);
		SV_ASSERT(t->IsDepthStencilView());
		_ClearTextureDSV(depth, stencil, t, cmd);
	}
	void Graphics::UpdateTexture(void* data, ui32 size, Texture& t, CommandList cmd)
	{
		SVAssertValidation(t);
		_UpdateTexture(data, size, t, cmd);
	}
	void Graphics::BindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, Texture& t, CommandList cmd)
	{
		SVAssertValidation(t);
		_BindTexture(slot, type, t, cmd);
	}
	void Graphics::BindTextures(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Texture** ts, CommandList cmd)
	{
		_BindTextures(slot, type, count, ts, cmd);
	}
	void Graphics::UnbindTexture(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindTexture(slot, type, cmd);
	}
	void Graphics::UnbindTextures(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindTextures(type, cmd);
	}
	void Graphics::UnbindTextures(CommandList cmd)
	{
		_UnbindTextures(cmd);
	}

	// SAMPLER
	bool Graphics::CreateSampler(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Sampler& s)
	{
		if (s.IsValid()) ReleaseSampler(s);
		else s.Set(_AllocateSampler());

		return _CreateSampler(addressMode, filter, s);
	}
	void Graphics::ReleaseSampler(Sampler& s)
	{
		if (s.IsValid()) _ReleaseSampler(s);
	}
	void Graphics::BindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, Sampler& s, CommandList cmd)
	{
		SVAssertValidation(s);
		_BindSampler(slot, type, s, cmd);
	}
	void Graphics::BindSamplers(ui32 slot, SV_GFX_SHADER_TYPE type, ui32 count, Sampler** ss, CommandList cmd)
	{
		_BindSamplers(slot, type, count, ss, cmd);
	}
	void Graphics::UnbindSampler(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindSampler(slot, type, cmd);
	}
	void Graphics::UnbindSamplers(SV_GFX_SHADER_TYPE type, CommandList cmd)
	{
		_UnbindSamplers(type, cmd);
	}
	void Graphics::UnbindSamplers(CommandList cmd)
	{
		_UnbindSamplers(cmd);
	}

	// BLEND STATE
	bool Graphics::CreateBlendState(const SV_GFX_BLEND_STATE_DESC* desc, BlendState& bs)
	{
		if (bs.IsValid()) ReleaseBlendState(bs);
		else bs.Set(_AllocateBlendState());

		bs->m_Desc = *desc;

		return _CreateBlendState(bs);
	}
	void Graphics::ReleaseBlendState(BlendState& bs)
	{
		if (bs.IsValid()) _ReleaseBlendState(bs);
	}
	void Graphics::BindBlendState(ui32 sampleMask, const float* blendFactors, BlendState& bs, CommandList cmd)
	{
		SVAssertValidation(bs);
		_BindBlendState(sampleMask, blendFactors, bs, cmd);
	}
	void Graphics::BindBlendState(ui32 sampleMask, BlendState& bs, CommandList cmd)
	{
		BindBlendState(sampleMask, nullptr, bs, cmd);
	}
	void Graphics::BindBlendState(const float* blendFactors, BlendState& bs, CommandList cmd)
	{
		BindBlendState(0xffffffff, blendFactors, bs, cmd);
	}
	void Graphics::BindBlendState(BlendState& bs, CommandList cmd)
	{
		BindBlendState(0xffffffff, nullptr, bs, cmd);
	}
	void Graphics::UnbindBlendState(CommandList cmd)
	{
		_UnbindBlendState(cmd);
	}

	// DEPTH STENCIL STATE
	bool Graphics::CreateDepthStencilState(const SV_GFX_DEPTH_STENCIL_STATE_DESC* desc, DepthStencilState& dss)
	{
		if (dss.IsValid()) ReleaseDepthStencilState(dss);
		else dss.Set(_AllocateDepthStencilState());

		dss->m_Desc = *desc;

		return _CreateDepthStencilState(dss);
	}
	void Graphics::ReleaseDepthStencilState(DepthStencilState& dss)
	{
		if (dss.IsValid()) ReleaseDepthStencilState(dss);
	}
	void Graphics::BindDepthStencilState(ui32 stencilRef, DepthStencilState& dss, CommandList cmd)
	{
		SVAssertValidation(dss);
		_BindDepthStencilState(stencilRef, dss, cmd);
	}
	void Graphics::BindDepthStencilState(DepthStencilState& dss, CommandList cmd)
	{
		SVAssertValidation(dss);
		_BindDepthStencilState(0u, dss, cmd);
	}
	void Graphics::UnbindDepthStencilState(CommandList cmd)
	{
		_UnbindDepthStencilState(cmd);
	}

}
