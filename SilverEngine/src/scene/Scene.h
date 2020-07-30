#pragma once

#include "Transform.h"
#include "ComponentsIndices.h"

namespace sv {

	struct SceneLayer
	{
		ui16 value = 0;
		std::string name;
		ui16 ID = 0;
		inline operator ui16() { return value; }
		inline bool SameLayer(const SceneLayer& other) const noexcept { return other.ID == ID; }
		inline bool operator==(const SceneLayer& other) const noexcept { return value == other.value; }
		inline bool operator!=(const SceneLayer& other) const noexcept { return value != other.value; }
		inline bool operator<(const SceneLayer& other) const noexcept { return value < other.value; }
		inline bool operator>(const SceneLayer& other) const noexcept { return value > other.value; }
		inline bool operator<=(const SceneLayer& other) const noexcept { return value <= other.value; }
		inline bool operator>=(const SceneLayer& other) const noexcept { return value >= other.value; }
	};

	struct EntityData {

		size_t handleIndex = 0u;
		Entity parent = SV_INVALID_ENTITY;
		ui32 childsCount = 0u;
		_sv::EntityTransform transform;
		SceneLayer* layer = nullptr;
		ComponentsIndices indices;

	};

	struct BaseComponent {
		Entity entity = SV_INVALID_ENTITY;
	};

}

namespace _sv {

	template<typename T>
	struct Component : public sv::BaseComponent {
		const static sv::CompID ID;
		const static ui32 SIZE;
	};

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

	ui16 ecs_components_get_count();
	size_t ecs_components_get_size(CompID ID);
	const char* ecs_components_get_name(CompID ID);
	void ecs_components_create(CompID ID, BaseComponent* ptr, Entity entity);
	void ecs_components_destroy(CompID ID, BaseComponent* ptr);
	void ecs_components_move(CompID ID, BaseComponent* from, BaseComponent* to);
	void ecs_components_copy(CompID ID, BaseComponent* from, BaseComponent* to);

	bool ecs_components_get_id(const char* name, CompID* id);

}

#define SV_COMPONENT(x, ...) struct x; template _sv::Component<x>; struct x : public _sv::Component<x>, __VA_ARGS__

namespace sv {

	SV_COMPONENT(NameComponent) {
	private:
		std::string m_Name;

	public:
		NameComponent() : m_Name("Unnamed") {}
		NameComponent(const char* name) : m_Name(name) {}

		inline void SetName(const char* name) noexcept { m_Name = name; }
		inline void SetName(const std::string& name) noexcept { m_Name = name; }

		inline const std::string& GetName() const noexcept { return m_Name; }

	};
	
}

namespace sv {

	class Scene {

	public:
		~Scene();

		void Initialize() noexcept;
		void Close() noexcept;

		void ClearScene();

		// ENTITIES
	private:
		std::vector<Entity> m_Entities;
		std::vector<EntityData> m_EntityData;
		std::vector<Entity> m_FreeEntityData;

	public:
		Entity CreateEntity(Entity parent = SV_INVALID_ENTITY) noexcept;
		void CreateEntities(ui32 count, Entity parent = SV_INVALID_ENTITY, Entity* entities = nullptr) noexcept;
		
		Entity DuplicateEntity(Entity duplicated);
		bool IsEmpty(Entity entity);
		void DestroyEntity(Entity entity) noexcept;

		void GetEntityChilds(Entity parent, Entity const** childsArray, ui32* size) const noexcept;
		Entity GetEntityParent(Entity entity);
		Transform GetTransform(Entity entity);

		void SetLayer(Entity entity, SceneLayer* layer) noexcept;
		SceneLayer* GetLayer(Entity entity) const noexcept;
		SceneLayer* GetLayerOf(Entity entity);

		inline std::vector<Entity>& GetEntityList() noexcept { return m_Entities; }
		inline std::vector<EntityData>& GetEntityDataList() noexcept { return m_EntityData; }

	private:
		Entity DuplicateEntity(Entity duplicate, Entity parent);

		void ReserveEntityData(ui32 count);
		Entity GetNewEntity();
		void UpdateChildsCount(Entity entity, i32 count);

		// COMPONENTS

	private:
		std::vector<std::vector<ui8>> m_Components;

	public:
		template<typename Component, typename... Args>
		inline void AddComponent(Entity entity, Args... args) {
			Component component(std::forward<Args...>(args...));
			AddComponent(entity, (BaseComponent*) & component, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline void AddComponent(Entity entity) {
			AddComponent(entity, Component::ID, Component::SIZE);
		}
		template<typename Component, typename... Args>
		inline void AddComponents(Entity* entities, ui32 count, Args... args) {
			Component component(std::forward<Args...>(args...));
			AddComponents(entities, count, (BaseComponent*) & component, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline void AddComponents(Entity* entities, ui32 count) {
			AddComponents(entities, count, nullptr, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline void RemoveComponent(Entity entity) {
			RemoveComponent(entity, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline Component* GetComponent(Entity entity)
		{
			return (Component*)GetComponent(entity, Component::ID);
		}

		inline std::vector<ui8>& GetComponentsList(CompID ID) noexcept { return m_Components[ID]; }

	private:
		BaseComponent* GetComponent(Entity e, CompID componentID) noexcept;
		BaseComponent* GetComponent(const EntityData& e, CompID componentID) noexcept;

		void AddComponent(Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;
		void AddComponent(Entity entity, CompID componentID, size_t componentSize) noexcept;
		void AddComponents(Entity* entities, ui32 count, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;

		void RemoveComponent(Entity entity, CompID componentID, size_t componentSize) noexcept;
		void RemoveComponents(EntityData& entityData) noexcept;

		// LAYERS
	private:
		std::map<std::string, std::unique_ptr<SceneLayer>> m_Layers;
		
	public:	
		void CreateLayer(const char* name, ui16 value);
		SceneLayer* GetLayer(const char* name);
		void DestroyLayer(const char* name);
		ui32 GetLayerCount();

		// SYSTEMS
	public:
		void ExecuteSystems(const SV_ECS_SYSTEM_DESC* params, ui32 count, float dt);

	private:
		// Linear Systems
		void UpdateLinearSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);

		void LinearSystem_OneRequest(SystemFunction system, CompID compID, float dt);
		void LinearSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);

		// MultithreadedSystems
		void UpdateMultithreadedSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);

		void MultithreadedSystem_OneRequest(SystemFunction system, CompID compID, float dt);
		void PartialSystem_OneRequest(SystemFunction system, CompID compID, size_t offset, size_t size, float dt);

		void MultithreadedSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);
		void PartialSystem(SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt);

		// Find the sortest component list and return the index of the list
		ui32 GetSortestComponentList(CompID* compIDs, ui32 count);

	};

}