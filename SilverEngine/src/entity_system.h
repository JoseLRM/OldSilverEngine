#pragma once

#include "core.h"

#define SV_COMPONENT(x, ...) struct x; template Component<x>; struct x : public Component<x>, __VA_ARGS__
#define SV_ENTITY_NULL 0u

namespace sv {

	typedef ui16 CompID;
	typedef ui32 Entity;

	struct BaseComponent {
		Entity entity = SV_ENTITY_NULL;
	};

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
		std::vector<std::pair<CompID, size_t>> indices;

	};

	struct ECS {
		std::vector<Entity>				entities;
		std::vector<EntityData>			entityData;
		std::vector<Entity>				freeEntityData;
		std::vector<std::vector<ui8>>	components;
	};

	typedef void(*CreateComponentFunction)(BaseComponent*, Entity);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

	typedef void(*SystemFunction)(Entity entity, BaseComponent** components, ECS& ecs, float dt);

	enum SystemExecutionMode : ui8 {
		SystemExecutionMode_Safe,
		SystemExecutionMode_Parallel,
		SystemExecutionMode_Multithreaded,
	};

	struct SystemDesc {
		SystemFunction			system;
		SystemExecutionMode		executionMode;
		CompID*					pRequestedComponents;
		ui32					requestedComponentsCount;
		CompID*					pOptionalComponents;
		ui32					optionalComponentsCount;
	};

	template<typename T>
	struct Component : public BaseComponent {
		const static CompID ID;
		const static ui32 SIZE;
	};

}

#include "entity_system/Transform.h"

namespace sv {

	ECS		ecs_create();
	void	ecs_destroy(ECS& ecs);
	void	ecs_clear(ECS& ecs);

	// Components types

	CompID ecs_components_register(ui32 size, const std::type_info&, CreateComponentFunction createFn, DestoryComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn);

	template<typename Component>
	void ecs_components_create_function(BaseComponent* compPtr, Entity entity)
	{
		new(compPtr) Component();
		compPtr->entity = entity;
	}
	template<typename Component>
	void ecs_components_destroy_function(BaseComponent* compPtr)
	{
		Component* comp = reinterpret_cast<Component*>(compPtr);
		comp->~Component();
	}
	template<typename Component>
	void ecs_components_move_function(BaseComponent* fromB, BaseComponent* toB)
	{
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		to->~Component();
		new(toB) Component();
		to->operator=(std::move(*from));
	}
	template<typename Component>
	void ecs_components_copy_function(BaseComponent* fromB, BaseComponent* toB)
	{
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		to->~Component();
		new(toB) Component();
		to->operator=(*from);
	}

	template<typename T>
	const CompID Component<T>::ID(ecs_components_register(sizeof(T), typeid(T), ecs_components_create_function<T>, ecs_components_destroy_function<T>, ecs_components_move_function<T>, ecs_components_copy_function<T>));
	template<typename T>
	const ui32 Component<T>::SIZE(sizeof(T));

	ui16		ecs_components_get_count();
	size_t		ecs_components_get_size(CompID ID);
	const char* ecs_components_get_name(CompID ID);
	bool		ecs_components_get_id(const char* name, CompID* id);
	void		ecs_components_create(CompID ID, BaseComponent* ptr, Entity entity);
	void		ecs_components_destroy(CompID ID, BaseComponent* ptr);
	void		ecs_components_move(CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_components_copy(CompID ID, BaseComponent* from, BaseComponent* to);

	// Entity

	Entity		ecs_entity_create(Entity parent, ECS& ecs);
	void		ecs_entity_destroy(Entity entity, ECS& ecs);
	Entity		ecs_entity_duplicate(Entity duplicated, ECS& ecs);
	bool		ecs_entity_is_empty(Entity entity, ECS& ecs);
	void		ecs_entity_get_childs(Entity parent, Entity const** childsArray, ui32* size, ECS& ecs);
	Entity		ecs_entity_get_parent(Entity entity, ECS& ecs);
	Transform	ecs_entity_get_transform(Entity entity, ECS& ecs);

	void ecs_entities_create(ui32 count, Entity parent, Entity* entities, ECS& ecs);
	void ecs_entities_destroy(Entity* entities, ui32 count, ECS& ecs);

	// Components

	void ecs_component_add(sv::Entity entity, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize, ECS& ecs);
	void ecs_components_add(sv::Entity* entities, ui32 count, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize, ECS& ecs);

	void				ecs_component_add_by_id(sv::Entity entity, sv::CompID componentID, ECS& ecs);
	sv::BaseComponent*	ecs_component_get_by_id(sv::Entity e, sv::CompID componentID, ECS& ecs);
	void				ecs_component_remove_by_id(sv::Entity entity, sv::CompID componentID, ECS& ecs);
	void				ecs_components_remove(Entity entity, ECS& ecs);

	template<typename Component, typename... Args>
	void ecs_component_add(Entity entity, ECS& ecs, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		sv::ecs_component_add(entity, (BaseComponent*)& component, Component::ID, Component::SIZE, ecs);
	}

	template<typename Component>
	void ecs_component_add(Entity entity, ECS& ecs) {
		sv::ecs_component_add_by_id(entity, Component::ID, ecs);
	}

	template<typename Component>
	Component* ecs_component_get(Entity entity, ECS& ecs)
	{
		return (Component*)ecs_component_get_by_id(entity, Component::ID, ecs);
	}

	template<typename Component, typename... Args>
	void ecs_components_add(Entity* entities, ui32 count, ECS& ecs, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		sv::ecs_components_add(entities, count, (BaseComponent*)& component, Component::ID, Component::SIZE, ecs);
	}
	template<typename Component>
	void ecs_components_add(Entity* entities, ui32 count, ECS& ecs) {
		sv::ecs_components_add(entities, count, nullptr, Component::ID, Component::SIZE, ecs);
	}
	template<typename Component>
	void ecs_component_remove_by_id(Entity entity, ECS& ecs) {
		ecs_component_remove_by_id(entity, Component::ID, ecs);
	}

	// System

	void ecs_system_execute(const SystemDesc* desc, ui32 count, float dt, ECS& ecs);

	// Entity Iterators

	template<typename Component>
	class EntityView {

		ECS* pECS;

		inline std::vector<ui8>& List() const noexcept
		{
			return pECS->components[Component::ID];
		}

	public:
		class Iterator {
			Component* ptr;

		public:
			Iterator(Component* ptr) : ptr(ptr) {}

			Component* operator->() { return ptr; }
			bool operator==(const Iterator& other) { return ptr == other.ptr; }
			bool operator!=(const Iterator& other) { return ptr != other.ptr; }
			void operator+=(ui32 count) { ptr += count; }
			void operator-=(ui32 count) { ptr -= count; }
			void operator++() { ptr++; }
			void operator--() { ptr--; }
			Component& operator*() { return *ptr; }
		};

	public:
		EntityView(ECS& ecs) : pECS(&ecs) {}

		ui32 size()
		{
			return List().size() / sizeof(Component);
		}

		Iterator begin()
		{
			return { reinterpret_cast<Component*>(List().data()) };
		}

		Iterator end()
		{
			return { reinterpret_cast<Component*>(List().data() + List().size()) };
		}

		Component& operator[](ui32 index) {
			return *reinterpret_cast<Component*>(&List()[index * sizeof(Component)]);
		}
		const Component& operator[](ui32 index) const {
			return *reinterpret_cast<Component*>(&List()[index * sizeof(Component)]);
		}

	};

}