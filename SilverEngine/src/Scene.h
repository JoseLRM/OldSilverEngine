#pragma once

#include "core.h"
#include "Transform.h"

namespace SV {

	class Scene;
	class EntityData;
	struct BaseComponent;

	typedef ui16 CompID;
	typedef ui32 Entity;

	typedef void(*CreateComponentFunction)(BaseComponent*, SV::Entity, SV::Scene*);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);

	typedef void(*SystemFunction)(Scene& scene, Entity entity, BaseComponent** components, float dt);

	

}

enum SV_ECS_SYSTEM_EXECUTION_MODE : ui8 {
	SV_ECS_SYSTEM_EXECUTION_MODE_SAFE,
	SV_ECS_SYSTEM_EXECUTION_MODE_PARALLEL,
	SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED,
};

struct SV_ECS_SYSTEM_DESC {
	SV::SystemFunction					system;
	SV_ECS_SYSTEM_EXECUTION_MODE		executionMode;
	SV::CompID*							pRequestedComponents;
	ui32								requestedComponentsCount;
	SV::CompID*							pOptionalComponents;
	ui32								optionalComponentsCount;
};

constexpr SV::Entity SV_INVALID_ENTITY = 0u;

namespace SV {

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

	class EntityData {
	public:
		size_t handleIndex = 0u;
		Entity parent = SV_INVALID_ENTITY;
		ui32 childsCount = 0u;
		Transform transform;
		SceneLayer* layer = nullptr;
		std::map<CompID, size_t> indices;

		EntityData() : transform(0, 0) {}
	};

	struct BaseComponent {
		Entity entity = SV_INVALID_ENTITY;
		SV::Scene* pScene = nullptr;
	};

	namespace _internal {
		template<typename T>
		struct Component : public BaseComponent {
			const static CompID ID;
			const static ui32 SIZE;
		};
	}

	namespace ECS {

		namespace _internal {
			CompID RegisterComponent(ui32 size, const std::type_info&, CreateComponentFunction createFn, DestoryComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn);
		}

		ui16 GetComponentsCount();

		size_t GetComponentSize(CompID ID);
		const char* GetComponentName(CompID ID);
		void ConstructComponent(CompID ID, SV::BaseComponent* ptr, SV::Entity entity, SV::Scene*);
		void DestroyComponent(CompID ID, SV::BaseComponent* ptr);
		void MoveComponent(CompID ID, SV::BaseComponent* from, SV::BaseComponent* to);
		void CopyComponent(CompID ID, SV::BaseComponent* from, SV::BaseComponent* to);

		bool GetComponentID(const char* name, CompID* id);

		template<typename Component>
		void CreateComponent(SV::BaseComponent* compPtr, SV::Entity entity, SV::Scene* pScene)
		{
			new(compPtr) Component();
			compPtr->entity = entity;
			compPtr->pScene = pScene;
		}
		template<typename Component>
		void DestroyComponent(SV::BaseComponent* compPtr)
		{
			Component* comp = reinterpret_cast<Component*>(compPtr);
			comp->~Component();
		}
		template<typename Component>
		void MoveComponent(SV::BaseComponent* fromB, SV::BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to	= reinterpret_cast<Component*>(toB);
			to->~Component();
			new(toB) Component();
			to->operator=(std::move(*from));
		}
		template<typename Component>
		void CopyComponent(SV::BaseComponent* fromB, SV::BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			to->~Component();
			new(toB) Component();
			to->operator=(*from);
		}
	}
	
	namespace _internal {
		template<typename T>
		const CompID Component<T>::ID(SV::ECS::_internal::RegisterComponent(sizeof(T), typeid(T), SV::ECS::CreateComponent<T>, SV::ECS::DestroyComponent<T>, SV::ECS::MoveComponent<T>, SV::ECS::CopyComponent<T>));
		template<typename T>
		const ui32 Component<T>::SIZE(sizeof(T));
	}
}

