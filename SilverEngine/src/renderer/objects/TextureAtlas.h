#pragma once

#include "graphics.h"

namespace sv {

	class TextureAtlas;

	struct Sprite {
		TextureAtlas*	pTextureAtlas;
		ui32			index;
	};

	class TextureAtlas {
		GPUImage			m_Image;
		Sampler				m_Sampler;
		std::vector<vec4>	m_Sprites;

	public:
		bool CreateFromFile(const char* filePath, bool linearFilter, SamplerAddressMode addressMode);
		bool Destroy();

		Sprite AddSprite(float x, float y, float w, float h);
		inline ui32 GetSpriteCount() const { return m_Sprites.size(); }
		inline vec4 GetSprite(ui32 index) { return m_Sprites[index]; }

		inline GPUImage& GetImage() { return m_Image; }
		inline Sampler& GetSampler() { return m_Sampler; }

	};

}