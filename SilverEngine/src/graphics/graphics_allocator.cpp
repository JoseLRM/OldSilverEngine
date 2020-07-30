#include "core.h"

#include "graphics_allocator.h"
#include "graphics.h"

using namespace sv;

namespace _sv {

	/////////////////////////////////////////////////////// POOL ///////////////////////////////////////////////////////////
	void* PrimitiveAllocator::Construct(SV_GFX_PRIMITIVE type, const void* desc)
	{
		void* ptr = m_Constructor(type, desc);
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			m_Data.push_back(reinterpret_cast<Primitive_internal*>(ptr));

			// TODO: Binary insertion
		}
		return ptr;
	}
	bool PrimitiveAllocator::Destroy(Primitive& primitive)
	{
		if (primitive.GetPtr() == nullptr) return true;

		// Remove ptr from data
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			
			// TODO: Binary search
			m_Data.erase(std::find(m_Data.begin(), m_Data.end(), reinterpret_cast<Primitive_internal*>(primitive.GetPtr())));
		}

		// Deallocate
		return m_Destructor(primitive);
	}

	void PrimitiveAllocator::SetFunctions(PrimitiveConstructor constructor, PrimitiveDestructor destructor)
	{
		m_Constructor = constructor;
		m_Destructor = destructor;
	}
	void PrimitiveAllocator::Clear()
	{
		// Call Destructors
		for (ui32 i = 0; i < m_Data.size(); ++i) {
			Primitive_internal* ptr = m_Data[i];
			Primitive primitive(ptr);

			m_Destructor(primitive);
		}

		// Free
		m_Data.clear();
	}
}