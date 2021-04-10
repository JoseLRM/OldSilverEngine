#include "os.h"

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
    
}
