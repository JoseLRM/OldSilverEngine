#pragma once

#include "Transform.h"

namespace _sv {

	sv::CompID ecs_components_register(ui32 size, const std::type_info&, sv::CreateComponentFunction createFn, sv::DestoryComponentFunction destroyFn, sv::MoveComponentFunction moveFn, sv::CopyComponentFunction copyFn);

	template<typename Component>
	void ecs_components_create_function(sv::BaseComponent* compPtr, sv::Entity entity)
	{
		new(compPtr) Component();
		compPtr->entity = entity;
	}
	template<typename Component>
	void ecs_components_destroy_function(sv::BaseComponent* compPtr)
	{
		Component* comp = reinterpret_cast<Component*>(compPtr);
		comp->~Component();
	}
	template<typename Component>
	void ecs_components_move_function(sv::BaseComponent* fromB, sv::BaseComponent* toB)
	{
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		to->~Component();
		new(toB) Component();
		to->operator=(std::move(*from));
	}
	template<typename Component>
	void ecs_components_copy_function(sv::BaseComponent* fromB, sv::BaseComponent* toB)
	{
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		to->~Component();
		new(toB) Component();
		to->operator=(*from);
	}

	template<typename T>
	const sv::CompID Component<T>::ID(_sv::ecs_components_register(sizeof(T), typeid(T), _sv::ecs_components_create_function<T>, _sv::ecs_components_destroy_function<T>, _sv::ecs_components_move_function<T>, _sv::ecs_components_copy_function<T>));
	template<typename T>
	const ui32 Component<T>::SIZE(sizeof(T));

}

namespace sv {

	ui16		ecs_components_get_count();
	size_t		ecs_components_get_size(CompID ID);
	const char* ecs_components_get_name(CompID ID);
	bool		ecs_components_get_id(const char* name, CompID* id);
	void		ecs_components_create(CompID ID, BaseComponent* ptr, Entity entity);
	void		ecs_components_destroy(CompID ID, BaseComponent* ptr);
	void		ecs_components_move(CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_components_copy(CompID ID, BaseComponent* from, BaseComponent* to);

}

namespace _sv {

	void scene_ecs_component_add(sv::Entity entity, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize);
	void scene_ecs_components_add(sv::Entity* entities, ui32 count, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize);

	std::vector<EntityData>& scene_ecs_get_entity_data();
	EntityData& scene_ecs_get_entity_data(sv::Entity entity);
	std::vector<sv::Entity>& scene_ecs_get_entities();
	std::vector<ui8>& scene_ecs_get_components(sv::CompID ID);

}

namespace sv {

	Scene	scene_create();
	void	scene_destroy(Scene& scene);
	void	scene_clear(Scene scene);

	Entity	scene_camera_get();
	void	scene_camera_set(Entity entity);

	PhysicsWorld& scene_physics_world_get();

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
		_sv::scene_ecs_component_add(entity, (BaseComponent*)& component, Component::ID, Component::SIZE);
	}

	template<typename Component>
	void scene_ecs_component_add(Entity entity) {
		_sv::scene_ecs_component_add_by_id(entity, Component::ID);
	}

	template<typename Component>
	Component* scene_ecs_component_get(Entity entity)
	{
		return (Component*)scene_ecs_component_get_by_id(entity, Component::ID);
	}

	template<typename Component, typename... Args>
	void scene_ecs_components_add(Entity* entities, ui32 count, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		_sv::scene_ecs_components_add(entities, count, (BaseComponent*)& component, Component::ID, Component::SIZE);
	}
	template<typename Component>
	void scene_ecs_components_add(Entity* entities, ui32 count) {
		_sv::scene_ecs_components_add(entities, count, nullptr, Component::ID, Component::SIZE);
	}
	template<typename Component>
	void scene_ecs_component_remove_by_id(Entity entity) {
		scene_ecs_component_remove_by_id(entity, Component::ID);
	}

	// System Functions

	void scene_ecs_system_execute(const SV_ECS_SYSTEM_DESC* desc, ui32 count, float dt);

	// Default Components

	SV_COMPONENT(NameComponent) {
private:
	std::string m_Name;

public:
	NameComponent() : m_Name("Unnamed") {}
	NameComponent(const char* name) : m_Name(name) {}

	inline void SetName(const char* name) noexcept { m_Name = name; }
	inline void SetName(const std::string & name) noexcept { m_Name = name; }

	inline const std::string& GetName() const noexcept { return m_Name; }

	};

}