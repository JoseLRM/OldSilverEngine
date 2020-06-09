#include "core.h"

#include "TextureAtlas.h"

namespace SV {

	TextureAtlas::TextureAtlas()
	{}
	TextureAtlas::~TextureAtlas()
	{
		Release();
	}

	bool TextureAtlas::Create(const char* filePath, SV::GraphicsDevice& device)
	{
		device.ValidateTexture(&m_Texture);

		SV_GFX_FORMAT format;
		ui32 width, height;
		void* data;

		bool result = SV::Utils::LoadImage(filePath, &data, &width, &height, &format);
		if (!result) {
			SV::LogE("Image '%s' not found", filePath);
			return false;
		}

		if (!m_Texture->Create(data, width, height, format, SV_GFX_USAGE_STATIC, false, false, device))
			return false;

		m_HasTexture = true;

		return true;
	}

	void TextureAtlas::Release()
	{
		if (m_Texture.IsValid()) m_Texture->Release();
		if (m_Sampler.IsValid()) m_Sampler->Release();
	}

	bool TextureAtlas::SetSamplerState(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, SV::GraphicsDevice& device)
	{
		device.ValidateSampler(&m_Sampler);

		if (!m_Sampler->Create(addressMode, filter, device))
			return false;

		m_HasSampler = true;

		return true;
	}

	void TextureAtlas::Bind(SV_GFX_SHADER_TYPE type, ui32 slot, SV::CommandList& cmd)
	{
		if (!CheckPrimitives(cmd.GetDevice())) return;

		m_Texture->Bind(type, slot, cmd);
		m_Sampler->Bind(type, slot, cmd);
	}
	void TextureAtlas::Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, SV::CommandList& cmd)
	{
		if (!CheckPrimitives(cmd.GetDevice())) return;

		m_Texture->Unbind(type, slot, cmd);
		m_Sampler->Unbind(type, slot, cmd);
	}

	bool TextureAtlas::CheckPrimitives(SV::GraphicsDevice& device)
	{
		if (!m_HasTexture) return false;
		if (!m_HasSampler) SetSamplerState(SV_GFX_TEXTURE_ADDRESS_WRAP, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_LINEAR, device);
		return true;
	}

	Sprite TextureAtlas::CreateSprite(float x0, float y0, float w, float h) noexcept
	{
		Sprite sprite;
		sprite.ID = ui32(m_Sprites.size());
		sprite.pTextureAtlas = this;

		m_Sprites.emplace_back(x0, y0, w, h);
		return sprite;
	}

	SV::vec4 TextureAtlas::GetSpriteTexCoords(ui32 ID) const noexcept
	{
		return m_Sprites[ID];
	}

}