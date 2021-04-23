#include "utils/allocators.h"

#include "debug/console.h"

namespace sv {

    ////////////////////////////////////// RAW LIST ///////////////////////////////////////////////

    void RawList::write_back(const void* src, size_t size)
    {
	if (_size + size > _capacity)
	    _reallocate(size_t(round(double(_size + size) * 1.7)));

	memcpy(_data + _size, src, size);
	_size += size;
    }

    void RawList::read(void* dst, size_t size, size_t pos)
    {
	SV_ASSERT(pos + size <= _size);
	memcpy(dst, _data + pos, size);
    }

    void RawList::read_and_pop_back(void* dst, size_t size)
    {
	SV_ASSERT(size <= _size);
	memcpy(dst, _data + _size - size, size);
	_size -= size;
    }

    void RawList::read_safe(void* dst, size_t size, size_t pos)
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

    void RawList::pop_back(size_t size)
    {
	SV_ASSERT(size >= _size);
	_size -= size;
    }

    void RawList::reserve(size_t size)
    {
	if (_size + size > _capacity) _reallocate(_size + size);
    }

    void RawList::resize(size_t size)
    {
	reserve(size);
	_size = size;
    }

    void RawList::clear()
    {
	if (_data != nullptr) {
	    SV_FREE_MEMORY(_data);
	    _data = nullptr;
	}
	_size = 0u;
	_capacity = 0u;
    }

    void RawList::_reallocate(size_t size)
    {
	u8* new_data = (u8*)SV_ALLOCATE_MEMORY(size);
		
	if (_data) {

	    memcpy(new_data, _data, _size);
	    SV_FREE_MEMORY(_data);
	}

	_data = new_data;
	_capacity = size;
    }

    ///////////////////////////////////// DO UNDO STACK //////////////////////////////////////////
    
    void DoUndoStack::push_action(ActionFn do_fn, ActionFn undo_fn)
    {
	if (_stack_pos != _stack.size()) {

	    _buffer.resize(_stack[_stack_pos].buffer_offset);
	}
	
	Action action;
	action.do_fn = do_fn;
	action.undo_fn = undo_fn;
	action.buffer_offset = _buffer.size();
	
	_stack.resize(_stack_pos + 1u);
	_stack[_stack_pos] = action;
    }
    
    void DoUndoStack::push_data(const void* data, size_t size)
    {
	SV_ASSERT(_stack.size());
	_buffer.write_back(data, size);
    }

    bool DoUndoStack::do_action(void* return_data)
    {
	bool done = false;
	
	if (_stack_pos < _stack.size()) {

	    Action action = _stack[_stack_pos];
	    if (action.do_fn) {

		void* ptr = _buffer.data() + action.buffer_offset;
		action.do_fn(ptr, return_data);
		done = true;
	    }
	    ++_stack_pos;
	}

	return done;
    }

    bool DoUndoStack::undo_action(void* return_data)
    {
	bool done = false;
	
	if (_stack_pos != 0u) {

	    --_stack_pos;
	    
	    Action action = _stack[_stack_pos];
	    if (action.undo_fn) {

		void* ptr = _buffer.data() + action.buffer_offset;
		action.undo_fn(ptr, return_data);
		done = true;
	    }
	}
	
	return done;
    }

    void DoUndoStack::clear()
    {
	_stack.clear();
	_buffer.clear();
	_stack_pos = 0u;
    }

    /////////////////////////////////// INSTANCE ALLOCATOR ////////////////////////////////////////////

    SizedInstanceAllocatorPool::SizedInstanceAllocatorPool(size_t instanceSize) : INSTANCE_SIZE(instanceSize) {}
    SizedInstanceAllocatorPool::SizedInstanceAllocatorPool(SizedInstanceAllocatorPool&& other)
	: INSTANCE_SIZE(other.INSTANCE_SIZE)
    {
	instances = other.instances;
	size = other.size;
	beginCount = other.beginCount;
	INSTANCE_SIZE = other.INSTANCE_SIZE;
	other.instances = nullptr;
	other.size = 0u;
	other.beginCount = 0u;
	other.INSTANCE_SIZE = 0u;
    }
    SizedInstanceAllocatorPool& SizedInstanceAllocatorPool::operator=(SizedInstanceAllocatorPool&& other) noexcept
    {
	instances = other.instances;
	size = other.size;
	beginCount = other.beginCount;
	INSTANCE_SIZE = other.INSTANCE_SIZE;
	other.instances = nullptr;
	other.size = 0u;
	other.beginCount = 0u;
	other.INSTANCE_SIZE = 0u;
	return *this;
    }

    u32 SizedInstanceAllocatorPool::unfreed_count() const noexcept
    {
	u32 unfreed = 0u;
	for (auto it = begin(); it != end(); ++it) {
	    ++unfreed;
	}
	return unfreed;
    }

    SizedInstanceAllocatorPool::iterator SizedInstanceAllocatorPool::begin() const
    {
	if (beginCount == u32_max) return iterator(instances, (u8*)& beginCount, INSTANCE_SIZE);
	return iterator(instances, instances + (beginCount * INSTANCE_SIZE), INSTANCE_SIZE);
    }

