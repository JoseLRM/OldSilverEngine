#pragma once

#include "core.h"

namespace sv {

	// SIZED INSTANCE ALLOCATOR

	struct SizedInstanceAllocatorPoolIterator {

		using iterator = SizedInstanceAllocatorPoolIterator;

		u8* ptr;
		u8* nextFreeInstance;
		size_t INSTANCE_SIZE;

		inline void* operator*() const noexcept { return ptr; }

		inline iterator& operator++() noexcept { next(); return *this; }
		inline iterator operator++(int) noexcept { iterator temp = *this; next(); return temp; }
		inline iterator& operator+=(i64 n) noexcept { while (n-- > 0) next(); return *this; }

		inline bool operator==(const iterator& other) const noexcept { return ptr == other.ptr; }
		inline bool operator!=(const iterator& other) const noexcept { return ptr != other.ptr; }
		inline bool operator<(const iterator& other) const noexcept { return ptr < other.ptr; }
		inline bool operator<=(const iterator& other) const noexcept { return ptr <= other.ptr; }
		inline bool operator>(const iterator& other) const noexcept { return ptr > other.ptr; }
		inline bool operator>=(const iterator& other) const noexcept { return ptr >= other.ptr; }

		inline iterator() : ptr(nullptr), nextFreeInstance(nullptr), INSTANCE_SIZE(0) {}
		inline iterator(u8* ptr, u8* nextFreeInstance, size_t instanceSize)
			: ptr(ptr), nextFreeInstance(nextFreeInstance), INSTANCE_SIZE(instanceSize)
		{
			checkInvalidInstances();
		}

	private:
		inline void checkInvalidInstances()
		{
			while (ptr == nextFreeInstance) {
				u32* nextCount = reinterpret_cast<u32*>(nextFreeInstance);
				if (*nextCount == u32_max) {
					ptr += INSTANCE_SIZE;
					break;
				}
				nextFreeInstance += *nextCount * INSTANCE_SIZE;
				ptr += INSTANCE_SIZE;
			}
		}

		inline void next()
		{
			ptr += INSTANCE_SIZE;
			checkInvalidInstances();
		}

	};

	struct SizedInstanceAllocatorPool {

		using iterator = SizedInstanceAllocatorPoolIterator;

		u8* instances = nullptr;
		u32 size = 0u;
		u32 beginCount = u32_max;
		size_t INSTANCE_SIZE;

		SizedInstanceAllocatorPool(size_t instanceSize);
		SizedInstanceAllocatorPool(SizedInstanceAllocatorPool&& other);
		SizedInstanceAllocatorPool& operator=(SizedInstanceAllocatorPool&& other) noexcept;

		u32 unfreed_count() const noexcept;

		iterator begin() const;
		iterator end() const;

	};

	class SizedInstanceAllocator {
	public:

		using Pool = SizedInstanceAllocatorPool;
		using PoolIterator = SizedInstanceAllocatorPoolIterator;

		SizedInstanceAllocator();
		SizedInstanceAllocator(size_t instanceSize, size_t poolSize);
		SizedInstanceAllocator(const SizedInstanceAllocator& other) = delete;
		SizedInstanceAllocator(SizedInstanceAllocator&& other) noexcept;
		~SizedInstanceAllocator();

		void* alloc();
		void free(void* ptr_);
		void clear();

		inline u32 pool_count() { return u32(m_Pools.size()); }
		inline SizedInstanceAllocatorPool& operator[](size_t i) noexcept { return m_Pools[i]; };
		inline const SizedInstanceAllocatorPool& operator[](size_t i) const noexcept { return m_Pools[i]; };

		u32 unfreed_count() const noexcept;

		inline auto begin()
		{
			return m_Pools.begin();
		}
		inline auto end()
		{
			return m_Pools.end();
		}

	private:
		u32* findLastNextCount(void* ptr, Pool& pool);

	private:
		std::vector<Pool> m_Pools;
		const size_t INSTANCE_SIZE;
		const size_t POOL_SIZE;

	};

