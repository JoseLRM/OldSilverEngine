#pragma once

#include "defines.h"
#include "debug/console.h"

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


    ////////////////////////////////////////////////////// LIST //////////////////////////////////////////////////
    
    template<typename T>
	struct SV_API ListIterator {
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
	struct SV_API ListConstIterator {
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
	struct SV_API List {

	    List() = default;
	    ~List()
		{
		    clear();
		}

	    List(const List& other)
		{
		    if (other._data) {

			_data = (T*)SV_ALLOCATE_MEMORY(sizeof(T) * other._size);
				
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

		    _data = (T*)SV_ALLOCATE_MEMORY(sizeof(T) * other._size);

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

	    void erase(size_t index)
		{
		    SV_ASSERT(index < _size);
		    T* it = _data + index;
		    T* end = _data + _size;
		    it->~T();

		    while (it != end - 1u) {

			*it = std::move(*(it + 1u));
			++it;
		    }

		    --_size;
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
			SV_FREE_MEMORY(_data);
			_data = nullptr;
		    }
		    _size = 0u;
		    _capacity = 0u;
		}

	    SV_INLINE void _reallocate(size_t size)
		{
		    T* newData = reinterpret_cast<T*>(SV_ALLOCATE_MEMORY(size * sizeof(T)));

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

			SV_FREE_MEMORY(_data);
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

    struct SV_API RawList {

	RawList() = default;
	~RawList()
	    {
		clear();
	    }

	void write_back(const void* src, size_t size);
	
	void read(void* dst, size_t size, size_t pos);
	void read_and_pop_back(void* dst, size_t size);
	void read_safe(void* dst, size_t size, size_t pos);

	void pop_back(size_t size);
	
	void reserve(size_t size);
	void resize(size_t size);

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
	
	void clear();
	
	void _reallocate(size_t size);

	u8* _data = nullptr;
	size_t _size = 0u;
	size_t _capacity = 0u;

    };
    
    //////////////////////////////////// DO UNDO STACK /////////////////////////////////////////////////
    
    typedef void(*ActionFn)(void* data, void* return_data);
    
    struct SV_API DoUndoStack {
	
	void push_action(ActionFn do_fn, ActionFn undo_fn);
	void push_data(const void* data, size_t size);
	
	// Execute the last do function with the last pushed data
	bool do_action(void* return_data);

	// Execute the last undo function with the last pushed data. To redo the action simply call doAction()
	bool undo_action(void* return_data);

	void clear();

	SV_INLINE void lock() {} // TODO
	SV_INLINE void unlock() {} // TODO
	
	struct Action {
	    ActionFn do_fn = nullptr;
	    ActionFn undo_fn = nullptr;
	    size_t buffer_offset = 0u;
	};
	
	List<Action> _stack;
	u32          _stack_pos = 0u;
	RawList      _buffer;

    };
    
}
