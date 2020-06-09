#pragma once

#include "core.h"
#include "GraphicsDesc.h"

namespace SV {
	namespace Utils {
		std::wstring ToWString(const char* c);

		bool LoadImage(const char* filePath, void** pdata, ui32* width, ui32* height, SV_GFX_FORMAT* pformat);
	}
}