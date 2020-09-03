#pragma once

#include "entity_system.h"

namespace sv {

	struct EntityTransform {

		XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
		XMFLOAT3 localRotation = { 0.f, 0.f, 0.f };
		XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 worldMatrix;

		bool modified = true;
		bool wakePhysics = true;

	};

	struct EntityData {

		size_t handleIndex = 0u;
		Entity parent = SV_ENTITY_NULL;
		ui32 childsCount = 0u;
		EntityTransform transform;
		std::vector<std::pair<CompID, BaseComponent*>> components;

	};

	class EntityDataAllocator {
		EntityData* m_Data;
		ui32 m_Size;
		ui32 m_Capacity;

		std::vector<Entity> m_FreeList;

	public:
		EntityDataAllocator();

		Entity add();
		void remove(Entity entity);
		inline EntityData& operator[](Entity entity) noexcept { return m_Data[entity]; }
		void clear();
		inline ui32 size() const noexcept { return m_Size; }

	};

	class ComponentPool {
		ui8* m_Data;
		size_t m_Size;

		std::vector<ui8*> m_FreeList;

	public:

		ComponentPool();
		~ComponentPool();

		ComponentPool(const ComponentPool& other) = delete;
		ComponentPool(ComponentPool&& other) noexcept;

		void allocate(ui32 compSize);
		void free();
		void* add(ui32 compSize) noexcept;
		void remove(ui32 compSize, void* ptr);
		bool is_filled(ui32 compSize) const noexcept;
		bool exist(void* ptr) const noexcept;
		ui32 size(ui32 compSize) const noexcept;

		inline size_t byte_size() const noexcept { return m_Size; }
		inline ui8* get() const noexcept { return m_Data; }

	};

	class ComponentAllocator {

		std::vector<ComponentPool> m_Pools;
		CompID m_CompID;

	public:
		void create(CompID compID);
		void destroy();
		
		// Uses constructor
		BaseComponent* alloc_component(Entity hnd); 
		// Uses copy
		BaseComponent* alloc_component(BaseComponent* src); 

		// Uses destructor
		void free_component(BaseComponent* comp); 

		ui32 size() const noexcept;
		bool empty() const noexcept;
		ui32 get_pool_count() const noexcept;
		ComponentPool& get_pool(ui32 i) noexcept;

	private:
		ComponentPool& create_pool();
		ComponentPool& prepare_pool();

	};

	struct ECS_internal {
		
		std::vector<Entity>				entities;
		EntityDataAllocator				entityData;
		std::vector<ComponentAllocator>	components;

	};

}