    SizedInstanceAllocatorPool::iterator SizedInstanceAllocatorPool::end() const
    {
	return iterator(instances + (size * INSTANCE_SIZE), nullptr, INSTANCE_SIZE);
    }

    SizedInstanceAllocator::SizedInstanceAllocator()
	: INSTANCE_SIZE(0u), POOL_SIZE(0u)
    {
    }

    SizedInstanceAllocator::SizedInstanceAllocator(size_t instanceSize, size_t poolSize)
	: INSTANCE_SIZE(instanceSize), POOL_SIZE(poolSize)
    {
	SV_ASSERT(INSTANCE_SIZE >= sizeof(u32) && "The size must be greater than 4 bytes");
    }
    SizedInstanceAllocator::SizedInstanceAllocator(SizedInstanceAllocator&& other) noexcept 
	: INSTANCE_SIZE(other.INSTANCE_SIZE), POOL_SIZE(other.POOL_SIZE)
    {
	m_Pools = std::move(other.m_Pools);
    }
    SizedInstanceAllocator::~SizedInstanceAllocator()
    {
	clear();
    }

    void* SizedInstanceAllocator::alloc()
    {
	// Validate back pool
	if (m_Pools.empty()) m_Pools.emplace_back(INSTANCE_SIZE);
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
	    m_Pools.emplace_back(INSTANCE_SIZE);

	found:
	    Pool aux = std::move(m_Pools[poolIndex]);
	    m_Pools[poolIndex] = std::move(m_Pools.back());
	    m_Pools.back() = std::move(aux);

	}

	Pool& pool = m_Pools.back();

	// Allocate
	if (pool.instances == nullptr) {
	    pool.instances = new u8[INSTANCE_SIZE * POOL_SIZE];
	}

	// Get ptr
	u8* ptr;

	if (pool.beginCount != u32_max) {
	    ptr = pool.instances + (pool.beginCount * INSTANCE_SIZE);

	    // Change nextCount
	    u32* freeInstance = reinterpret_cast<u32*>(ptr);

	    if (*freeInstance == u32_max)
		pool.beginCount = u32_max;
	    else
		pool.beginCount += *freeInstance;
	}
	else {
	    ptr = pool.instances + size_t(pool.size * INSTANCE_SIZE);
	    ++pool.size;
	}

	return ptr;
    }

    void SizedInstanceAllocator::free(void* ptr_)
    {
	u8* ptr = reinterpret_cast<u8*>(ptr_);

	// Find pool
	Pool* pool = nullptr;

	for (Pool& p : m_Pools) {
	    if (p.instances <= ptr && p.instances + (p.size * INSTANCE_SIZE) > ptr) {
		pool = &p;
		break;
	    }
	}

	if (pool == nullptr || pool->size == 0u) {
	    return;
	}

	// Remove from list
	if (ptr == pool->instances + size_t((pool->size - 1u) * INSTANCE_SIZE)) {
	    --pool->size;
	}
	else {
	    // Update next count
	    u32* nextCount = findLastNextCount(ptr, *pool);

	    u32 distance;

	    if (nextCount == &pool->beginCount) {
		distance = u32((ptr - pool->instances) / INSTANCE_SIZE);
	    }
	    else {
		if ((void*)nextCount >= ptr)
		    system("PAUSE");

		distance = u32((ptr - reinterpret_cast<u8*>(nextCount)) / INSTANCE_SIZE);
	    }

	    u32* freeInstance = reinterpret_cast<u32*>(ptr);

	    if (*nextCount == u32_max)
		* freeInstance = u32_max;
	    else
		*freeInstance = *nextCount - distance;

	    *nextCount = distance;
	}
    }

    void SizedInstanceAllocator::clear()
    {
	for (Pool& pool : m_Pools) {
	    delete pool.instances;
	}
	m_Pools.clear();
    }

    u32 SizedInstanceAllocator::unfreed_count() const noexcept
    {
	u32 unfreed = 0u;
	for (auto it = m_Pools.cbegin(); it != m_Pools.cend(); ++it) {
	    unfreed += it->unfreed_count();
	}
	return unfreed;
    }

    u32* SizedInstanceAllocator::findLastNextCount(void* ptr, Pool& pool)
    {
	u32* nextCount;

	if (pool.beginCount == u32_max) return &pool.beginCount;

	u8* next = pool.instances + (pool.beginCount * INSTANCE_SIZE);
	u8* end = pool.instances + (pool.size * INSTANCE_SIZE);

	if (next >= ptr) nextCount = &pool.beginCount;
	else {
	    u8* last = next;

	    while (next != end) {

		u32 nextCount = *reinterpret_cast<u32*>(last);
		if (nextCount == u32_max) break;

		next += size_t(nextCount * INSTANCE_SIZE);
		if (next > ptr) break;

		last = next;
	    }
	    nextCount = reinterpret_cast<u32*>(last);
	}

	return nextCount;
    }
}
