#pragma once

#include "core.h"

namespace sv {

	class SizedInstanceAllocator {

		struct Pool {
			ui8* data = nullptr;
			size_t size = 0u;
			void* freeList = nullptr;
		};

		std::vector<Pool> m_Pools;
		size_t m_PoolSize;
		const size_t INSTANCE_SIZE;

	public:
		SizedInstanceAllocator(size_t instanceSize, size_t poolSize = 100u);
		~SizedInstanceAllocator();

		void* alloc();
		void free(void* ptr);
		void clear();

		bool has_unfreed() const noexcept;
		ui32 unfreed_count() const noexcept;

	};

	template<typename T>
	class InstanceAllocator {
		
		SizedInstanceAllocator m_Allocator;

	public:
		InstanceAllocator(size_t poolSize = 100u) : m_Allocator(sizeof(T), poolSize) {}
		~InstanceAllocator() = default;

		template<typename... Args>
		T* create(Args&& ...args)
		{
			T* instance = (T*)m_Allocator.alloc();
			new(instance) T(std::forward<Args>(args)...);
			return instance;
		}

		void destroy(T* instance)
		{
			instance->~T();
			m_Allocator.free(instance);
		}

		inline void clear() { m_Allocator.clear(); }
		inline bool has_unfreed() const noexcept { return m_Allocator.has_unfreed(); }
		inline ui32 unfreed_count() const noexcept { return m_Allocator.unfreed_count(); }

	};

}