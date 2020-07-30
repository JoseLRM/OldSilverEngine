#pragma once

#include "core.h"
#include "RenderList.h"

enum SV_REND_SORT_MODE : ui8 {
	SV_REND_SORT_MODE_NONE,
	SV_REND_SORT_MODE_COORD_X,
	SV_REND_SORT_MODE_COORD_Y,
	SV_REND_SORT_MODE_COORD_Z,
};

namespace _sv {

	struct SpriteInstance {
		XMMATRIX tm;
		sv::Color color;

		SpriteInstance() = default;
		SpriteInstance(const XMMATRIX& m, sv::Color color) : tm(m), color(color) {}
	};

}

namespace sv {

	struct RenderLayer {
		RenderList<_sv::SpriteInstance>		sprites;
		SV_REND_SORT_MODE							sortMode;
		i16											sortValue;

		void Reset()
		{
			sprites.Reset();
		}
	};
}



