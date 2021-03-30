#pragma once

#include "core.h"

namespace sv {

	template<typename T>
	struct ListIterator {
		T* ptr;

		T* operator->() const noexcept { return ptr; }
		T& operator*() const noexcept { return *ptr; }

		ListIterator& operator++() noexcept { ++ptr; return *this; }
		ListIterator operator++(int) noexcept { ListIterator temp = *this; ++ptr; return temp; }
		ListIterator& operator+=(i64 n) noexcept { ptr += n; return *this; }

		ListIterator& operator--() noexcept { --ptr; return *this; }
		ListIterator operator--(int) noexcept { ListIterator temp = *this; --ptr; return temp; }
		ListIterator& operator-=(i64 n) noexcept { ptr -= n; return *this; }

		bool operator==(const ListIterator& other) const noexcept { return ptr == other.ptr; }
		bool operator!=(const ListIterator& other) const noexcept { return ptr != other.ptr; }
		bool operator<(const ListIterator& other) const noexcept { return ptr < other.ptr; }
		bool operator<=(const ListIterator& other) const noexcept { return ptr <= other.ptr; }
		bool operator>(const ListIterator& other) const noexcept { return ptr > other.ptr; }
		bool operator>=(const ListIterator& other) const noexcept { return ptr >= other.ptr; }

		ListIterator(T* ptr) : ptr(ptr) {}
	};

	template<typename T>
	struct ListConstIterator {
		const T* ptr;

		const T* operator->() const noexcept { return ptr; }
		const T& operator*() const noexcept { return *ptr; }

		ListConstIterator& operator++() noexcept { ++ptr; return *this; }
		ListConstIterator operator++(int) noexcept { ListConstIterator temp = *this; ++ptr; return temp; }
		ListConstIterator& operator+=(i64 n) noexcept { ptr += n; return *this; }

		ListConstIterator& operator--() noexcept { --ptr; return *this; }
		ListConstIterator operator--(int) noexcept { ListConstIterator temp = *this; --ptr; return temp; }
		ListConstIterator& operator-=(i64 n) noexcept { ptr -= n; return *this; }

		bool operator==(const ListConstIterator& other) const noexcept { return ptr == other.ptr; }
		bool operator!=(const ListConstIterator& other) const noexcept { return ptr != other.ptr; }
		bool operator<(const ListConstIterator& other) const noexcept { return ptr < other.ptr; }
		bool operator<=(const ListConstIterator& other) const noexcept { return ptr <= other.ptr; }
		bool operator>(const ListConstIterator& other) const noexcept { return ptr > other.ptr; }
		bool operator>=(const ListConstIterator& other) const noexcept { return ptr >= other.ptr; }

		ListConstIterator(const T* ptr) : ptr(ptr) {}
	};

	template<typename T>
	struct List {

		List() = default;
		~List()
		{
			clear();
		}

		List(const List& other)
		{
			if (other._data) {

				_data = (T*)malloc(sizeof(T) * other._size);
				
				foreach(i, other._size) {
					_data[i] = other._data[i];
				}
				_size = other._size;
				_capacity = other._capacity;
			}
			else {
				_size = 0u;
				_capacity = 0u;
				_data = nullptr;
			}
		}

		List(List&& other)
		{
			_data = other._data;
			_size = other._size;
			_capacity = other._capacity;
			other._data = nullptr;
			other._size = 0u;
			other._capacity = 0u;
		}

		List& operator=(const List& other) 
		{
			clear();

			if (other._data) {

				_data = (T*)malloc(sizeof(T) * other.size);

				foreach(i, other._size) {
					_data[i] = other._data[i];
				}
				_size = other._size;
				_capacity = other._capacity;
			}
			else {
				_size = 0u;
				_capacity = 0u;
				_data = nullptr;
			}

			return *this;
		}

		List& operator=(List&& other) 
		{
			clear();

			_data = other._data;
			_size = other._size;
			_capacity = other._capacity;
			other._data = nullptr;
			other._size = 0u;
			other._capacity = 0u;

			return *this;
		}

		template<typename... Args>
		T& emplace_back(Args&& ...args)
		{
			T* t = _add();
			new(t) T(std::forward<Args>(args)...);
			return *t;
		}

		void push_back(const T& o)
		{
			T* t = _add();
			new(t) T(o);
		}

		void push_back(T&& o)
		{
			T* t = _add();
			new(t) T(std::move(o));
		}

		void pop_back()
		{
			back().~T();
			--_size;
		}

		void pop_back(u32 count)
		{
			SV_ASSERT(_size >= count);

			T* end = _data + _size;
			T* it = _data + _size - count;

			while (it != end) {

				it->~T();
				++it;
			}
			
			_size -= count;
		}

		void reserve(size_t size)
		{
			if (_size + size > _capacity) _reallocate(_size + size);
		}

		void resize(size_t size)
		{
			reserve(size);
			if (size > _size) {
				for (size_t i = _size; i < size; ++i) {
					new(_data + i) T();
				}
			}
			_size = size;
		}

