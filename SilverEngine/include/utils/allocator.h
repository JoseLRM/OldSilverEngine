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

}