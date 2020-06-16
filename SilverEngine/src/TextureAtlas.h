#pragma once

#include "core.h"
#include "Graphics.h"

namespace SV {

	class TextureAtlas;

	struct Sprite {
		TextureAtlas* pTextureAtlas;
		ui32 ID;
	};

	class TextureAtlas {
		SV::Texture m_Texture;
		SV::Sampler m_Sampler;

		bool m_HasTexture = false;
		bool m_HasSampler = false;

		std::vector<SV::vec4> m_Sprites;

	public:
		TextureAtlas();
		~TextureAtlas();

		bool Create(const char* filePath, SV::Graphics& gfx);
		void Release(Graphics& gfx);

		bool SetSamplerState(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, SV::Graphics& gfx);

		inline SV::Texture& GetTexture() noexcept { return m_Texture; }
		inline SV::Sampler& GetSampler() noexcept { return m_Sampler; }

		Sprite CreateSprite(float x0, float y0, float w, float h) noexcept;
		SV::vec4 GetSpriteTexCoords(ui32 ID) const noexcept;

	};

}