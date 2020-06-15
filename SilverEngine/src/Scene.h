#pragma once

#include "core.h"
#include "Transform.h"

#define SV_ECS_SYSTEM_SAFE				0u
#define SV_ECS_SYSTEM_PARALLEL			1u
#define SV_ECS_SYSTEM_MULTITHREADED		2u

namespace SV {

	class Scene;
	class EntityData;
	struct BaseComponent;

	typedef ui16 CompID;
	typedef ui32 Entity;

	typedef void(*CreateComponentFunction)(BaseComponent*, SV::Entity, SV::Scene*);
	typedef void(*DestoryComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);

}

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

		std::map<ui16, size_t> indices;

		EntityData() : transform(0, 0) {}
	};

	struct BaseComponent {
		Entity entity = SV_INVALID_ENTITY;
		SV::Scene* pScene = nullptr;

#ifdef SV_IMGUI
		virtual void ShowInfo() {}
#endif
	};

	template<typename T>
	struct Component : public BaseComponent {
		const static CompID ID;
		const static size_t SIZE;
		const static const char* NAME;
		const static CreateComponentFunction CREATE_FUNCTION;
		const static DestoryComponentFunction DESTROY_FUNCTION;
		const static MoveComponentFunction MOVE_FUNCTION;
	};

	class System {
	private:
		static ui32 s_SystemCount;
		static std::mutex s_SystemCountMutex;

		static ui32 CreateSystemID();

	private:
		std::vector<CompID> m_RequestedComponents;
		std::vector<CompID> m_OptionalComponents;

		bool m_IndividualSystem = true;
		ui8 m_ExecuteType = SV_ECS_SYSTEM_SAFE;

		std::string m_Name;
		ui32 m_SystemID;

	public:
		System() {
			m_SystemID = GetSystemID();
			m_Name = "System " + std::to_string(m_SystemID);
		}

		System(const char* name) {
			m_SystemID = GetSystemID();
			m_Name = name;
		}

		// VIRTUAL METHODS

		virtual void UpdateEntity(Entity entity, BaseComponent** components, Scene& scene, float deltaTime) {}
		virtual void UpdateEntities(std::vector<BaseComponent**>& components, Scene& scene, float deltaTime) {}

		// SETTERS
		void SetIndividualSystem() noexcept;
		void SetCollectiveSystem() noexcept;
		void SetExecuteType(ui8 type) noexcept;

		inline void SetName(const char* name) noexcept { m_Name = name; }

		void AddRequestedComponent(CompID ID) noexcept;
		void AddOptionalComponent(CompID ID) noexcept;

		// GETTERS
		inline const char* GetName() const noexcept { return m_Name.c_str(); }

		inline ui8 GetExecuteType() const noexcept { return m_ExecuteType; }
		inline ui32 GetSystemID() const noexcept { return m_SystemID; }

		inline std::vector<CompID>& GetRequestedComponents() noexcept { return m_RequestedComponents; }
		inline std::vector<CompID>& GetOptionalComponents() noexcept { return m_OptionalComponents; }

		inline bool IsIndividualSystem() const noexcept { return m_IndividualSystem; }
		inline bool IsCollectiveSystem() const noexcept { return !m_IndividualSystem; }

	};

	namespace ECS {

		namespace _internal {
			CompID GetComponentID();
			size_t SetComponentSize(CompID ID, size_t size);
			const char* SetComponentName(CompID ID, const char* name);
			CreateComponentFunction SetComponentCreateFunction(CompID ID, CreateComponentFunction fn);
			DestoryComponentFunction SetComponentDestroyFunction(CompID ID, DestoryComponentFunction fn);
			MoveComponentFunction SetComponentMoveFunction(CompID ID, MoveComponentFunction fn);
		}

		ui16 GetComponentsCount();

		size_t GetComponentSize(CompID ID);
		const char* GetComponentName(CompID ID);
		void ConstructComponent(CompID ID, SV::BaseComponent* ptr, SV::Entity entity, SV::Scene*);
		void DestroyComponent(CompID ID, SV::BaseComponent* ptr);
		void MoveComponent(CompID ID, SV::BaseComponent* from, SV::BaseComponent* to);

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

	}

}