	// TEMPLATED INSTANCE ALLOCATOR

	template<typename T>
	struct InstanceAllocatorPoolIterator {

		using iterator = InstanceAllocatorPoolIterator<T>;

		T* ptr;
		u8* nextFreeInstance;

		T& operator*() const noexcept { return *ptr; }
		T* operator->() const noexcept { return ptr; }

		iterator& operator++() noexcept { next(); return *this; }
		iterator operator++(int) noexcept { iterator temp = *this; next(); return temp; }
		iterator& operator+=(i64 n) noexcept { while (n-- > 0) next(); return *this; }

		bool operator==(const iterator& other) const noexcept { return ptr == other.ptr; }
		bool operator!=(const iterator& other) const noexcept { return ptr != other.ptr; }
		bool operator<(const iterator& other) const noexcept { return ptr < other.ptr; }
		bool operator<=(const iterator& other) const noexcept { return ptr <= other.ptr; }
		bool operator>(const iterator& other) const noexcept { return ptr > other.ptr; }
		bool operator>=(const iterator& other) const noexcept { return ptr >= other.ptr; }

		iterator(T* ptr, u8* nextFreeInstance)
			: ptr(ptr), nextFreeInstance(nextFreeInstance)
		{
			checkInvalidInstances();
		}

	private:
		inline void checkInvalidInstances()
		{
			while (reinterpret_cast<u8*>(ptr) == nextFreeInstance) {
				u32* nextCount = reinterpret_cast<u32*>(nextFreeInstance);
				if (*nextCount == u32_max) {
					++ptr;
					break;
				}
				nextFreeInstance += *nextCount * sizeof(T);
				++ptr;
			}
		}

		inline void next()
		{
			++ptr;
			checkInvalidInstances();
		}

	};

	template<typename T>
	struct InstanceAllocatorPool {

		using iterator = InstanceAllocatorPoolIterator<T>;

		T* instances = nullptr;
		u32 size = 0u;
		u32 beginCount = u32_max;

		InstanceAllocatorPool() {}
		InstanceAllocatorPool(InstanceAllocatorPool&& other)
		{
			instances = other.instances;
			size = other.size;
			beginCount = other.beginCount;
			other.instances = nullptr;
			other.size = 0u;
			other.beginCount = 0u;
		}
		InstanceAllocatorPool& operator=(InstanceAllocatorPool&& other) noexcept
		{
			instances = other.instances;
			size = other.size;
			beginCount = other.beginCount;
			other.instances = nullptr;
			other.size = 0u;
			other.beginCount = 0u;
			return *this;
		}

		u32 unfreed_count() const noexcept
		{
			u32 unfreed = 0u;
			for (auto it = begin(); it != end(); ++it) {
				++unfreed;
			}
			return unfreed;
		}

		iterator begin() const
		{
			if (beginCount == u32_max) return iterator(instances, (u8*)& beginCount);
			return iterator(instances, reinterpret_cast<u8*>(instances + beginCount));
		}

		iterator end() const
		{
			return iterator(instances + size, nullptr);
		}

	};

	template<typename T, size_t POOL_SIZE>
	class InstanceAllocator {
	public:

		using Pool = InstanceAllocatorPool<T>;
		using PoolIterator = InstanceAllocatorPoolIterator<T>;

		InstanceAllocator()
		{
			static_assert(sizeof(T) >= sizeof(u32), "The size must be greater than 4u");
		}
		~InstanceAllocator()
		{
			clear();
		}

