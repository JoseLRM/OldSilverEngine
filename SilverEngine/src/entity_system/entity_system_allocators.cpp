#include "core.h"

#include "entity_system_internal.h"

namespace sv {

	// EntityDataAllocator

	EntityDataAllocator::EntityDataAllocator() : m_Size(1u), m_Capacity(SV_ECS_ENTITY_ALLOC_SIZE)
	{
		m_Data = new EntityData[SV_ECS_ENTITY_ALLOC_SIZE];
	}

	Entity EntityDataAllocator::add()
	{
		if (m_FreeList.empty()) {

			if (m_Size == m_Capacity) {
				EntityData* newData = new EntityData[m_Capacity + SV_ECS_ENTITY_ALLOC_SIZE];

				EntityData* end = m_Data + m_Capacity;
				while (m_Data != end) {

					*newData = std::move(*m_Data);

					newData++;
					m_Data++;
				}
				m_Data -= m_Capacity;
				newData -= m_Capacity;

				delete[] m_Data;
				m_Capacity += SV_ECS_ENTITY_ALLOC_SIZE;
				m_Data = newData;
			}
		
			return m_Size++;

		}
		else {
			Entity result = m_FreeList.back();
			m_FreeList.pop_back();
			return result;
		}
	}

	void EntityDataAllocator::remove(Entity entity)
	{
		m_Data[entity] = EntityData();
		
		if (entity + 1u == m_Size) {
			m_Size--;
		}
		else {
			m_FreeList.push_back(entity);
		}
	}

	void EntityDataAllocator::clear()
	{
		if (m_Capacity > SV_ECS_ENTITY_ALLOC_SIZE) {

			m_Capacity = SV_ECS_ENTITY_ALLOC_SIZE;
			delete[] m_Data;
			m_Data = new EntityData[SV_ECS_ENTITY_ALLOC_SIZE];
			
		}
		else {

			EntityData* end = m_Data + m_Size;
			while (m_Data != end) {
				*m_Data = EntityData();
				m_Data++;
			}

			m_Data -= m_Size;

		}

		m_Size = 1u;
		m_FreeList.clear();
	}

	// ComponentPool

	ComponentPool::ComponentPool() : m_Data(nullptr), m_Size(0) {}
	ComponentPool::~ComponentPool()
	{
		free();
	}

	ComponentPool::ComponentPool(ComponentPool&& other) noexcept
	{
		m_Data = other.m_Data;
		m_Size = other.m_Size;
		other.m_Data = nullptr;
		other.m_Size = 0u;
	}

	void ComponentPool::allocate(ui32 compSize)
	{
		free();
		m_Data = new ui8[size_t(compSize) * SV_ECS_COMPONENT_POOL_SIZE];
	}

	void ComponentPool::free()
	{
		if (m_Data != nullptr) {
			delete[] m_Data;
			m_Data = nullptr;
			m_Size = 0u;
			m_FreeList.clear();
		}
	}

	void* ComponentPool::add(ui32 compSize) noexcept
	{
		ui8* ptr;

		if (m_FreeList.empty()) {
			ptr = m_Data + m_Size;
			m_Size += compSize;
		}
		else {
			ptr = m_FreeList.back();
			m_FreeList.pop_back();
		}

		return ptr;
	}

	void ComponentPool::remove(ui32 compSize, void* ptr)
	{
		if (ptr == m_Data + m_Size) {
			m_Size -= compSize;
		}
		else {
			m_FreeList.push_back(reinterpret_cast<ui8*>(ptr));
		}
	}

	bool ComponentPool::is_filled(ui32 compSize) const noexcept
	{
		return (m_Size / compSize) == SV_ECS_COMPONENT_POOL_SIZE;
	}

	bool ComponentPool::exist(void* ptr) const noexcept
	{
		return ptr > m_Data && ptr < (m_Data + m_Size);
	}

	ui32 ComponentPool::size(ui32 compSize) const noexcept
	{
		return m_Size / size_t(compSize) - m_FreeList.size();
	}

	// ComponentAllocator

	void ComponentAllocator::create(CompID compID)
	{
		destroy();
		m_CompID = compID;
		create_pool();
	}

	void ComponentAllocator::destroy()
	{
		m_Pools.clear();
	}

	BaseComponent* ComponentAllocator::alloc_component(Entity hnd)
	{
		ComponentPool& pool = prepare_pool();
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(pool.add(ecs_register_sizeof(m_CompID)));

		ecs_register_create(m_CompID, comp, hnd);

		return comp;
	}

	BaseComponent* ComponentAllocator::alloc_component(BaseComponent* src)
	{
		ComponentPool& pool = prepare_pool();
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(pool.add(ecs_register_sizeof(m_CompID)));
		svZeroMemory(comp, ecs_register_sizeof(m_CompID));

		ecs_register_copy(m_CompID, src, comp);

		return comp;
	}

	void ComponentAllocator::free_component(BaseComponent* comp)
	{
		SV_ASSERT(comp != nullptr);

		for (auto it = m_Pools.begin(); it != m_Pools.end(); ++it) {

			if (it->exist(comp)) {
				ecs_register_destroy(m_CompID, comp);
				it->remove(ecs_register_sizeof(m_CompID), comp);
			}

		}
	}

	ui32 ComponentAllocator::size() const noexcept
	{
		ui32 compSize = ecs_register_sizeof(m_CompID);
		ui32 res = 0u;
		for (const ComponentPool& pool : m_Pools) {
			res += pool.size(compSize);
		}
		return res;
	}

	bool ComponentAllocator::empty() const noexcept
	{
		ui32 compSize = ecs_register_sizeof(m_CompID);
		for (const ComponentPool& pool : m_Pools) {
			if (pool.byte_size() > 0u) return false;
		}
		return true;
	}

	ui32 ComponentAllocator::get_pool_count() const noexcept
	{
		return ui32(m_Pools.size());
	}

	ComponentPool& ComponentAllocator::get_pool(ui32 i) noexcept
	{
		return m_Pools[i];
	}

	ComponentPool& ComponentAllocator::create_pool()
	{
		ComponentPool& pool = m_Pools.emplace_back();
		pool.allocate(ecs_register_sizeof(m_CompID));
		return pool;
	}

	ComponentPool& ComponentAllocator::prepare_pool()
	{
		if (m_Pools.back().is_filled(ecs_register_sizeof(m_CompID))) {
			return create_pool();
		}
		return m_Pools.back();
	}
	
}