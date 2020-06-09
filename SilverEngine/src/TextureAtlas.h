#pragma once

#include "core.h"

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

		bool Create(const char* filePath, SV::GraphicsDevice& device);
		void Release();

		bool SetSamplerState(SV_GFX_TEXTURE_ADDRESS_MODE addressMode, SV_GFX_TEXTURE_FILTER filter, SV::GraphicsDevice& device);

		void Bind(SV_GFX_SHADER_TYPE type, ui32 slot, SV::CommandList& cmd);
		void Unbind(SV_GFX_SHADER_TYPE type, ui32 slot, SV::CommandList& cmd);

		Sprite CreateSprite(float x0, float y0, float w, float h) noexcept;
		SV::vec4 GetSpriteTexCoords(ui32 ID) const noexcept;

	private:
		bool CheckPrimitives(SV::GraphicsDevice& device);

	};

}