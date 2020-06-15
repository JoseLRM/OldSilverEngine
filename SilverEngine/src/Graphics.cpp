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

}

// VERTEX BUFFER
bool SV::_internal::VertexBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device)
{
	m_Size = size;
	m_Usage = usage;
	return _Create(size, usage, CPUWriteAccess, CPUReadAccess, data, device);
}
void SV::_internal::VertexBuffer_internal::Release()
{
	_Release();
}
void SV::_internal::VertexBuffer_internal::Update(void* data, ui32 size, CommandList& cmd)
{
	_Update(data, size, cmd);
}
void SV::_internal::VertexBuffer_internal::Bind(ui32 slot, ui32 stride, ui32 offset, CommandList& cmd)
{
	m_LastSlot = slot;
	_Bind(slot, stride, offset, cmd);
}
void SV::_internal::VertexBuffer_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
// INDEX BUFFER
bool SV::_internal::IndexBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device)
{
	return _Create(size, usage, CPUWriteAccess, CPUReadAccess, data, device);
}
void SV::_internal::IndexBuffer_internal::Release()
{
	_Release();
}
void SV::_internal::IndexBuffer_internal::Bind(SV_GFX_FORMAT format, ui32 offset, CommandList& cmd)
{
	_Bind(format, offset, cmd);
}
void SV::_internal::IndexBuffer_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
// CONSTANT BUFFER
bool SV::_internal::ConstantBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, Graphics& device)
{
	m_Size = size;
	m_Usage = usage;
	return _Create(size, usage, CPUWriteAccess, CPUReadAccess, data, device);
}
void SV::_internal::ConstantBuffer_internal::Release()
{
	_Release();
}
void SV::_internal::ConstantBuffer_internal::Update(void* data, ui32 size, CommandList& cmd)
{
	_Update(data, size, cmd);
}
void SV::_internal::ConstantBuffer_internal::Bind(ui32 slot, SV_GFX_SHADER_TYPE type, CommandList& cmd)
{
	m_LastSlot[type] = slot;
	_Bind(slot, type, cmd);
}
void SV::_internal::ConstantBuffer_internal::Unbind(SV_GFX_SHADER_TYPE type, CommandList& cmd)
{
	_Unbind(type, cmd);
}
// FRAME BUFFER
bool SV::_internal::FrameBuffer_internal::Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::Graphics& device)
{
	m_Format = format;
	m_Width = width;
	m_Height = height;
	m_TextureUsage = textureUsage;
	return _Create(width, height, format, textureUsage, device);
}
void SV::_internal::FrameBuffer_internal::Release()
{
	_Release();
}
bool SV::_internal::FrameBuffer_internal::Resize(ui32 width, ui32 height, SV::Graphics& device)
{
	return _Resize(width, height, device);
}
void SV::_internal::FrameBuffer_internal::Clear(SV::Color4f color, CommandList& cmd)
{
	_Clear(color, cmd);
}
void SV::_internal::FrameBuffer_internal::Bind(ui32 slot, CommandList& cmd)
{
	m_LastSlot = slot;
	_Bind(slot, cmd);
}
void SV::_internal::FrameBuffer_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
void SV::_internal::FrameBuffer_internal::BindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
	_BindAsTexture(type, slot, cmd);
}
void SV::_internal::FrameBuffer_internal::UnbindAsTexture(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
}
// SHADER
bool SV::_internal::Shader_internal::Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::Graphics& device)
{
	m_ShaderType = type;
	return _Create(type, filePath, device);
}
void SV::_internal::Shader_internal::Release()
{
	_Release();
}
void SV::_internal::Shader_internal::Bind(CommandList& cmd)
{
	_Bind(cmd);
}
void SV::_internal::Shader_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
// INPUT LAYOUT
bool SV::_internal::InputLayout_internal::Create(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, SV::Graphics& device)
{
	return _Create(desc, count, vs, device);
}
void SV::_internal::InputLayout_internal::Release()
{
	_Release();
}
void SV::_internal::InputLayout_internal::Bind(CommandList& cmd)
{
	_Bind(cmd);
}
void SV::_internal::InputLayout_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
// TEXTURE
bool SV::_internal::Texture_internal::Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::Graphics& device)
{	
	if (CPUWriteAccess && CPUReadAccess) m_CPUAccess = 3;
	else if (CPUWriteAccess) m_CPUAccess = 1;
	else if (CPUReadAccess) m_CPUAccess = 2;
	else m_CPUAccess = 0u;

	if(!_Create(data, width, height, format, usage, CPUWriteAccess, CPUReadAccess, device)) return false;

	m_Width = width;
	m_Height = height;
	m_Format = format;
	m_Usage = usage;
	m_Size = width * height * SV::GetFormatSize(format);
	return true;
}
void SV::_internal::Texture_internal::Release()
{
	_Release();
}
bool SV::_internal::Texture_internal::Resize(void* data, ui32 width, ui32 height, SV::Graphics& device)
{
	Release();

	bool CPUWriteAccess = (m_CPUAccess == 1 || m_CPUAccess == 3);
	bool CPUReadAccess = (m_CPUAccess == 2 || m_CPUAccess == 3);

	return Create(data, width, height, m_Format, m_Usage, CPUWriteAccess, CPUReadAccess, device);
}
void SV::_internal::Texture_internal::Update(void* data, ui32 size, CommandList& cmd)
{
	_Update(data, size, cmd);
}
void SV::_internal::Texture_internal::Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
	_Bind(type, slot, cmd);
}
void SV::_internal::Texture_internal::Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
	_Unbind(type, slot, cmd);
}
// SAMPLER
bool SV::_internal::Sampler_internal::Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, Graphics& device)
{
	return _Create(addressMode, filter, device);
}
void SV::_internal::Sampler_internal::Release()
{
	_Release();
}
void SV::_internal::Sampler_internal::Bind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
	_Bind(type, slot, cmd);
}
void SV::_internal::Sampler_internal::Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, CommandList& cmd)
{
	_Unbind(type, slot, cmd);
}

