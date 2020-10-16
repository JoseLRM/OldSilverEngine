#include "core.h"

#include "utils/allocator.h"

namespace sv {

	SizedInstanceAllocator::SizedInstanceAllocator(size_t instanceSize, size_t poolSize) : m_PoolSize(poolSize), INSTANCE_SIZE(std::max(instanceSize, sizeof(void*)))
	{
		m_Pools.emplace_back();
	}
	SizedInstanceAllocator::~SizedInstanceAllocator()
	{
		clear();
	}

	void* SizedInstanceAllocator::alloc()
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);

		{
			Pool& pool = m_Pools.back();

			if (pool.freeList) {
				void* ptr = pool.freeList;
				pool.freeList = *reinterpret_cast<void**>(pool.freeList);
				return ptr;
			}

			// Find new pool
			if (pool.size == m_PoolSize) {

				Pool* found = nullptr;
				auto end = m_Pools.end() - 1u;

				// Find pool with free list
				for (auto it = m_Pools.begin(); it != end; ++it) {
					if (it->freeList) {
						found = &(*it);
						break;
					}
				}

				// Find pool with free size
				if (found == nullptr) {
					for (auto it = m_Pools.begin(); it != end; ++it) {
						if (it->size < m_PoolSize) {
							found = &(*it);
							break;
						}
					}
				}

				// Add new empty pool
				if (found == nullptr)
					m_Pools.emplace_back();
				else {
					// Set pool back to the list
					Pool aux = *found;
					*found = m_Pools.back();
					m_Pools.back() = aux;
				}
			}
		}
		Pool& pool = m_Pools.back();

		if (pool.data == nullptr) {
			pool.data = new ui8[m_PoolSize * INSTANCE_SIZE];
			pool.freeList = nullptr;
			pool.size = 0u;
		}

		ui8* res = (ui8*)(pool.data) + (pool.size * INSTANCE_SIZE);
		++pool.size;
		return res;
	}

	void SizedInstanceAllocator::free(void* ptr)
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);

		Pool* pool = nullptr;
		for (auto it = m_Pools.rbegin(); it != m_Pools.rend(); ++it) {
			if (it->data <= ptr && it->data + (it->size * INSTANCE_SIZE) > ptr) {
				pool = &(*it);
				break;
			}
		}

		if (pool == nullptr) return;

		if (ptr == pool->data + ((pool->size - 1u) * INSTANCE_SIZE)) --pool->size;
		else {
			void** newFreeList = reinterpret_cast<void**>(ptr);
			*newFreeList = pool->freeList;
			pool->freeList = ptr;

			// Set pool back to the list
			Pool aux = *pool;
			*pool = m_Pools.back();
			m_Pools.back() = aux;
		}
	}

	void SizedInstanceAllocator::clear()
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);

		// Free memory
		for (auto it = m_Pools.begin(); it != m_Pools.end(); ++it) {
			Pool& pool = *it;
			if (pool.data) {
				delete[] pool.data;
			}
		}

		m_Pools.clear();
	}

	bool SizedInstanceAllocator::has_unfreed() noexcept
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);

		for (auto it = m_Pools.cbegin(); it != m_Pools.cend(); ++it) {
			void* freeList = it->freeList;
			ui32 count = 0u;
			while (freeList) {

				++count;
				freeList = *reinterpret_cast<void**>(freeList);

			}

			if (count != it->size) {
				return true;
			}
		}

		return false;
	}

	ui32 SizedInstanceAllocator::unfreed_count() noexcept
	{
		std::scoped_lock<std::mutex> lock(m_Mutex);

		ui32 freeCount = 0u;
		ui32 totalSize = 0u;

		for (auto it = m_Pools.cbegin(); it != m_Pools.cend(); ++it) {
			void* freeList = it->freeList;
			
			while (freeList) {

				++freeCount;
				freeList = *reinterpret_cast<void**>(freeList);

			}

			totalSize += it->size;
		}

		return totalSize - freeCount;
	}

}