		ListIterator<T> erase(const ListIterator<T>& it_)
		{
			T* it = it_.ptr;
			T* end = _data + _size;
			it->~T();

			while (it != end - 1u) {

				*it = std::move(*(it + 1u));
				++it;
			}

			--_size;
			return ListIterator<T>(it);
		}

		T& operator[](size_t index)
		{
			SV_ASSERT(index < _size);
			return _data[index];
		}

		const T& operator[](size_t index) const
		{
			SV_ASSERT(index < _size);
			return _data[index];
		}

		bool empty() const noexcept
		{
			return _size == 0u;
		}
		size_t size() const noexcept
		{
			return _size;
		}
		size_t capacity() const noexcept
		{
			return _capacity;
		}
		T* data() noexcept
		{
			return _data;
		}
		const T* data() const noexcept
		{
			return _data;
		}

		T& back()
		{
			SV_ASSERT(_size != 0u);
			return _data[_size - 1u];
		}
		const T& back() const
		{
			SV_ASSERT(_size != 0u);
			return _data[_size - 1u];
		}
		T& front()
		{
			SV_ASSERT(_size != 0u);
			return *_data;
		}
		const T& front() const
		{
			SV_ASSERT(_size != 0u);
			return *_data;
		}

		void reset()
		{
			foreach(i, _size)
				_data[i].~T();

			_size = 0u;
		}
		void clear()
		{
			foreach(i, _size)
				_data[i].~T();

			if (_data != nullptr) {
				free(_data);
				_data = nullptr;
			}
			_size = 0u;
			_capacity = 0u;
		}

		SV_INLINE void _reallocate(size_t size)
		{
			T* newData = reinterpret_cast<T*>(operator new(size * sizeof(T)));

			if (_data) {
				if (size < _size) {
					T* it = _data + size;
					T* end = _data + _size;
					while (it != end) {
						it->~T();
						++it;
					}
					_size = size;
				}

				T* it0 = _data;
				T* it1 = newData;
				T* end = _data + _size;

				while (it0 != end) {

					new(it1) T(std::move(*it0));

					++it0;
					++it1;
				}

				operator delete(_data);
			}

			_data = newData;
			_capacity = size;
		}

		SV_INLINE T* _add()
		{
			if (_size == _capacity)
				_reallocate(size_t(round(double(_capacity + 1) * 1.7)));
			return _data + _size++;
		}

		SV_INLINE ListIterator<T> begin() noexcept
		{
			return ListIterator<T>(_data);
		}
		SV_INLINE ListConstIterator<T> begin() const noexcept
		{
			return ListConstIterator<T>(_data);
		}

		SV_INLINE ListIterator<T> end() noexcept
		{
			return ListIterator<T>(_data + _size);
		}
		SV_INLINE ListConstIterator<T> end() const noexcept
		{
			return ListConstIterator<T>(_data + _size);
		}

		T* _data = nullptr;
		size_t _size = 0u;
		size_t _capacity = 0u;

	};

	// RAW LIST

	struct RawList {

		RawList() = default;
		~RawList()
		{
			clear();
		}

		SV_INLINE void write_back(const void* src, size_t size)
		{
			if (_size + size > _capacity)
				_reallocate(size_t(round(double(_size + size) * 1.7)));

			memcpy(_data + _size, src, size);
			_size += size;
		}

		SV_INLINE void read(void* dst, size_t size, size_t pos)
		{
			SV_ASSERT(pos + size <= _size);
			memcpy(dst, _data + pos, size);
		}

		SV_INLINE void read_safe(void* dst, size_t size, size_t pos)
		{
			if (pos > _size) {
				memset(dst, 0, size);
			}
			else {
				
				size_t s = _size - pos;
				
				if (s >= size) {

					memcpy(dst, _data + pos, size);
				}
				else {

					memcpy(dst, _data + pos, s);
					memset((u8*)dst + s, 0, size - s);
				}
			}
		}

		SV_INLINE void pop_back(size_t size)
		{
			SV_ASSERT(size >= _size);
			_size -= size;
		}

		SV_INLINE void reserve(size_t size)
		{
			if (_size + size > _capacity) _reallocate(_size + size);
		}

		SV_INLINE void resize(size_t size)
		{
			reserve(size);
			_size = size;
		}

		SV_INLINE bool empty() const noexcept
		{
			return _size == 0u;
		}
		SV_INLINE size_t size() const noexcept
		{
			return _size;
		}
		SV_INLINE size_t capacity() const noexcept
		{
			return _capacity;
		}
		SV_INLINE u8* data() noexcept
		{
			return _data;
		}
		SV_INLINE const u8* data() const noexcept
		{
			return _data;
		}

		SV_INLINE void reset()
		{
			_size = 0u;
		}
		SV_INLINE void clear()
		{
			if (_data != nullptr) {
				free(_data);
				_data = nullptr;
			}
			_size = 0u;
			_capacity = 0u;
		}

		SV_INLINE void _reallocate(size_t size)
		{
			u8* new_data = (u8*)malloc(size);

			if (_data) {

				free(_data);
			}

			_data = new_data;
			_capacity = size;
		}

		u8* _data = nullptr;
		size_t _size = 0u;
		size_t _capacity = 0u;

	};

}
