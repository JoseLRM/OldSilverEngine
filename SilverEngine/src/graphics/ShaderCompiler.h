#pragma once

#include "GraphicsDesc.h"

namespace SV {
	namespace Graphics {

		bool GetShaderBinPath(const char* filePath, SV_GFX_API api, std::string& binPath);

	}
}