#define SV_COMPONENT(x, ...) struct x; template SV::_internal::Component<x>; struct x : public SV::_internal::Component<x>, __VA_ARGS__

namespace SV {

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

namespace SV {

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
		SV::Entity CreateEntity(SV::Entity parent = SV_INVALID_ENTITY) noexcept;
		void CreateEntities(ui32 count, SV::Entity parent = SV_INVALID_ENTITY, std::vector<SV::Entity>* entities = nullptr) noexcept;
		
		SV::Entity DuplicateEntity(SV::Entity duplicated);
		bool IsEmpty(SV::Entity entity);
		void DestroyEntity(SV::Entity entity) noexcept;

		void GetEntitySons(SV::Entity parent, SV::Entity** sonsArray, ui32* size) noexcept;
		SV::Entity GetEntityParent(SV::Entity entity);
		SV::Transform& GetTransform(SV::Entity entity);

		void SetLayer(SV::Entity entity, SV::SceneLayer* layer) noexcept;
		SV::SceneLayer* GetLayer(SV::Entity entity) const noexcept;
		SV::SceneLayer* GetLayerOf(SV::Entity entity);

		inline std::vector<SV::Entity>& GetEntityList() noexcept { return m_Entities; }
		inline std::vector<SV::EntityData>& GetEntityDataList() noexcept { return m_EntityData; }

	private:
		SV::Entity DuplicateEntity(SV::Entity duplicate, SV::Entity parent);

		void ReserveEntityData(ui32 count);
		SV::Entity GetNewEntity();
		void UpdateChildsCount(SV::Entity entity, i32 count);

		// COMPONENTS

	private:
		std::vector<std::vector<ui8>> m_Components;

	public:
		template<typename Component>
		inline void AddComponent(SV::Entity entity, const Component& component) {
			AddComponent(entity, (SV::BaseComponent*) & component, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline void AddComponents(std::vector<SV::Entity>& entities, const Component& component) {
			AddComponents(entities, (SV::BaseComponent*) & component, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline void RemoveComponent(SV::Entity entity) {
			RemoveComponent(entity, Component::ID, Component::SIZE);
		}
		template<typename Component>
		inline Component* GetComponent(SV::Entity entity)
		{
			return (Component*)GetComponent(entity, Component::ID);
		}

		inline std::vector<ui8>& GetComponentsList(CompID ID) noexcept { return m_Components[ID]; }

	private:
		BaseComponent* GetComponent(SV::Entity e, CompID componentID) noexcept;
		BaseComponent* GetComponent(const SV::EntityData& e, CompID componentID) noexcept;

		void AddComponent(SV::Entity entity, SV::BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;
		void AddComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept;
		void AddComponents(std::vector<SV::Entity>& entities, SV::BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;

		void RemoveComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept;
		void RemoveComponents(SV::EntityData& entityData) noexcept;

		// LAYERS
	private:
		std::map<std::string, std::unique_ptr<SceneLayer>> m_Layers;
		
	public:	
		void CreateLayer(const char* name, ui16 value);
		SV::SceneLayer* GetLayer(const char* name);
		void DestroyLayer(const char* name);
		ui32 GetLayerCount();

		// SYSTEMS
	public:
		void ExecuteSystems(const SV_ECS_SYSTEM_DESC* params, ui32 count, float dt);

	private:
		// Linear Systems
		void UpdateLinearSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);

		void LinearSystem_OneRequest(SV::SystemFunction system, CompID compID, float dt);
		void LinearSystem(SV::SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);

		// MultithreadedSystems
		void UpdateMultithreadedSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);

		void MultithreadedSystem_OneRequest(SV::SystemFunction system, CompID compID, float dt);
		void PartialSystem_OneRequest(SV::SystemFunction system, CompID compID, size_t offset, size_t size, float dt);

		void MultithreadedSystem(SV::SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);
		void PartialSystem(SV::SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt);

		// Find the sortest component list and return the index of the list
		ui32 GetSortestComponentList(CompID* compIDs, ui32 count);

	};

}