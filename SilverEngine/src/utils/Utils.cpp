#include "core.h"

#include "graphics/Win.h"

#define STB_IMAGE_IMPLEMENTATION
#include "..//external/stbi_lib.h"

#undef LoadImage

namespace SV {
	namespace Utils {
		std::wstring ToWString(const char* c)
		{
			std::wstring str;
			str.resize(strlen(c));
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, c, str.size(), &str[0], str.size());
			return str;
		}
		/*
		bool LoadImage(const char* filePath, void** pdata, ui32* width, ui32* height, SV_GFX_FORMAT* pformat) {

			int w = 0, h = 0, bits = 0;
			void* data = stbi_load(filePath, &w, &h, &bits, 4);

			*pdata = nullptr;
			*width = w;
			*height = h;
			*pformat = SV_GFX_FORMAT_R8G8B8A8_UNORM;

			if (!data) return false;
			*pdata = data;
			return true;
		}
		*/
	}
}