#include "core.h"

#include "utils/allocator.h"

namespace sv {

	SizedInstanceAllocatorPool::SizedInstanceAllocatorPool(size_t instanceSize) : INSTANCE_SIZE(instanceSize) {}
	SizedInstanceAllocatorPool::SizedInstanceAllocatorPool(const SizedInstanceAllocatorPool& other)
		: INSTANCE_SIZE(other.INSTANCE_SIZE)
	{
		instances = other.instances;
		size = other.size;
		beginCount = other.beginCount;
	}
	SizedInstanceAllocatorPool& SizedInstanceAllocatorPool::operator=(const SizedInstanceAllocatorPool& other)
	{
		instances = other.instances;
		size = other.size;
		beginCount = other.beginCount;
		return *this;
	}

	ui32 SizedInstanceAllocatorPool::unfreed_count() const noexcept
	{
		ui32 unfreed = 0u;
		for (auto it = begin(); it != end(); ++it) {
			++unfreed;
		}
		return unfreed;
	}

	SizedInstanceAllocatorPool::iterator SizedInstanceAllocatorPool::begin() const
	{
		if (beginCount == ui32_max) return iterator(instances, (ui8*)& beginCount, INSTANCE_SIZE);
		return iterator(instances, instances + (beginCount * INSTANCE_SIZE), INSTANCE_SIZE);
	}

	SizedInstanceAllocatorPool::iterator SizedInstanceAllocatorPool::end() const
	{
		return iterator(instances + (size * INSTANCE_SIZE), nullptr, INSTANCE_SIZE);
	}

	SizedInstanceAllocator::SizedInstanceAllocator(size_t instanceSize, size_t poolSize)
		: INSTANCE_SIZE(instanceSize), POOL_SIZE(poolSize)
	{
		SV_ASSERT(INSTANCE_SIZE >= sizeof(ui32) && "The size must be greater than 4u");
	}
	SizedInstanceAllocator::~SizedInstanceAllocator()
	{
		clear();
	}

	void* SizedInstanceAllocator::alloc()
	{
		// Validate back pool
		if (m_Pools.empty()) m_Pools.emplace_back(INSTANCE_SIZE);
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
			m_Pools.emplace_back(INSTANCE_SIZE);

		found:
			Pool aux = m_Pools[poolIndex];
			m_Pools[poolIndex] = m_Pools.back();
			m_Pools.back() = aux;

		}

		Pool& pool = m_Pools.back();

		// Allocate
		if (pool.instances == nullptr) {
			pool.instances = new ui8[INSTANCE_SIZE * POOL_SIZE];
		}

		// Get ptr
		ui8* ptr;

		if (pool.beginCount != ui32_max) {
			ptr = pool.instances + (pool.beginCount * INSTANCE_SIZE);

			// Change nextCount
			ui32* freeInstance = reinterpret_cast<ui32*>(ptr);

			if (*freeInstance == ui32_max)
				pool.beginCount = ui32_max;
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
		ui8* ptr = reinterpret_cast<ui8*>(ptr_);

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
			ui32* nextCount = findLastNextCount(ptr, *pool);

			ui32 distance;

			if (nextCount == &pool->beginCount) {
				distance = ui32((ptr - pool->instances) / INSTANCE_SIZE);
			}
			else {
				if ((void*)nextCount >= ptr)
					system("PAUSE");

				distance = ui32((ptr - reinterpret_cast<ui8*>(nextCount)) / INSTANCE_SIZE);
			}

			ui32* freeInstance = reinterpret_cast<ui32*>(ptr);

			if (*nextCount == ui32_max)
				* freeInstance = ui32_max;
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

	ui32 SizedInstanceAllocator::unfreed_count() const noexcept
	{
		ui32 unfreed = 0u;
		for (auto it = m_Pools.cbegin(); it != m_Pools.cend(); ++it) {
			unfreed += it->unfreed_count();
		}
		return unfreed;
	}

	ui32* SizedInstanceAllocator::findLastNextCount(void* ptr, Pool& pool)
	{
		ui32* nextCount;

		if (pool.beginCount == ui32_max) return &pool.beginCount;

		ui8* next = pool.instances + (pool.beginCount * INSTANCE_SIZE);
		ui8* end = pool.instances + (pool.size * INSTANCE_SIZE);

		if (next >= ptr) nextCount = &pool.beginCount;
		else {
			ui8* last = next;

			while (next != end) {

				ui32 nextCount = *reinterpret_cast<ui32*>(last);
				if (nextCount == ui32_max) break;

				next += size_t(nextCount * INSTANCE_SIZE);
				if (next > ptr) break;

				last = next;
			}
			nextCount = reinterpret_cast<ui32*>(last);
		}

		return nextCount;
	}
}