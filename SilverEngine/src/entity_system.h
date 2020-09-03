#pragma once

#include "core.h"

#define SV_ENTITY_NULL 0u

namespace sv {

	typedef ui16 CompID;
	typedef ui32 Entity;
	typedef void ECS;

	struct BaseComponent {
		Entity entity = SV_ENTITY_NULL;
	};

	template<typename T>
	struct Component : public BaseComponent {
		static CompID ID;
		static ui32 SIZE;
	};

	template<typename T>
	CompID Component<T>::ID;

	template<typename T>
	ui32 Component<T>::SIZE;

	typedef void(*CreateComponentFunction)(BaseComponent*, Entity);
	typedef void(*DestroyComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

}

#include "entity_system/Transform.h"

namespace sv {

	ECS*	ecs_create();
	void	ecs_destroy(ECS* ecs);
	void	ecs_clear(ECS* ecs);

	// Component register

	CompID ecs_register(ui32 size, const char* name, CreateComponentFunction createFn, DestroyComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn);

	ui32		ecs_register_count();
	ui32		ecs_register_sizeof(CompID ID);
	const char* ecs_register_nameof(CompID ID);
	void		ecs_register_create(CompID ID, BaseComponent* ptr, Entity entity);
	void		ecs_register_destroy(CompID ID, BaseComponent* ptr);
	void		ecs_register_move(CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_register_copy(CompID ID, BaseComponent* from, BaseComponent* to);

	// Entity

	Entity		ecs_entity_create(ECS* ecs, Entity parent = SV_ENTITY_NULL);
	void		ecs_entity_destroy(ECS* ecs, Entity entity);
	void		ecs_entity_clear(ECS* ecs, Entity entity);
	Entity		ecs_entity_duplicate(ECS* ecs, Entity entity);
	bool		ecs_entity_is_empty(ECS* ecs, Entity entity);
	bool		ecs_entity_exist(ECS* ecs, Entity entity);
	ui32		ecs_entity_childs_count(ECS* ecs, Entity parent);
	void		ecs_entity_childs_get(ECS* ecs, Entity parent, Entity const** childsArray);
	Entity		ecs_entity_parent_get(ECS* ecs, Entity entity);
	Transform	ecs_entity_transform_get(ECS* ecs, Entity entity);
	ui32		ecs_entity_component_count(ECS* ecs, Entity entity);

	void ecs_entities_create(ECS* ecs, ui32 count, Entity parent = SV_ENTITY_NULL, Entity* entities = nullptr);
	void ecs_entities_destroy(ECS* ecs, Entity const* entities, ui32 count);

	ui32								ecs_entity_count(ECS* ecs);
	Entity								ecs_entity_get(ECS* ecs, ui32 index);

	// Components

	BaseComponent*	ecs_component_add(ECS* ecs, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
	BaseComponent*	ecs_component_add_by_id(ECS* ecs, Entity entity, CompID componentID);

	BaseComponent*						ecs_component_get_by_id(ECS* ecs, Entity entity, CompID componentID);
	std::pair<CompID, BaseComponent*>	ecs_component_get_by_index(ECS* ecs, Entity entity, ui32 index);
	
	void ecs_component_remove_by_id(ECS* ecs, Entity entity, CompID componentID);

	ui32			ecs_component_count(ECS* ecs, CompID ID);

	// Iterators

	class ComponentIterator {
		ECS* ecs_;
		CompID compID;

		BaseComponent* it;
		ui32 pool;

	public:
		ComponentIterator(ECS* ecs, CompID compID, bool end);

		BaseComponent* get_ptr();
		
		void start_begin();
		void start_end();

		bool equal(const ComponentIterator& other) const noexcept;
		void next();
		void last();

	};

	// TEMPLATES
	template<typename Component>
	void ecs_register(const char* name)
	{
		CreateComponentFunction createFn = [](BaseComponent* compPtr, Entity entity)
		{
			new(compPtr) Component();
			compPtr->entity = entity;
		};

		DestroyComponentFunction destroyFn = [](BaseComponent* compPtr)
		{
			Component* comp = reinterpret_cast<Component*>(compPtr);
			comp->~Component();
		};

		MoveComponentFunction moveFn = [](BaseComponent* fromB, BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			new(to) Component();
			to->operator=(std::move(*from));
		};

		CopyComponentFunction copyFn = [](BaseComponent* fromB, BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			new(to) Component();
			to->operator=(*from);
		};

		Component::SIZE = sizeof(Component);
		Component::ID = ecs_register(Component::SIZE, name, createFn, destroyFn, moveFn, copyFn);
	}

	template<typename Component, typename... Args>
	Component* ecs_component_add(ECS* ecs, Entity entity, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		return reinterpret_cast<Component*>(ecs_component_add(ecs, entity, (BaseComponent*)& component, Component::ID, Component::SIZE));
	}

	template<typename Component>
	Component* ecs_component_add(ECS* ecs, Entity entity) {
		return reinterpret_cast<Component*>(ecs_component_add_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	Component* ecs_component_get(ECS* ecs, Entity entity)
	{
		return reinterpret_cast<Component*>(ecs_component_get_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	void ecs_component_remove(ECS* ecs, Entity entity) {
		ecs_component_remove_by_id(ecs, entity, Component::ID);
	}

	template<typename Component>
	class EntityView {

		ECS* m_ECS;

	public:

		class TemplatedComponentIterator {
			ComponentIterator it;

		public:
			TemplatedComponentIterator(ECS* ecs, CompID compID, bool end)
				: it(ecs, compID, end) {}
			inline Component* operator->() { return reinterpret_cast<Component*>(it.get_ptr()); }
			inline Component& operator*() { return *reinterpret_cast<Component*>(it.get_ptr()); }

			inline bool operator==(const TemplatedComponentIterator& other) const noexcept { return it.equal(other.it); }
			inline bool operator!=(const TemplatedComponentIterator& other) const noexcept { return !it.equal(other.it); }
			inline void operator+=(ui32 count) { while (count-- > 0) it.next(); }
			inline void operator-=(ui32 count) { while (count-- > 0) it.last(); }
			inline void operator++() { it.next(); }
			inline void operator--() { it.last(); }
		};

	public:
		EntityView(ECS* ecs) : m_ECS(ecs) {}

		ui32 size()
		{
			return ecs_component_count(m_ECS, Component::ID);
		}

		TemplatedComponentIterator begin()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, false);
			return iterator;
		}

		TemplatedComponentIterator end()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, true);
			return iterator;
		}

	};

}