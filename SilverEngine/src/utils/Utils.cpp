#include "core.h"

#include "platform/windows/windows_impl.h"
#undef LoadImage

#define STB_IMAGE_IMPLEMENTATION
#include "..//external/stbi_lib.h"

namespace sv {

	static std::chrono::steady_clock::time_point g_InitialTime;

	static std::chrono::steady_clock::time_point timer_now_chrono()
	{
		return std::chrono::steady_clock::now();
	}

	Result utils_initialize()
	{
		g_InitialTime = timer_now_chrono();
		return Result_Success;
	}
	Result utils_close()
	{
		return Result_Success;
	}

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
		size_t size = wcslen(c) + 1u;
		std::string str;
		str.resize(size);
		size_t i;
		wcstombs_s(&i, str.data(), size, c, size);
		return str;
	}

	// Loader

	Result utils_loader_image(const char* filePath, void** pdata, ui32* width, ui32* height)
	{
		int w = 0, h = 0, bits = 0;

#ifdef SV_SRC_PATH
		std::string filePathStr = SV_SRC_PATH;
		filePathStr += filePath;
		void* data = stbi_load(filePathStr.c_str(), &w, &h, &bits, 4);
#else
		void* data = stbi_load(filePath, &w, &h, &bits, 4);
#endif

		*pdata = nullptr;
		*width = w;
		*height = h;

		if (!data) return Result_NotFound;
		*pdata = data;
		return Result_Success;
	}

	// Timer

	Time timer_now()
	{
		return Time(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
	}

}