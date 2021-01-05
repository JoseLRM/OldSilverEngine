#pragma once

#include "simulation/entity_system.h"

#define parseECS() ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(ecs_)

namespace sv {

	constexpr u32 ECS_COMPONENT_POOL_SIZE = 100u;
	constexpr u32 ECS_ENTITY_ALLOC_SIZE = 100u;

	struct ComponentRegister {

		std::string						name;
		u32								size;
		CreateComponentFunction			createFn = nullptr;
		DestroyComponentFunction		destroyFn = nullptr;
		MoveComponentFunction			moveFn = nullptr;
		CopyComponentFunction			copyFn = nullptr;
		SerializeComponentFunction		serializeFn = nullptr;
		DeserializeComponentFunction	deserializeFn = nullptr;

	};

	struct EntityTransform {

		XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
		XMFLOAT4 localRotation = { 0.f, 0.f, 0.f, 1.f };
		XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 worldMatrix;

		bool modified = true;

	};

	struct EntityData {

		size_t handleIndex = u64_max;
		Entity parent = SV_ENTITY_NULL;
		u32 childsCount = 0u;
		std::vector<std::pair<CompID, BaseComponent*>> components;

	};

	struct EntityDataAllocator {

		EntityData* data = nullptr;
		EntityTransform* transformData = nullptr;

		EntityData* accessData = nullptr;
		EntityTransform* accessTransformData = nullptr;

		u32 size = 0u;
		u32 capacity = 0u;
		std::vector<Entity> freeList;

		inline EntityData& operator[](Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessData[entity]; }
		inline EntityTransform& get_transform(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessTransformData[entity]; }

	};

	struct ComponentPool {

		u8*			data;
		size_t		size;
		size_t		freeCount;

	};

	struct ComponentAllocator {

		std::vector<ComponentPool> pools;

	};

	struct ECS_internal {

		std::vector<Entity>				entities;
		EntityDataAllocator				entityData;
		std::vector<ComponentAllocator>	components;

		EventListener* listenerOnEntityCreate;
		EventListener* listenerOnEntityDestroy;
		EventListener* listenerOnComponentAdd;
		EventListener* listenerOnComponentRemove;
		std::vector<std::pair<EventListener*, EventListener*>> listenerComponents; // first-> OnComponentAdd, second-> OnComponentRemove

	};

	// MEMORY

	Entity	entityAlloc(EntityDataAllocator& allocator);
	void	entityFree(EntityDataAllocator& allocator, Entity entity);
	void	entityClear(EntityDataAllocator& allocator);

	void	componentPoolAlloc(ComponentPool& pool, size_t compSize);						// Allocate components memory
	void	componentPoolFree(ComponentPool& pool);											// Deallocate components memory
	void*	componentPoolGetPtr(ComponentPool& pool, size_t compSize);						// Return new component
	void	componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr);			// Remove component
	bool	componentPoolFull(const ComponentPool& pool, size_t compSize);					// Check if there are free space in the pool
	bool	componentPoolPtrExist(const ComponentPool& pool, void* ptr);					// Check if the pool contains the ptr
	u32		componentPoolCount(const ComponentPool& pool, size_t compSize);					// Return the number of valid components allocated in this pool

	void			componentAllocatorCreate(ECS* ecs, CompID ID);									// Create the allocator
	void			componentAllocatorDestroy(ECS* ecs, CompID ID);									// Destroy the allocator
	BaseComponent*	componentAlloc(ECS* ecs, CompID compID, Entity entity, bool create = true);		// Allocate and create new component
	BaseComponent*	componentAlloc(ECS* ecs, CompID compID, BaseComponent* srcComp);				// Allocate and copy new component
	void			componentFree(ECS* ecs, CompID compID, BaseComponent* comp);					// Free and destroy component
	u32				componentAllocatorCount(ECS* ecs, CompID compId);								// Return the number of valid components in all the pools
	bool			componentAllocatorIsEmpty(ECS* ecs, CompID compID);								// Return if the allocator is empty

	// EVENTS

	void ecs_dispatch_OnEntityCreate(ECS_internal& ecs, Entity entity);
	void ecs_dispatch_OnEntityDestroy(ECS_internal& ecs, Entity entity);
	void ecs_dispatch_OnComponentAdd(ECS_internal& ecs, BaseComponent* comp, CompID ID);
	void ecs_dispatch_OnComponentRemove(ECS_internal& ecs, BaseComponent* comp, CompID ID);

}