		template<typename... Args>
		T& create(Args&& ... args)
		{
			// Validate back pool
			if (m_Pools.empty()) m_Pools.emplace_back();
			else if (m_Pools.back().beginCount == u32_max && m_Pools.back().size == POOL_SIZE) {

				u32 poolIndex;

				// Find available freelist
				for (u32 i = 0u; i < m_Pools.size(); ++i) {
					if (m_Pools[i].beginCount != u32_max) {
						poolIndex = i;
						goto found;
					}
				}

				// Find available size
				for (u32 i = 0u; i < m_Pools.size(); ++i) {
					if (m_Pools[i].size < POOL_SIZE) {
						poolIndex = i;
						goto found;
					}
				}

				// Emplace new pool
				poolIndex = u32(m_Pools.size());
				m_Pools.emplace_back();

			found:
				Pool aux = std::move(m_Pools[poolIndex]);
				m_Pools[poolIndex] = std::move(m_Pools.back());
				m_Pools.back() = std::move(aux);

			}

			Pool& pool = m_Pools.back();

			// Allocate
			if (pool.instances == nullptr) {
				pool.instances = (T*) operator new(POOL_SIZE * sizeof(T));
			}

			// Get ptr
			T* ptr;

			if (pool.beginCount != u32_max) {
				ptr = pool.instances + pool.beginCount;

				// Change nextCount
				u32* freeInstance = reinterpret_cast<u32*>(ptr);

				if (*freeInstance == u32_max)
					pool.beginCount = u32_max;
				else
					pool.beginCount += *freeInstance;
			}
			else {
				ptr = pool.instances + pool.size;
				++pool.size;
			}

			return *new(ptr) T(std::forward<Args>(args)...);
		}

		void destroy(T& obj)
		{
			T* ptr = &obj;

			// Find pool
			Pool* pool = nullptr;

			for (Pool& p : m_Pools) {
				if (p.instances <= ptr && p.instances + p.size > ptr) {
					pool = &p;
					break;
				}
			}

			if (pool == nullptr || pool->size == 0u) {
				return;
			}

			obj.~T();

			// Remove from list
			if (ptr == pool->instances + size_t(pool->size - 1u)) {
				--pool->size;
			}
			else {
				// Update next count
				u32* nextCount = findLastNextCount(ptr, *pool);

				u32 distance;

				if (nextCount == &pool->beginCount) {
					distance = u32(ptr - pool->instances);
				}
				else {
					if ((void*)nextCount >= ptr)
						system("PAUSE");

					distance = u32(ptr - reinterpret_cast<T*>(nextCount));
				}

				u32* freeInstance = reinterpret_cast<u32*>(ptr);

				if (*nextCount == u32_max)
					* freeInstance = u32_max;
				else
					*freeInstance = *nextCount - distance;

				*nextCount = distance;
			}
		}

		void clear()
		{
			for (Pool& pool : m_Pools) {
				for (T& t : pool) {
					t.~T();
				}
				operator delete(pool.instances);
			}
			m_Pools.clear();
		}

		inline u32 pool_count() { u32(m_Pools.size()); }
		inline Pool& operator[](size_t i) noexcept { return m_Pools[i]; };
		inline const Pool& operator[](size_t i) const noexcept { return m_Pools[i]; };

		u32 unfreed_count() const noexcept
		{
			u32 unfreed = 0u;
			for (auto it = m_Pools.cbegin(); it != m_Pools.cend(); ++it) {
				unfreed += it->unfreed_count();
			}
			return unfreed;
		}

		inline auto begin()
		{
			return m_Pools.begin();
		}
		inline auto end()
		{
			return m_Pools.end();
		}

	private:
		u32* findLastNextCount(void* ptr, Pool& pool)
		{
			u32* nextCount;

			if (pool.beginCount == u32_max) return &pool.beginCount;

			T* next = pool.instances + pool.beginCount;
			T* end = pool.instances + pool.size;

			if (next >= ptr) nextCount = &pool.beginCount;
			else {
				T* last = next;

				while (next != end) {

					u32 nextCount = *reinterpret_cast<u32*>(last);
					if (nextCount == u32_max) break;

					next += nextCount;
					if (next > ptr) break;

					last = next;
				}
				nextCount = reinterpret_cast<u32*>(last);
			}

			return nextCount;
		}

	private:
		std::vector<Pool> m_Pools;

	};

}
