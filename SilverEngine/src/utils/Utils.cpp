#include "core.h"

#include "platform/windows_impl.h"
#undef LoadImage

#define STB_IMAGE_IMPLEMENTATION
#include "..//external/stbi_lib.h"

namespace _sv {

	static std::chrono::steady_clock::time_point g_InitialTime;

	static std::chrono::steady_clock::time_point timer_now_chrono()
	{
		return std::chrono::steady_clock::now();
	}

	bool utils_initialize()
	{
		g_InitialTime = timer_now_chrono();
		return true;
	}
	bool utils_close()
	{
		return true;
	}

}

namespace sv {

	using namespace _sv;

	// String
	
	std::wstring utils_wstring_parse(const char* c)
	{
		std::wstring str;
		str.resize(strlen(c));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, c, str.size(), &str[0], str.size());
		return str;
	}

	std::string utils_string_parse(const wchar* c)
	{
		sv::log_error("Undefined 'utils_string_parse'");
		return "";
	}

	// Timer

	Time timer_now()
	{
		return Time(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
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