#include "core.h"

#include "entity_system_internal.h"

namespace sv {

	// EntityDataAllocator

	Entity ecs_allocator_entity_alloc(EntityDataAllocator& a)
	{
		if (a.freeList.empty()) {

			if (a.size == a.capacity) {
				EntityData* newData = new EntityData[a.capacity + SV_ECS_ENTITY_ALLOC_SIZE];
				EntityTransform* newTransformData = new EntityTransform[a.capacity + SV_ECS_ENTITY_ALLOC_SIZE];

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
						EntityTransform* end = a.transformData + a.capacity;
						while (a.transformData != end) {

							*newTransformData = std::move(*a.transformData);

							++newTransformData;
							++a.transformData;
						}
					}

					a.data -= a.capacity;
					a.transformData -= a.capacity;
					newData -= a.capacity;
					newTransformData -= a.capacity;
					delete[] a.data;
					delete[] a.transformData;
				}

				a.capacity += SV_ECS_ENTITY_ALLOC_SIZE;
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

	void ecs_allocator_entity_free(EntityDataAllocator& a, Entity entity)
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

	void ecs_allocator_entity_clear(EntityDataAllocator& a)
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

	void ecs_allocator_component_pool_alloc(ComponentPool& pool, ui32 compSize)
	{
		ecs_allocator_component_pool_free(pool);
		pool.compSize = compSize;
		pool.data = new ui8[size_t(compSize) * SV_ECS_COMPONENT_POOL_SIZE];
	}

	void ecs_allocator_component_pool_free(ComponentPool& pool)
	{
		if (pool.data != nullptr) {
			delete[] pool.data;
			pool.data = nullptr;
		}
		pool.size;
		pool.freeList.clear();
	}

	void* ecs_allocator_component_pool_add(ComponentPool& pool)
	{
		ui8* ptr;

		if (pool.freeList.empty()) {
			ptr = pool.data + pool.size;
			pool.size += pool.compSize;
		}
		else {
			ptr = pool.freeList.back();
			pool.freeList.pop_back();
		}

		return ptr;
	}

	void ecs_allocator_component_pool_remove(ComponentPool& pool, void* ptr)
	{
		if (ptr == pool.data + pool.size) {
			pool.size -= pool.compSize;
		}
		else {
			pool.freeList.push_back(reinterpret_cast<ui8*>(ptr));
		}
	}

	bool ecs_allocator_component_pool_is_filled(const ComponentPool& pool)
	{
		return (pool.size / pool.compSize) == SV_ECS_COMPONENT_POOL_SIZE;
	}

	bool ecs_allocator_component_pool_exist(const ComponentPool& pool, void* ptr)
	{
		return ptr >= pool.data && ptr < (pool.data + pool.size);
	}

	ui32 ecs_allocator_component_pool_count(const ComponentPool& pool)
	{
		return pool.size / size_t(pool.compSize) - pool.freeList.size();
	}

	// ComponentAllocator

	ComponentPool& ecs_allocator_component_create_pool(ComponentAllocator& a)
	{
		ComponentPool& pool = a.pools.emplace_back();
		ecs_allocator_component_pool_alloc(pool, ecs_register_sizeof(a.compID));
		return pool;
	}

	ComponentPool& ecs_allocator_component_prepare_pool(ComponentAllocator& a)
	{
		if (ecs_allocator_component_pool_is_filled(a.pools.back())) {
			return ecs_allocator_component_create_pool(a);
		}
		return a.pools.back();
	}

	void ecs_allocator_component_create(ComponentAllocator& a, CompID ID)
	{
		ecs_allocator_component_destroy(a);
		a.compID = ID;
		ecs_allocator_component_create_pool(a);
	}

	void ecs_allocator_component_destroy(ComponentAllocator& a)
	{
		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			ecs_allocator_component_pool_free(*it);

		}
		a.pools.clear();
	}

	BaseComponent* ecs_allocator_component_alloc(ComponentAllocator& a, Entity entity)
	{
		ComponentPool& pool = ecs_allocator_component_prepare_pool(a);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ecs_allocator_component_pool_add(pool));

		ecs_register_create(a.compID, comp, entity);

		return comp;
	}

	BaseComponent* ecs_allocator_component_alloc(ComponentAllocator& a, BaseComponent* srcComp)
	{
		ComponentPool& pool = ecs_allocator_component_prepare_pool(a);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ecs_allocator_component_pool_add(pool));

		ecs_register_copy(a.compID, srcComp, comp);

		return comp;
	}

	void ecs_allocator_component_free(ComponentAllocator& a, BaseComponent* comp)
	{
		SV_ASSERT(comp != nullptr);

		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			if (ecs_allocator_component_pool_exist(*it, comp)) {

				ecs_register_destroy(a.compID, comp);
				ecs_allocator_component_pool_remove(*it, comp);

			}
		}
	}

	ui32 ecs_allocator_component_count(const ComponentAllocator& a)
	{
		ui32 compSize = ecs_register_sizeof(a.compID);
		ui32 res = 0u;
		for (const ComponentPool& pool : a.pools) {
			res += ecs_allocator_component_pool_count(pool);
		}
		return res;
	}

	ui32 ecs_allocator_component_empty(const ComponentAllocator& a)
	{
		ui32 compSize = ecs_register_sizeof(a.compID);
		for (const ComponentPool& pool : a.pools) {
			if (ecs_allocator_component_pool_count(pool) > 0u) return false;
		}
		return true;
	}
	
}