#pragma warning(disable : 4114)
#define SVDefineComponent(name) template struct SV::Component<name>; \
const SV::CompID name::ID(SV::ECS::_internal::GetComponentID());\
const size_t name::SIZE(SV::ECS::_internal::SetComponentSize(name::ID, sizeof(name))); \
const SV::CreateComponentFunction name::CREATE_FUNCTION(SV::ECS::_internal::SetComponentCreateFunction(name::ID, SV::ECS::CreateComponent<name>)); \
const SV::DestoryComponentFunction name::DESTROY_FUNCTION(SV::ECS::_internal::SetComponentDestroyFunction(name::ID, SV::ECS::DestroyComponent<name>)); \
const SV::MoveComponentFunction name::MOVE_FUNCTION(SV::ECS::_internal::SetComponentMoveFunction(name::ID, SV::ECS::MoveComponent<name>)); \
const const char* name::NAME(SV::ECS::_internal::SetComponentName(name::ID, #name));

#define SVDefineTag(name) struct name : public SV::Component<name> {}; SVDefineComponent(name)

namespace SV {

	struct NameComponent : public SV::Component<NameComponent> {
	private:
		std::string m_Name;

	public:
		NameComponent() : m_Name("Unnamed") {}
		NameComponent(const char* name) : m_Name(name) {}

		inline void SetName(const char* name) noexcept { m_Name = name; }
		inline void SetName(const std::string& name) noexcept { m_Name = name; }

		inline const std::string& GetName() const noexcept { return m_Name; }

#ifdef SV_IMGUI
		void ShowInfo() override;
#endif

	};
	SVDefineComponent(NameComponent);

}

namespace SV {

	class Scene {
		std::vector<Entity> m_Entities;
		std::vector<EntityData> m_EntityData;
		std::vector<Entity> m_FreeEntityData;

		std::vector<std::vector<ui8>> m_Components;

		std::map<std::string, std::unique_ptr<SceneLayer>> m_Layers;

	public:
		void Initialize() noexcept;
		void Close() noexcept;

		inline std::vector<SV::Entity>& GetEntityList() noexcept { return m_Entities; }
		inline std::vector<SV::EntityData>& GetEntityDataList() noexcept { return m_EntityData; }
		
		inline std::vector<ui8>& GetComponentsList(CompID ID) noexcept { return m_Components[ID]; }

		void ClearScene();

		// COMPONENT METHODS
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

		BaseComponent* GetComponent(SV::Entity e, CompID componentID) noexcept;
		void AddComponent(SV::Entity entity, SV::BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;
		void AddComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept;
		void AddComponents(std::vector<SV::Entity>& entities, SV::BaseComponent* comp, CompID componentID, size_t componentSize) noexcept;
		void RemoveComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept;

		// ENTITY METHODS
		SV::Entity CreateEntity(SV::Entity parent = SV_INVALID_ENTITY) noexcept;
		void CreateEntities(ui32 count, SV::Entity parent = SV_INVALID_ENTITY, std::vector<SV::Entity>* entities = nullptr) noexcept;

		SV::Entity DuplicateEntity(SV::Entity duplicated);

		void SetLayer(SV::Entity entity, SV::SceneLayer* layer) noexcept;
		SV::SceneLayer* GetLayer(SV::Entity entity) const noexcept;

		void GetEntitySons(SV::Entity parent, SV::Entity** sonsArray, ui32* size) noexcept;
		SV::Entity GetEntityParent(SV::Entity entity);
		SV::Transform& GetTransform(SV::Entity entity);
		SV::SceneLayer* GetLayerOf(SV::Entity entity);
		bool IsEmpty(SV::Entity entity);

		void DestroyEntity(SV::Entity entity) noexcept;

		// system methods
		void UpdateSystem(SV::System* system, float dt);
		void UpdateSystems(SV::System** systems, ui32 cant, float dt);

		// Layers
		void CreateLayer(const char* name, ui16 value);
		SV::SceneLayer* GetLayer(const char* name);
		void DestroyLayer(const char* name);
		ui32 GetLayerCount();

	private:
		SV::Entity DuplicateEntity(SV::Entity duplicate, SV::Entity parent);

		void ReserveEntityData(ui32 count);
		SV::Entity GetNewEntity();
		void UpdateChildsCount(SV::Entity entity, i32 count);

		BaseComponent* GetComponent(const SV::EntityData& e, CompID componentID) noexcept;
		void RemoveComponents(SV::EntityData& entityData) noexcept;

		// systems
		void UpdateIndividualSystem(SV::System* system, float deltaTime);
		void UpdateCollectiveSystem(SV::System* system, float deltaTime);

		// Find the sortest component list and return the index of the list
		size_t GetSortestComponentList(std::vector<CompID>& IDs);

	};

}