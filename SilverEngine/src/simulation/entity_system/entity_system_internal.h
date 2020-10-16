#pragma once

#include "simulation/entity_system.h"

#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Red | sv::LoggingStyle_Green, "[ECS] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Red | sv::LoggingStyle_Green, "[ECS_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[ECS_ERROR] "#x, __VA_ARGS__)

namespace sv {

	constexpr ui32 ECS_COMPONENT_POOL_SIZE = 200u;
	constexpr ui32 ECS_ENTITY_ALLOC_SIZE = 1000u;

	struct ComponentRegister {

		std::string name;
		ui32 size;
		CreateComponentFunction createFn;
		DestroyComponentFunction destroyFn;
		MoveComponentFunction moveFn;
		CopyComponentFunction copyFn;
		SerializeComponentFunction serializeFn;
		DeserializeComponentFunction deserializeFn;

	};

	struct EntityTransform {

		XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
		XMFLOAT3 localRotation = { 0.f, 0.f, 0.f };
		XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

		XMFLOAT4X4 worldMatrix;

		bool modified = true;

	};

	struct EntityData {

		size_t handleIndex = ui64_max;
		Entity parent = SV_ENTITY_NULL;
		ui32 childsCount = 0u;
		std::vector<std::pair<CompID, BaseComponent*>> components;

	};

	struct EntityDataAllocator {
		
		EntityData*				data			= nullptr;
		EntityTransform*		transformData	= nullptr;

		EntityData*				accessData			= nullptr;
		EntityTransform*		accessTransformData = nullptr;

		ui32 size = 0u;
		ui32 capacity = 0u;
		std::vector<Entity> freeList;

		inline EntityData&			operator[](Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessData[entity]; }
		inline EntityTransform&		get_transform(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessTransformData[entity]; }

	};

	struct ComponentPool {
		
		ui8* data;
		size_t size;
		ui32 compSize;
		std::vector<ui8*> freeList;

	};

	struct ComponentAllocator {

		std::vector<ComponentPool> pools;
		CompID compID;

	};

	struct ECS_internal {
		
		std::vector<ComponentRegister>	registers;
		std::vector<Entity>				entities;
		EntityDataAllocator				entityData;
		std::vector<ComponentAllocator>	components;

	};

	Entity	ecs_allocator_entity_alloc(EntityDataAllocator& allocator);
	void	ecs_allocator_entity_free(EntityDataAllocator& allocator, Entity entity);
	void	ecs_allocator_entity_clear(EntityDataAllocator& allocator);

	void	ecs_allocator_component_pool_alloc(ComponentPool& pool, ui32 compSize);
	void	ecs_allocator_component_pool_free(ComponentPool& pool);
	void*	ecs_allocator_component_pool_add(ComponentPool& pool);
	void	ecs_allocator_component_pool_remove(ComponentPool& pool, void* ptr);
	bool	ecs_allocator_component_pool_is_filled(const ComponentPool& pool);
	bool	ecs_allocator_component_pool_exist(const ComponentPool& pool, void* ptr);
	ui32	ecs_allocator_component_pool_count(const ComponentPool& pool);

	ComponentPool&	ecs_allocator_component_prepare_pool(ECS* ecs, ComponentAllocator& a);
	void			ecs_allocator_component_create(ECS* ecs, ComponentAllocator& allocator, CompID ID);
	void			ecs_allocator_component_destroy(ECS* ecs, ComponentAllocator& allocator);
	BaseComponent*	ecs_allocator_component_alloc(ECS* ecs, ComponentAllocator& allocator, Entity entity);
	BaseComponent*	ecs_allocator_component_alloc(ECS* ecs, ComponentAllocator& allocator, BaseComponent* srcComp);
	void			ecs_allocator_component_free(ECS* ecs, ComponentAllocator& allocator, BaseComponent* comp);
	ui32			ecs_allocator_component_count(ECS* ecs, const ComponentAllocator& allocator);
	ui32			ecs_allocator_component_empty(ECS* ecs, const ComponentAllocator& allocator);

}