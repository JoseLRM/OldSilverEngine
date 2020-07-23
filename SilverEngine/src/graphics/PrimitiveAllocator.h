#pragma once

#include "..//core.h"
#include "GraphicsDesc.h"

namespace SV {

	class Primitive;
	
	namespace _internal {

		class Primitive_internal;
		
		typedef void*(*PrimitiveConstructor)(SV_GFX_PRIMITIVE, const void*);
		typedef bool(*PrimitiveDestructor)(Primitive&);

		class PrimitiveAllocator {
			std::vector<Primitive_internal*> m_Data;

			PrimitiveConstructor m_Constructor;
			PrimitiveDestructor m_Destructor;

			std::mutex m_Mutex;

		public:
			void* Construct(SV_GFX_PRIMITIVE type, const void* desc);
			bool Destroy(Primitive& primitive);
			void SetFunctions(PrimitiveConstructor constructor, PrimitiveDestructor destructor);
			void Clear();

		};
	}

}