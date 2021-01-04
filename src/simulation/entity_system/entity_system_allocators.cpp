#include "core.h"

#include "entity_system_internal.h"

namespace sv {

	// EntityDataAllocator

	Entity entityAlloc(EntityDataAllocator& a)
	{
		if (a.freeList.empty()) {

			if (a.size == a.capacity) {
				EntityData* newData = new EntityData[a.capacity + ECS_ENTITY_ALLOC_SIZE];
				EntityTransform* newTransformData = new EntityTransform[a.capacity + ECS_ENTITY_ALLOC_SIZE];

				if (a.data) {

					{
						EntityData* end = a.data + a.capacity;
						while (a.data != end) {

							*newData = std::move(*a.data);

							++newData;
							++a.data;
						}
					}
					{
						memcpy(newTransformData, a.transformData, a.capacity * sizeof(EntityTransform));
					}

					a.data -= a.capacity;
					newData -= a.capacity;
					delete[] a.data;
					delete[] a.transformData;
				}

				a.capacity += ECS_ENTITY_ALLOC_SIZE;
				a.data = newData;
				a.transformData = newTransformData;
				a.accessData = a.data - 1u;
				a.accessTransformData = a.transformData - 1u;
			}

			return ++a.size;

		}
		else {
			Entity result = a.freeList.back();
			a.freeList.pop_back();
			return result;
		}
	}

	void entityFree(EntityDataAllocator& a, Entity entity)
	{
		SV_ASSERT(a.size >= entity);
		a.accessData[entity] = EntityData();
		a.accessTransformData[entity] = EntityTransform();

		if (entity == a.size) {
			a.size--;
		}
		else {
			a.freeList.push_back(entity);
		}
	}

	void entityClear(EntityDataAllocator& a)
	{
		if (a.data) {
			a.capacity = 0u;
			delete[] a.data;
			a.data = nullptr;
			delete[] a.transformData;
			a.transformData = nullptr;
		}

		a.accessData = nullptr;
		a.accessTransformData = nullptr;
		a.size = 0u;
		a.freeList.clear();
	}

	// ComponentPool

	void componentPoolAlloc(ComponentPool& pool, size_t compSize)
	{
		componentPoolFree(pool);
		pool.data = new u8[compSize * ECS_COMPONENT_POOL_SIZE];
		pool.freeCount = 0u;
	}

	void componentPoolFree(ComponentPool& pool)
	{
		if (pool.data != nullptr) {
			delete[] pool.data;
			pool.data = nullptr;
		}
		pool.size = 0u;
	}

	void* componentPoolGetPtr(ComponentPool& pool, size_t compSize)
	{
		u8* ptr = nullptr;

		if (pool.freeCount) {
			
			u8* it = pool.data;
			u8* end = it + pool.size;

			while (it != end) {
				
				if (reinterpret_cast<BaseComponent*>(it)->entity == SV_ENTITY_NULL) {
					ptr = it;
					break;
				}
				it += compSize;
			}
			SV_ASSERT(ptr != nullptr && "Free component not found!!");
			--pool.freeCount;
		}
		else {
			ptr = pool.data + pool.size;
			pool.size += compSize;
		}

		return ptr;
	}

	void componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr)
	{
		if (ptr == pool.data + pool.size) {
			pool.size -= compSize;
		}
		else {
			++pool.freeCount;
		}
	}

	bool componentPoolFull(const ComponentPool& pool, size_t compSize)
	{
		return (pool.size == ECS_COMPONENT_POOL_SIZE * compSize) && pool.freeCount == 0u;
	}

	bool componentPoolPtrExist(const ComponentPool& pool, void* ptr)
	{
		return ptr >= pool.data && ptr < (pool.data + pool.size);
	}

	u32 componentPoolCount(const ComponentPool& pool, size_t compSize)
	{
		return u32(pool.size / compSize - pool.freeCount);
	}

	// ComponentAllocator

	SV_INLINE static ComponentPool& componentAllocatorCreatePool(ComponentAllocator& a, size_t compSize)
	{
		ComponentPool& pool = a.pools.emplace_back();
		componentPoolAlloc(pool, compSize);
		return pool;
	}

	SV_INLINE static ComponentPool& componentAllocatorPreparePool(ComponentAllocator& a, size_t compSize)
	{
		if (componentPoolFull(a.pools.back(), compSize)) {
			return componentAllocatorCreatePool(a, compSize);
		}
		return a.pools.back();
	}

	void componentAllocatorCreate(ECS* ecs_, CompID compID)
	{
		parseECS();
		componentAllocatorDestroy(ecs_, compID);
		componentAllocatorCreatePool(ecs.components[compID], size_t(ecs_component_size(compID)));
	}

	void componentAllocatorDestroy(ECS* ecs_, CompID compID)
	{
		parseECS();

		ComponentAllocator& a = ecs.components[compID];
		size_t compSize = size_t(ecs_component_size(compID));

		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			u8* ptr = it->data;
			u8* endPtr = it->data + it->size;

			while (ptr != endPtr) {

				BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
				if (comp->entity != SV_ENTITY_NULL) {
					ecs_register_destroy(ecs_, compID, comp);
				}

				ptr += compSize;
			}

			componentPoolFree(*it);

		}
		a.pools.clear();
	}

	BaseComponent* componentAlloc(ECS* ecs_, CompID compID, Entity entity, bool create)
	{
		parseECS();

		ComponentAllocator& a = ecs.components[compID];
		size_t compSize = size_t(ecs_component_size(compID));

		ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

		if (create) ecs_register_create(ecs_, compID, comp, entity);

		return comp;
	}

	BaseComponent* componentAlloc(ECS* ecs_, CompID compID, BaseComponent* srcComp)
	{
		parseECS();

		ComponentAllocator& a = ecs.components[compID];
		size_t compSize = size_t(ecs_component_size(compID));

		ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

		ecs_register_copy(ecs_, compID, srcComp, comp);

		return comp;
	}

	void componentFree(ECS* ecs_, CompID compID, BaseComponent* comp)
	{
		parseECS();
		SV_ASSERT(comp != nullptr);

		ComponentAllocator& a = ecs.components[compID];
		size_t compSize = size_t(ecs_component_size(compID));

		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			if (componentPoolPtrExist(*it, comp)) {

				ecs_register_destroy(ecs_, compID, comp);
				componentPoolRmvPtr(*it, compSize, comp);

				break;
			}
		}
	}

	u32 componentAllocatorCount(ECS* ecs_, CompID compID)
	{
		parseECS();

		const ComponentAllocator& a = ecs.components[compID];
		u32 compSize = ecs_component_size(compID);

		u32 res = 0u;
		for (const ComponentPool& pool : a.pools) {
			res += componentPoolCount(pool, compSize);
		}
		return res;
	}

	bool componentAllocatorIsEmpty(ECS* ecs_, CompID compID)
	{
		parseECS();

		const ComponentAllocator& a = ecs.components[compID];
		u32 compSize = ecs_component_size(compID);

		for (const ComponentPool& pool : a.pools) {
			if (componentPoolCount(pool, compSize) > 0u) return false;
		}
		return true;
	}

}