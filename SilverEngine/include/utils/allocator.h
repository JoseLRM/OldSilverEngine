#pragma once

#include "core.h"

namespace sv {

	// SIZED INSTANCE ALLOCATOR

	struct SizedInstanceAllocatorPoolIterator {

		using iterator = SizedInstanceAllocatorPoolIterator;

		ui8* ptr;
		ui8* nextFreeInstance;
		const size_t INSTANCE_SIZE;

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

		inline iterator(ui8* ptr, ui8* nextFreeInstance, size_t instanceSize)
			: ptr(ptr), nextFreeInstance(nextFreeInstance), INSTANCE_SIZE(instanceSize)
		{
			checkInvalidInstances();
		}

	private:
		inline void checkInvalidInstances()
		{
			while (ptr == nextFreeInstance) {
				ui32* nextCount = reinterpret_cast<ui32*>(nextFreeInstance);
				if (*nextCount == ui32_max) {
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

		ui8* instances = nullptr;
		ui32 size = 0u;
		ui32 beginCount = ui32_max;
		const size_t INSTANCE_SIZE;

		SizedInstanceAllocatorPool(size_t instanceSize);
		SizedInstanceAllocatorPool(const SizedInstanceAllocatorPool& other);
		SizedInstanceAllocatorPool& operator=(const SizedInstanceAllocatorPool& other);

		ui32 unfreed_count() const noexcept;

		iterator begin() const;
		iterator end() const;

	};

	class SizedInstanceAllocator {
	public:

		using Pool = SizedInstanceAllocatorPool;
		using PoolIterator = SizedInstanceAllocatorPoolIterator;

		SizedInstanceAllocator(size_t instanceSize, size_t poolSize);
		~SizedInstanceAllocator();

		void* alloc();
		void free(void* ptr_);
		void clear();

		inline ui32 pool_count() { ui32(m_Pools.size()); }
		inline SizedInstanceAllocatorPool& operator[](size_t i) noexcept { return m_Pools[i]; };
		inline const SizedInstanceAllocatorPool& operator[](size_t i) const noexcept { return m_Pools[i]; };

		ui32 unfreed_count() const noexcept;

		inline auto begin()
		{
			return m_Pools.begin();
		}
		inline auto end()
		{
			return m_Pools.end();
		}

	private:
		ui32* findLastNextCount(void* ptr, Pool& pool);

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
		ui8* nextFreeInstance;

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

		iterator(T* ptr, ui8* nextFreeInstance)
			: ptr(ptr), nextFreeInstance(nextFreeInstance)
		{
			checkInvalidInstances();
		}

	private:
		inline void checkInvalidInstances()
		{
			while (reinterpret_cast<ui8*>(ptr) == nextFreeInstance) {
				ui32* nextCount = reinterpret_cast<ui32*>(nextFreeInstance);
				if (*nextCount == ui32_max) {
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
		ui32 size = 0u;
		ui32 beginCount = ui32_max;

		InstanceAllocatorPool() {}
		InstanceAllocatorPool(const InstanceAllocatorPool& other)
		{
			instances = other.instances;
			size = other.size;
			beginCount = other.beginCount;
		}
		InstanceAllocatorPool& operator=(const InstanceAllocatorPool& other)
		{
			instances = other.instances;
			size = other.size;
			beginCount = other.beginCount;
			return *this;
		}

		ui32 unfreed_count() const noexcept
		{
			ui32 unfreed = 0u;
			for (auto it = begin(); it != end(); ++it) {
				++unfreed;
			}
			return unfreed;
		}

		iterator begin() const
		{
			if (beginCount == ui32_max) return iterator(instances, (ui8*)& beginCount);
			return iterator(instances, reinterpret_cast<ui8*>(instances + beginCount));
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
			static_assert(sizeof(T) >= sizeof(ui32), "The size must be greater than 4u");
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
			else if (m_Pools.back().beginCount == ui32_max && m_Pools.back().size == POOL_SIZE) {

				ui32 poolIndex;

				// Find available freelist
				for (ui32 i = 0u; i < m_Pools.size(); ++i) {
					if (m_Pools[i].beginCount != ui32_max) {
						poolIndex = i;
						goto found;
					}
				}

				// Find available size
				for (ui32 i = 0u; i < m_Pools.size(); ++i) {
					if (m_Pools[i].size < POOL_SIZE) {
						poolIndex = i;
						goto found;
					}
				}

				// Emplace new pool
				poolIndex = ui32(m_Pools.size());
				m_Pools.emplace_back();

			found:
				Pool aux = m_Pools[poolIndex];
				m_Pools[poolIndex] = m_Pools.back();
				m_Pools.back() = aux;

			}

			Pool& pool = m_Pools.back();

			// Allocate
			if (pool.instances == nullptr) {
				pool.instances = (T*) operator new(POOL_SIZE * sizeof(T));
			}

			// Get ptr
			T* ptr;

			if (pool.beginCount != ui32_max) {
				ptr = pool.instances + pool.beginCount;

				// Change nextCount
				ui32* freeInstance = reinterpret_cast<ui32*>(ptr);

				if (*freeInstance == ui32_max)
					pool.beginCount = ui32_max;
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
				ui32* nextCount = findLastNextCount(ptr, *pool);

				ui32 distance;

				if (nextCount == &pool->beginCount) {
					distance = ui32(ptr - pool->instances);
				}
				else {
					if ((void*)nextCount >= ptr)
						system("PAUSE");

					distance = ui32(ptr - reinterpret_cast<T*>(nextCount));
				}

				ui32* freeInstance = reinterpret_cast<ui32*>(ptr);

				if (*nextCount == ui32_max)
					* freeInstance = ui32_max;
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

		inline ui32 pool_count() { ui32(m_Pools.size()); }
		inline Pool& operator[](size_t i) noexcept { return m_Pools[i]; };
		inline const Pool& operator[](size_t i) const noexcept { return m_Pools[i]; };

		ui32 unfreed_count() const noexcept
		{
			ui32 unfreed = 0u;
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
		ui32* findLastNextCount(void* ptr, Pool& pool)
		{
			ui32* nextCount;

			if (pool.beginCount == ui32_max) return &pool.beginCount;

			T* next = pool.instances + pool.beginCount;
			T* end = pool.instances + pool.size;

			if (next >= ptr) nextCount = &pool.beginCount;
			else {
				T* last = next;

				while (next != end) {

					ui32 nextCount = *reinterpret_cast<ui32*>(last);
					if (nextCount == ui32_max) break;

					next += nextCount;
					if (next > ptr) break;

					last = next;
				}
				nextCount = reinterpret_cast<ui32*>(last);
			}

			return nextCount;
		}

	private:
		std::vector<Pool> m_Pools;

	};

	// FRAME LIST

	template<typename T>
	class FrameList {
		T*		m_Data = nullptr;
		size_t	m_Size = 0u;
		size_t	m_Capacity = 0u;

	public:
		FrameList() = default;
		~FrameList()
		{
			clear();
		}

		template<typename... Args>
		T& emplace_back(Args&& ...args)
		{
			T* t = add();
			new(t) T(std::forward<Args>(args)...);
			return *t;
		}

		void push_back(const T& o)
		{
			T* t = add();
			new(t) T(o);
		}

		void push_back(T&& o)
		{
			T* t = add();
			new(t) T(std::move(o));
		}

		void pop_back()
		{
			back().~T();
			--m_Size;
		}

		void reserve(size_t size)
		{
			if (m_Size + size > m_Capacity) realloc(m_Size + size);
		}

		T& operator[](size_t index)
		{
			SV_ASSERT(index < m_Size);
			return m_Data[index];
		}

		const T& operator[](size_t index) const
		{
			SV_ASSERT(index < m_Size);
			return m_Data[index];
		}

		bool empty() const noexcept
		{
			return m_Size == 0u;
		}
		size_t size() const noexcept
		{
			return m_Size;
		}
		size_t capacity() const noexcept
		{
			return m_Capacity;
		}
		T* data() noexcept
		{
			return m_Data;
		}
		const T* data() const noexcept
		{
			return m_Data;
		}

		T& back()
		{
			SV_ASSERT(m_Size != 0u);
			return m_Data[m_Size - 1u];
		}
		const T& back() const
		{
			SV_ASSERT(m_Size != 0u);
			return m_Data[m_Size - 1u];
		}
		T& front()
		{
			SV_ASSERT(m_Size != 0u);
			return *m_Data;
		}
		const T& front() const
		{
			SV_ASSERT(m_Size != 0u);
			return *m_Data;
		}

		void reset()
		{
			m_Size = 0u;
		}
		void clear()
		{
			if (m_Data != nullptr) {
				delete[] m_Data;
				m_Data = nullptr;
			}
			m_Size = 0u;
			m_Capacity = 0u;
		}

	private:
		void realloc(size_t size) 
		{
			T* newData = reinterpret_cast<T*>(operator new(size * sizeof(T)));
			
			if (m_Data) {
				if (size < m_Size) {
					T* it = m_Data + size;
					T* end = m_Data + m_Size;
					while (it != end) {
						it->~T();
						++it;
					}
					m_Size = size;
				}

				T* it0 = m_Data;
				T* it1 = newData;
				T* end = m_Data + m_Size;

				while (it0 != end) {

					new(it1) T(std::move(*it0));

					++it0;
					++it1;
				}

				operator delete(m_Data);
			}

			m_Data = newData;
			m_Capacity = size;
		}

		inline T* add()
		{
			if (m_Size == m_Capacity) 
				realloc(size_t(round(double(m_Capacity + 1) * 1.7)));
			return m_Data + m_Size++;
		}

	public:
		struct iterator {
			T* ptr;

			T* operator->() const noexcept { return ptr; }
			T& operator*() const noexcept { return *ptr; }

			iterator& operator++() noexcept { ++ptr; return *this; }
			iterator operator++(int) noexcept { iterator temp = *this; ++ptr; return temp; }
			iterator& operator+=(i64 n) noexcept { ptr += n; return *this; }

			iterator& operator--() noexcept { --ptr; return *this; }
			iterator operator--(int) noexcept { iterator temp = *this; --ptr; return temp; }
			iterator& operator-=(i64 n) noexcept { ptr -= n; return *this; }

			bool operator==(const iterator& other) const noexcept { return ptr == other.ptr; }
			bool operator!=(const iterator& other) const noexcept { return ptr != other.ptr; }
			bool operator<(const iterator& other) const noexcept { return ptr < other.ptr; }
			bool operator<=(const iterator& other) const noexcept { return ptr <= other.ptr; }
			bool operator>(const iterator& other) const noexcept { return ptr > other.ptr; }
			bool operator>=(const iterator& other) const noexcept { return ptr >= other.ptr; }

			iterator(T* ptr) : ptr(ptr) {}
		};

		struct const_iterator {
			const T* ptr;

			const T* operator->() const noexcept { return ptr; }
			const T& operator*() const noexcept { return *ptr; }

			const_iterator& operator++() noexcept { ++ptr; return *this; }
			const_iterator operator++(int) noexcept { const_iterator temp = *this; ++ptr; return temp; }
			const_iterator& operator+=(i64 n) noexcept { ptr += n; return *this; }

			const_iterator& operator--() noexcept { --ptr; return *this; }
			const_iterator operator--(int) noexcept { const_iterator temp = *this; --ptr; return temp; }
			const_iterator& operator-=(i64 n) noexcept { ptr -= n; return *this; }

			bool operator==(const const_iterator& other) const noexcept { return ptr == other.ptr; }
			bool operator!=(const const_iterator& other) const noexcept { return ptr != other.ptr; }
			bool operator<(const const_iterator& other) const noexcept { return ptr < other.ptr; }
			bool operator<=(const const_iterator& other) const noexcept { return ptr <= other.ptr; }
			bool operator>(const const_iterator& other) const noexcept { return ptr > other.ptr; }
			bool operator>=(const const_iterator& other) const noexcept { return ptr >= other.ptr; }

			const_iterator(const T* ptr) : ptr(ptr) {}
		};

		iterator begin() noexcept
		{
			return iterator(m_Data);
		}
		const_iterator cbegin() const noexcept
		{
			return const_iterator(m_Data);
		}

		iterator end() noexcept
		{
			return iterator(m_Data + m_Size);
		}
		const_iterator cend() const noexcept
		{
			return const_iterator(m_Data + m_Size);
		}

	};

}