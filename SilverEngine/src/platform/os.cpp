#include "platform/os.h"

#define STBI_ASSERT(x) SV_ASSERT(x)
#define STBI_MALLOC(size) sv::allocate_memory(size)
// TODO
#define STBI_REALLOC(ptr, size) realloc(ptr, size)
#define STBI_FREE(ptr) sv::free_memory(ptr)
#define STB_IMAGE_IMPLEMENTATION

#include "external/stbi_lib.h"

namespace sv {

    bool load_image(const char* filepath, void** pdata, u32* width, u32* height)
    {
	int w = 0, h = 0, bits = 0;
	void* data = stbi_load(filepath, &w, &h, &bits, 4);

	* pdata = nullptr;
	*width = w;
	*height = h;

	if (!data) return false;
	*pdata = data;
	return true;
    }

    // TODO: Platform specific!!

    ///////////////////////////////////////////////// TIMER /////////////////////////////////////////////////

    static std::chrono::steady_clock::time_point g_InitialTime = std::chrono::steady_clock::now();

    static std::chrono::steady_clock::time_point timer_now_chrono()
    {
	return std::chrono::steady_clock::now();
    }

    f64 timer_now()
    {
	return f64(std::chrono::duration<float>(timer_now_chrono() - g_InitialTime).count());
    }

    Date timer_date()
    {
	__time64_t t;
	_time64(&t);
	tm time;
	_localtime64_s(&time, &t);

	Date date;
	date.year = time.tm_year + 1900;
	date.month = time.tm_mon + 1;
	date.day = time.tm_mday;
	date.hour = time.tm_hour;
	date.minute = time.tm_min;
	date.second = time.tm_sec;

	return date;
    }
    
}
