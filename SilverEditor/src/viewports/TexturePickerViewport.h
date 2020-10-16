#pragma once

#include "high_level/asset_system.h"

namespace sve {

	class TexturePickerViewport {
	public:
		TexturePickerViewport();

		bool getTextureName(const char** pName);

	};

}