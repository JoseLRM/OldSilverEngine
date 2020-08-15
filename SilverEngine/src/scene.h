#pragma once

#include "core.h"

#define SV_COMPONENT(x, ...) struct x; template Component<x>; struct x : public Component<x>, __VA_ARGS__
#define SV_INVALID_ENTITY 0u

namespace sv {

	typedef ui16 CompID;
	typedef ui32 Entity;
	typedef void* Scene;

	struct BaseComponent {
		Entity entity = SV_INVALID_ENTITY;
	};

	typedef void(*CreateComponentFunction)(BaseComponent*, Entity);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

	typedef void(*SystemFunction)(Entity entity, BaseComponent** components, float dt);

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

#include "scene/Transform.h"

namespace sv {

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

	void scene_ecs_component_add(sv::Entity entity, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize);
	void scene_ecs_components_add(sv::Entity* entities, ui32 count, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize);

	std::vector<sv::Entity>& scene_ecs_get_entities();
	std::vector<ui8>& scene_ecs_get_components(sv::CompID ID);

	Scene	scene_create();
	void	scene_destroy(Scene scene);
	void	scene_clear(Scene scene);

	Entity	scene_camera_get();
	void	scene_camera_set(Entity entity);

	void	scene_bind(Scene scene);
	Scene	scene_get();

	// Entity Functions

	Entity		scene_ecs_entity_create(Entity parent = SV_INVALID_ENTITY);
	void		scene_ecs_entity_destroy(Entity entity);
	Entity		scene_ecs_entity_duplicate(Entity duplicated);
	bool		scene_ecs_entity_is_empty(Entity entity);
	void		scene_ecs_entity_get_childs(Entity parent, Entity const** childsArray, ui32* size);
	Entity		scene_ecs_entity_get_parent(Entity entity);
	Transform	scene_ecs_entity_get_transform(Entity entity);

	void scene_ecs_entities_create(ui32 count, Entity parent = SV_INVALID_ENTITY, Entity* entities = nullptr);
	void scene_ecs_entities_destroy(Entity* entities, ui32 count);

	// Component Functions

	void				scene_ecs_component_add_by_id(sv::Entity entity, sv::CompID componentID);
	sv::BaseComponent*	scene_ecs_component_get_by_id(sv::Entity e, sv::CompID componentID);
	void				scene_ecs_component_remove_by_id(sv::Entity entity, sv::CompID componentID);
	void				scene_ecs_components_remove(Entity entity);

	template<typename Component, typename... Args>
	void scene_ecs_component_add(Entity entity, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		sv::scene_ecs_component_add(entity, (BaseComponent*)& component, Component::ID, Component::SIZE);
	}

	template<typename Component>
	void scene_ecs_component_add(Entity entity) {
		sv::scene_ecs_component_add_by_id(entity, Component::ID);
	}

	template<typename Component>
	Component* scene_ecs_component_get(Entity entity)
	{
		return (Component*)scene_ecs_component_get_by_id(entity, Component::ID);
	}

	template<typename Component, typename... Args>
	void scene_ecs_components_add(Entity* entities, ui32 count, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		sv::scene_ecs_components_add(entities, count, (BaseComponent*)& component, Component::ID, Component::SIZE);
	}
	template<typename Component>
	void scene_ecs_components_add(Entity* entities, ui32 count) {
		sv::scene_ecs_components_add(entities, count, nullptr, Component::ID, Component::SIZE);
	}
	template<typename Component>
	void scene_ecs_component_remove_by_id(Entity entity) {
		scene_ecs_component_remove_by_id(entity, Component::ID);
	}

	// System Functions

	void scene_ecs_system_execute(const SystemDesc* desc, ui32 count, float dt);

	// Entity Iterators

	template<typename Component>
	class EntityView {

		inline std::vector<ui8>& List() const noexcept
		{
			return sv::scene_ecs_get_components(Component::ID);
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