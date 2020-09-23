#include "core.h"

#include "graphics_internal.h"

namespace sv {

	static std::vector<Primitive_internal*> g_Data;
	static std::mutex g_Mutex;

	Result graphics_allocator_construct(GraphicsPrimitiveType type, const void* desc, Primitive_internal** res)
	{
		svCheck(graphics_create_primitive(type, desc, res));
		{
			std::lock_guard<std::mutex> lock(g_Mutex);
			g_Data.push_back(*res);

			// TODO: Binary insertion
		}
		return Result_Success;
	}

	Result graphics_allocator_destroy(Primitive& primitive)
	{
		if (primitive.GetPtr() == nullptr) return Result_Success;

		// Remove ptr from data
		{
			std::lock_guard<std::mutex> lock(g_Mutex);
			
			// TODO: Binary search
			g_Data.erase(std::find(g_Data.begin(), g_Data.end(), reinterpret_cast<Primitive_internal*>(primitive.GetPtr())));
		}

		// Deallocate
		return graphics_destroy_primitive(primitive);
	}

	void graphics_allocator_clear()
	{
		// Call Destructors
		for (ui32 i = 0; i < g_Data.size(); ++i) {
			Primitive_internal* ptr = g_Data[i];
			Primitive primitive(ptr);

			graphics_destroy_primitive(primitive);
		}

		// Free
		g_Data.clear();
	}
}