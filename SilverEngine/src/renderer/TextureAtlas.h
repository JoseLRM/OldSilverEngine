#pragma once

#include "graphics/graphics.h"

namespace _sv {

	struct TextureAtlas_DrawData {
		sv::Image				image;
		sv::Sampler				sampler;
		std::vector<sv::vec4>	sprites;
	};

}

namespace sv {

	struct Sprite {
		_sv::TextureAtlas_DrawData* pTexAtlas = nullptr;
		ui32 ID = 0u;
	};

	class TextureAtlas {
		std::unique_ptr<_sv::TextureAtlas_DrawData> m_DrawData;

	public:
		bool CreateFromFile(const char* filePath, bool linearFilter, SV_GFX_ADDRESS_MODE addressMode);

		bool Destroy();

		Sprite AddSprite(float x, float y, float w, float h);
		vec4 operator[](ui32 ID) const noexcept;

	};

}