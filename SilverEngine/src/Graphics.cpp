#include "core.h"

#include "Graphics_dx11.h"

namespace SV {

	Graphics::Graphics()
	{}
	Graphics::~Graphics()
	{}

	bool Graphics::Initialize(const SV_GRAPHICS_INITIALIZATION_DESC& desc)
	{
		m_Device = std::make_unique<DirectX11Device>();

		m_Device->SetEngine(&GetEngine());

		if (!m_Device->Initialize(desc)) {
			SV::LogE("Can't initialize GraphicsDevice");
			return false;
		}

		return true;
	}

	bool Graphics::Close()
	{
		if (m_Device.get()) {
			if (!m_Device->Close()) {
				SV::LogE("Can't close GraphicsDevice");
			}
		}

		return true;
	}

	void Graphics::BeginFrame()
	{

	}
	void Graphics::EndFrame()
	{
		m_Device->Present();
	}

}

// VERTEX BUFFER
bool SV::_internal::VertexBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, GraphicsDevice& device)
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
bool SV::_internal::IndexBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, GraphicsDevice& device)
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
bool SV::_internal::ConstantBuffer_internal::Create(ui32 size, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, void* data, GraphicsDevice& device)
{
	return _Create(size, usage, CPUWriteAccess, CPUReadAccess, data, device);
}
void SV::_internal::ConstantBuffer_internal::Release()
{
	_Release();
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
bool SV::_internal::FrameBuffer_internal::Create(ui32 width, ui32 height, SV_GFX_FORMAT format, bool textureUsage, SV::GraphicsDevice& device)
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
bool SV::_internal::FrameBuffer_internal::Resize(ui32 width, ui32 height, SV::GraphicsDevice& device)
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
bool SV::_internal::Shader_internal::Create(SV_GFX_SHADER_TYPE type, const char* filePath, SV::GraphicsDevice& device)
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
bool SV::_internal::InputLayout_internal::Create(const SV_GFX_INPUT_ELEMENT_DESC* desc, ui32 count, const Shader& vs, SV::GraphicsDevice& device)
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
bool SV::_internal::Texture_internal::Create(void* data, ui32 width, ui32 height, SV_GFX_FORMAT format, SV_GFX_USAGE usage, bool CPUWriteAccess, bool CPUReadAccess, SV::GraphicsDevice& device)
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
bool SV::_internal::Texture_internal::Resize(void* data, ui32 width, ui32 height, SV::GraphicsDevice& device)
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
bool SV::_internal::Sampler_internal::Create(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, GraphicsDevice& device)
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