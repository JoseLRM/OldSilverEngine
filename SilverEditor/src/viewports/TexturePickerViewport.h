#pragma once

#include "high_level/asset_system.h"

namespace sve {

	class TexturePickerPopUp {
	public:
		TexturePickerPopUp();

		bool getTextureName(const char** pName);

	};

}