//BLEND STATE
void SV::_internal::BlendState_internal::SetIndependentRenderTarget(bool enable)
{
	m_IndependentRenderTarget = enable;
}
void SV::_internal::BlendState_internal::EnableBlending(ui8 renderTarget)
{
	m_BlendDesc[renderTarget].enabled = true;
}
void SV::_internal::BlendState_internal::DisableBlending(ui8 renderTarget)
{
	m_BlendDesc[renderTarget].enabled = false;
}
void SV::_internal::BlendState_internal::SetRenderTargetOperation(SV_GFX_BLEND src, SV_GFX_BLEND srcAlpha, SV_GFX_BLEND dest, SV_GFX_BLEND destAlpha, SV_GFX_BLEND_OP op, SV_GFX_BLEND_OP opAlpha, ui8 renderTarget)
{
	m_BlendDesc[renderTarget].src = src;
	m_BlendDesc[renderTarget].srcAlpha = srcAlpha;
	m_BlendDesc[renderTarget].dest = dest;
	m_BlendDesc[renderTarget].destAlpha = destAlpha;
	m_BlendDesc[renderTarget].op = op;
	m_BlendDesc[renderTarget].opAlpha = opAlpha;
}
void SV::_internal::BlendState_internal::SetRenderTargetOperation(SV_GFX_BLEND src, SV_GFX_BLEND dest, SV_GFX_BLEND_OP op, ui8 renderTarget)
{
	SetRenderTargetOperation(src, src, dest, dest, op, op, renderTarget);
}
void SV::_internal::BlendState_internal::SetWriteMask(ui8 writeMask, ui8 renderTarget)
{
	m_BlendDesc[renderTarget].writeMask = writeMask;
}
bool SV::_internal::BlendState_internal::Create(SV::Graphics& graphics)
{
	return _Create(graphics);
}
void SV::_internal::BlendState_internal::Release()
{
	_Release();
}
void SV::_internal::BlendState_internal::Bind(ui32 sampleMask, const float* blendFactors, CommandList& cmd)
{
	_Bind(sampleMask, blendFactors, cmd);
}
void SV::_internal::BlendState_internal::Unbind(CommandList& cmd)
{
	_Unbind(cmd);
}
