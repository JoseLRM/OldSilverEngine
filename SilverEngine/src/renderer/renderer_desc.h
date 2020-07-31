#pragma once

#include "core.h"
#include "RenderList.h"
#include "TextureAtlas.h"

enum SV_REND_SORT_MODE : ui8 {
	SV_REND_SORT_MODE_NONE,
	SV_REND_SORT_MODE_COORD_X,
	SV_REND_SORT_MODE_COORD_Y,
	SV_REND_SORT_MODE_COORD_Z,
};

namespace _sv {

	struct SpriteInstance {
		XMMATRIX tm;
		sv::Sprite sprite;
		sv::Color color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, sv::Sprite sprite, sv::Color color) : tm(m), sprite(sprite), color(color) {}
	};

	struct RenderLayer {
		sv::RenderList<SpriteInstance>	sprites;
		SV_REND_SORT_MODE					sortMode;
		i16									sortValue;

		void Reset()
		{
			sprites.Reset();
		}
	};

}

namespace sv {

	typedef void* RenderLayerID;

}

constexpr sv::RenderLayerID SV_RENDER_LAYER_DEFAULT = nullptr;



