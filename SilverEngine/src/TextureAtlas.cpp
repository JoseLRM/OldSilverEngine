#include "core.h"

#include "TextureAtlas.h"
#include "Graphics.h"

namespace SV {

	TextureAtlas::TextureAtlas()
	{}
	TextureAtlas::~TextureAtlas()
	{}

	bool TextureAtlas::Create(const char* filePath, SV::Graphics& gfx)
	{
		SV_GFX_FORMAT format;
		ui32 width, height;
		void* data;

		bool result = SV::Utils::LoadImage(filePath, &data, &width, &height, &format);
		if (!result) {
			SV::LogE("Image '%s' not found", filePath);
			return false;
		}

		if (!gfx.CreateTexture(data, width, height, format, SV_GFX_USAGE_STATIC, SV_GFX_CPU_ACCESS_NONE, m_Texture))
			return false;

		m_HasTexture = true;

		delete[] data;

		return true;
	}

	void TextureAtlas::Release(Graphics& gfx)
	{
		gfx.ReleaseTexture(m_Texture);
		gfx.ReleaseSampler(m_Sampler);
	}

	bool TextureAtlas::SetSamplerState(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, SV::Graphics& gfx)
	{
		if (!gfx.CreateSampler(addressMode, filter, m_Sampler))
			return false;

		m_HasSampler = true;

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