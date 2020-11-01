#pragma once

#include "core.h"

namespace sv {

	class SizedInstanceAllocator {

		struct Pool {
			ui8* data = nullptr;
			size_t size = 0u;
			void* freeList = nullptr;
		};

		std::mutex m_Mutex;
		std::vector<Pool> m_Pools;
		size_t m_PoolSize;
		const size_t INSTANCE_SIZE;

	public:
		SizedInstanceAllocator(size_t instanceSize, size_t poolSize = 100u);
		~SizedInstanceAllocator();

		void* alloc();
		void free(void* ptr);
		void clear();

		bool has_unfreed() noexcept;
		ui32 unfreed_count() noexcept;
		bool empty() const noexcept;

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
		inline bool has_unfreed() noexcept { return m_Allocator.has_unfreed(); }
		inline ui32 unfreed_count() noexcept { return m_Allocator.unfreed_count(); }
		inline bool empty() const noexcept { return m_Allocator.empty(); }

	};

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

	template<typename T>
	class IterableInstanceAllocator {
	public:

		struct Instance {
			T obj;
			ui32 nextCount = 0u;
		};

		const ui32 POOL_SIZE;

		struct Pool {
			Instance* instances = nullptr;
			Instance* freeList = nullptr;
			ui32 size = 0u;
			ui32 beginCount = 0u;

			struct iterator {
				Instance* ptr;

				T* operator->() const noexcept { return reinterpret_cast<T*>(ptr); }
				T& operator*() const noexcept { return *reinterpret_cast<T*>(ptr); }

				iterator& operator++() noexcept { next(); return *this; }
				iterator operator++(int) noexcept { iterator temp = *this; next(); return temp; }
				iterator& operator+=(i64 n) noexcept { while(n-- > 0) next(); return *this; }

				bool operator==(const iterator& other) const noexcept { return ptr == other.ptr; }
				bool operator!=(const iterator& other) const noexcept { return ptr != other.ptr; }
				bool operator<(const iterator& other) const noexcept { return ptr < other.ptr; }
				bool operator<=(const iterator& other) const noexcept { return ptr <= other.ptr; }
				bool operator>(const iterator& other) const noexcept { return ptr > other.ptr; }
				bool operator>=(const iterator& other) const noexcept { return ptr >= other.ptr; }

				iterator(Instance* ptr) : ptr(ptr) {}

			private:
				void next()
				{
					SV_ASSERT(ptr->nextCount != 0u);
					ptr += ptr->nextCount;
				}

			};

			iterator begin()
			{
				SV_ASSERT(instances);
				return iterator(instances + beginCount);
			}

			iterator end()
			{
				SV_ASSERT(instances);
				return iterator(instances + size);
			}

		};

	public:
		IterableInstanceAllocator(ui32 poolSize = 100u) : POOL_SIZE(poolSize)
		{
			static_assert(sizeof(T) >= sizeof(void*), "The size of this object must be greater or equals than sizeof(void*)");
		}

		T* create()
		{
			// Make sure that m_Pools.back() has a valid Pool
			{
				if (m_Pools.empty()) m_Pools.emplace_back();

				if (m_Pools.back().freeList == nullptr && m_Pools.back().size == POOL_SIZE) {

					ui32 nextPoolIndex = ui32_max;

					// Find pool with available freelist
					for (ui32 i = 0; i < m_Pools.size(); ++i) {
						if (m_Pools[i].freeList) {
							nextPoolIndex = i;
							goto swap;
						}
					}

					// Find pool with available size
					for (ui32 i = 0; i < m_Pools.size(); ++i) {
						if (m_Pools[i].size < POOL_SIZE) {
							nextPoolIndex = i;
							goto swap;
						}
					}

				swap:
					// if not found create new one
					if (nextPoolIndex == ui32_max) {
						m_Pools.emplace_back();
					}
					// else swap pools
					else {
						Pool aux = m_Pools.back();
						m_Pools.back() = m_Pools[nextPoolIndex];
						m_Pools[nextPoolIndex] = aux;
					}
				}
			}

			Pool& pool = m_Pools.back();
			Instance* res = nullptr;
			bool resized = false;

			if (pool.freeList) {
				res = pool.freeList;
				pool.freeList = *reinterpret_cast<Instance**>(res);
			}
			else if (pool.instances == nullptr) {
				pool.instances = (Instance*)operator new(POOL_SIZE * sizeof(Instance));
				pool.size = 0u;
				pool.beginCount = 0u;
				pool.freeList = nullptr;
			}

			if (res == nullptr) {
				resized = true;
				res = pool.instances + pool.size++;
			}

			new(res) Instance();

			// Change next count
			if (!resized) {
				if (res == pool.instances) {
					res->nextCount = pool.beginCount;
					pool.beginCount = 0u;
				}
				else {

					Instance* it = res - 1u;
					Instance* begin = pool.instances - 1u;

					while (it != begin) {

						if (it->nextCount != 0u) {
							break;
						}

						--it;
					}

					ui32& otherCount = (begin == it) ? pool.beginCount : it->nextCount;
					ui32 distance = ui32(res - it);
					SV_ASSERT(otherCount >= distance);

					res->nextCount = otherCount - distance;
					otherCount = distance;
				}
			}
			else res->nextCount = 1u;

			return &res->obj;
		}

		void destroy(T* obj)
		{
			if (m_Pools.empty()) return;

			obj->~T();
			Instance* ptr = reinterpret_cast<Instance*>(obj);

			Pool* pPool = findPool(ptr);

			if (pPool == nullptr) return;

			// Change the next count
			if (ptr->nextCount != 0u) {

				SV_ASSERT(ptr->nextCount != 0u);

				if (pPool->instances == ptr) {
					pPool->beginCount = ptr->nextCount;
				}
				else {

					Instance* begin = pPool->instances - 1u;
					Instance* it = ptr - 1u;

					// Find active animation
					while (it != begin) {

						if (it->nextCount != 0u && it->nextCount != ui32_max) {
							break;
						}

						--it;
					}

					if (begin == it) {
						pPool->beginCount += ptr->nextCount;
					}
					else it->nextCount += ptr->nextCount;
				}

				ptr->nextCount = 0u;
			}

			// Remove from pool
			memcpy(ptr, &pPool->freeList, sizeof(void*));
			pPool->freeList = ptr;

			// Destroy empty pool
			if (pPool->beginCount == pPool->size) {

				// free memory
				SV_ASSERT(pPool->instances);
				operator delete[](pPool->instances);

				for (auto it = m_Pools.begin(); it != m_Pools.end(); ++it) {
					if (&(*it) == pPool) {
						m_Pools.erase(it);
						break;
					}
				}
			}
		}

		void clear()
		{

		}

	private:
		Pool* findPool(Instance* ptr)
		{
			Pool* pPool = nullptr;

			for (auto& pool : m_Pools) {
				if (pool.instances <= ptr && pool.instances + pool.size > ptr) {
					pPool = &pool;
					break;
				}
			}

			return pPool;
		}

	private:
		std::vector<Pool> m_Pools;

	public:
		inline auto begin() 
		{
			return m_Pools.begin();
		}

		inline auto end()
		{
			return m_Pools.end();
		}

	};

}