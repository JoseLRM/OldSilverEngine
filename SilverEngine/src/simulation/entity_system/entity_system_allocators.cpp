#include "core.h"

#include "entity_system_internal.h"

namespace sv {

	// EntityDataAllocator

	Entity ecs_allocator_entity_alloc(EntityDataAllocator& a)
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
		pool.data = new ui8[size_t(compSize) * ECS_COMPONENT_POOL_SIZE];
	}

	void ecs_allocator_component_pool_free(ComponentPool& pool)
	{
		if (pool.data != nullptr) {
			delete[] pool.data;
			pool.data = nullptr;
		}
		pool.size = 0u;
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
		return (pool.size / pool.compSize) == ECS_COMPONENT_POOL_SIZE;
	}

	bool ecs_allocator_component_pool_exist(const ComponentPool& pool, void* ptr)
	{
		return ptr >= pool.data && ptr < (pool.data + pool.size);
	}

	ui32 ecs_allocator_component_pool_count(const ComponentPool& pool)
	{
		return ui32(pool.size / size_t(pool.compSize) - pool.freeList.size());
	}

	// ComponentAllocator

	ComponentPool& ecs_allocator_component_create_pool(ECS* ecs, ComponentAllocator& a)
	{
		ComponentPool& pool = a.pools.emplace_back();
		ecs_allocator_component_pool_alloc(pool, ecs_component_size(a.compID));
		return pool;
	}

	ComponentPool& ecs_allocator_component_prepare_pool(ECS* ecs, ComponentAllocator& a)
	{
		if (ecs_allocator_component_pool_is_filled(a.pools.back())) {
			return ecs_allocator_component_create_pool(ecs, a);
		}
		return a.pools.back();
	}

	void ecs_allocator_component_create(ECS* ecs, ComponentAllocator& a, CompID ID)
	{
		ecs_allocator_component_destroy(ecs, a);
		a.compID = ID;
		ecs_allocator_component_create_pool(ecs, a);
	}

	void ecs_allocator_component_destroy(ECS* ecs, ComponentAllocator& a)
	{
		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			ui8* ptr = it->data;
			ui8* endPtr = it->data + it->size;

			while (ptr != endPtr) {

				BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
				if (comp->entity != SV_ENTITY_NULL) {
					ecs_register_destroy(ecs, a.compID, comp);
				}

				ptr += it->compSize;
			}

			ecs_allocator_component_pool_free(*it);

		}
		a.pools.clear();
	}

	BaseComponent* ecs_allocator_component_alloc(ECS* ecs, ComponentAllocator& a, Entity entity, bool create)
	{
		ComponentPool& pool = ecs_allocator_component_prepare_pool(ecs, a);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ecs_allocator_component_pool_add(pool));

		if (create) ecs_register_create(ecs, a.compID, comp, entity);

		return comp;
	}

	BaseComponent* ecs_allocator_component_alloc(ECS* ecs, ComponentAllocator& a, BaseComponent* srcComp)
	{
		ComponentPool& pool = ecs_allocator_component_prepare_pool(ecs, a);
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ecs_allocator_component_pool_add(pool));

		ecs_register_copy(ecs, a.compID, srcComp, comp);

		return comp;
	}

	void ecs_allocator_component_free(ECS* ecs, ComponentAllocator& a, BaseComponent* comp)
	{
		SV_ASSERT(comp != nullptr);

		for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

			if (ecs_allocator_component_pool_exist(*it, comp)) {

				ecs_register_destroy(ecs, a.compID, comp);
				ecs_allocator_component_pool_remove(*it, comp);

			}
		}
	}

	ui32 ecs_allocator_component_count(ECS* ecs, const ComponentAllocator& a)
	{
		ui32 compSize = ecs_component_size(a.compID);
		ui32 res = 0u;
		for (const ComponentPool& pool : a.pools) {
			res += ecs_allocator_component_pool_count(pool);
		}
		return res;
	}

	ui32 ecs_allocator_component_empty(ECS* ecs, const ComponentAllocator& a)
	{
		ui32 compSize = ecs_component_size(a.compID);
		for (const ComponentPool& pool : a.pools) {
			if (ecs_allocator_component_pool_count(pool) > 0u) return false;
		}
		return true;
	}
	
}