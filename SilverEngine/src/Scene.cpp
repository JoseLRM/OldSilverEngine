#include "core.h"

#include "Scene.h"

namespace SV {

	using namespace _internal;

	ui32 System::s_SystemCount = 0;
	std::mutex System::s_SystemCountMutex;

	ui32 System::CreateSystemID()
	{
		std::lock_guard<std::mutex> lock(s_SystemCountMutex);
		return s_SystemCount++;
	}

	void System::SetIndividualSystem() noexcept { m_IndividualSystem = true; }
	void System::SetCollectiveSystem() noexcept
	{
		m_IndividualSystem = false;
	}
	void System::SetExecuteType(ui8 type) noexcept
	{
		m_ExecuteType = type;
	}

	void System::AddRequestedComponent(CompID ID) noexcept
	{
		m_RequestedComponents.push_back(ID);
	}
	void System::AddOptionalComponent(CompID ID) noexcept
	{
		m_OptionalComponents.push_back(ID);
	}

	///////////////////////////NAME COMPONENT///////////////
#ifdef SV_IMGUI
	void NameComponent::ShowInfo()
	{
		constexpr ui32 MAX_LENGTH = 32;
		char name[MAX_LENGTH];

		// Set actual name into buffer
		{
			ui32 i;
			ui32 size = m_Name.size();
			if (size >= MAX_LENGTH) size = MAX_LENGTH - 1;
			for (i = 0; i < size; ++i) {
				name[i] = m_Name[i];
			}
			name[i] = '\0';
		}

		ImGui::InputText("Name", name, MAX_LENGTH);

		// Set new name into buffer
		m_Name = name;
	}
#endif

	///////////////////////////SCENE////////////////////////
	BaseComponent* Scene::GetComponent(SV::Entity e, CompID componentID) noexcept
	{
		SV::EntityData& entity = m_EntityData[e];
		auto it = entity.indices.find(componentID);
		if (it != entity.indices.end()) return (BaseComponent*)(&(m_Components[componentID][it->second]));

		return nullptr;
	}
	BaseComponent* Scene::GetComponent(const SV::EntityData& entity, CompID componentID) noexcept
	{
		auto it = entity.indices.find(componentID);
		if (it != entity.indices.end()) return (BaseComponent*)(&(m_Components[componentID][it->second]));

		return nullptr;
	}

	Scene::~Scene()
	{
		Close();
	}

	void Scene::Initialize() noexcept
	{
		m_Components.reserve(ECS::GetComponentsCount());
		for (CompID id = 0; id < ECS::GetComponentsCount(); ++id) {
			m_Components.push_back(std::vector<ui8>());
		}
		// Create Layers
		CreateLayer("Default", 0);

		CreateEntity(); // allocate invalid Entity :)
	}

	void Scene::Close() noexcept
	{
		for (CompID id = 0; id < m_Components.size(); ++id) {
			m_Components[id].clear();
		}
		m_Components.clear();
		m_Entities.clear();
		m_EntityData.clear();
	}

	//////////////////////////////////ENTITIES////////////////////////////////
	void Scene::ReserveEntityData(ui32 count)
	{
		ui32 freeEntityDataCount = ui32(m_FreeEntityData.size());

		if (freeEntityDataCount < count) {
			m_EntityData.reserve(count - freeEntityDataCount);
		}
	}
	SV::Entity Scene::GetNewEntity()
	{
		SV::Entity entity;
		if (m_FreeEntityData.empty()) {
			entity = SV::Entity(m_EntityData.size());
			m_EntityData.emplace_back();
		}
		else {
			entity = m_FreeEntityData.back();
			m_FreeEntityData.pop_back();
		}
		m_EntityData[entity].transform = Transform(entity, this);
		m_EntityData[entity].layer = GetLayer("Default");
		return entity;
	}
	void Scene::UpdateChildsCount(SV::Entity entity, i32 count)
	{
		std::vector<Entity> parentsToUpdate;
		parentsToUpdate.emplace_back(entity);
		while (!parentsToUpdate.empty()) {

			Entity parentToUpdate = parentsToUpdate.back();
			parentsToUpdate.pop_back();
			EntityData& parentToUpdateEd = m_EntityData[parentToUpdate];
			parentToUpdateEd.childsCount += count;

			if (parentToUpdateEd.parent != SV_INVALID_ENTITY) parentsToUpdate.emplace_back(parentToUpdateEd.parent);
		}
	}

	Entity Scene::CreateEntity(SV::Entity parent) noexcept
	{
		ReserveEntityData(1u);
		SV::Entity entity = GetNewEntity();

		if (parent == SV_INVALID_ENTITY) {
			m_EntityData[entity].handleIndex = m_Entities.size();
			m_Entities.emplace_back(entity);
		}
		else {
			UpdateChildsCount(parent, 1u);

			// Set parent and handleIndex
			EntityData& parentData = m_EntityData[parent];
			size_t index = parentData.handleIndex + size_t(parentData.childsCount);

			EntityData& entityData = m_EntityData[entity];
			entityData.handleIndex = index;
			entityData.parent = parent;

			// Special case, the parent and childs are in back of the list
			if (index == m_Entities.size()) {
				m_Entities.emplace_back(entity);
			}
			else {
				m_Entities.insert(m_Entities.begin() + index, entity);

				for (size_t i = index + 1; i < m_Entities.size(); ++i) {
					m_EntityData[m_Entities[i]].handleIndex++;
				}
			}
		}

		return entity;
	}
	void Scene::CreateEntities(ui32 count, SV::Entity parent, std::vector<Entity>* entities) noexcept
	{
		ReserveEntityData(count);

		size_t entityIndex = 0u;

		if (parent == SV_INVALID_ENTITY) {

			entityIndex = m_Entities.size();
			m_Entities.reserve(count);

			// Create entities
			for (size_t i = 0; i < count; ++i) {
				SV::Entity entity = GetNewEntity();
				m_EntityData[entity].handleIndex = entityIndex + i;
				m_Entities.emplace_back(entity);
			}
		}
		else {
			// Indices and reserve memory
			EntityData& parentData = m_EntityData[parent];
			entityIndex = size_t(parentData.handleIndex) + size_t(parentData.childsCount) + 1u;

			size_t lastEntitiesSize = m_Entities.size();
			m_Entities.resize(lastEntitiesSize + count);

			// Update childs count
			UpdateChildsCount(parent, count);

			// if the parent and sons aren't in back of the list
			if (entityIndex != lastEntitiesSize) {
				size_t newDataIndex = entityIndex + size_t(parentData.childsCount);
				// move old entities
				for (size_t i = m_Entities.size() - 1; i >= newDataIndex; --i) {
					m_Entities[i] = m_Entities[i - size_t(count)];
					m_EntityData[m_Entities[i]].handleIndex = i;
				}
			}

			// creating entities
			if (entities) entities->reserve(count);

			for (size_t i = 0; i < count; ++i) {
				SV::Entity entity = GetNewEntity();

				EntityData& entityData = m_EntityData[entity];
				entityData.handleIndex = entityIndex + i;
				entityData.parent = parent;
				m_Entities[entityData.handleIndex] = entity;
			}
		}

		// Allocate entities in user list
		if (entities) {
			entities->reserve(count);

			for (size_t i = 0; i < count; ++i) {
				entities->emplace_back(m_Entities[entityIndex + i]);
			}
		}
	}

	void Scene::DestroyEntity(Entity entity) noexcept
	{
		EntityData& entityData = m_EntityData[entity];
		ui32 count = entityData.childsCount + 1;

		// notify parents
		if (entityData.parent != SV_INVALID_ENTITY) {
			UpdateChildsCount(entityData.parent, -i32(count));
		}

		// data to remove entities
		size_t indexBeginDest = entityData.handleIndex;
		size_t indexBeginSrc = entityData.handleIndex + count;
		size_t cpyCant = m_Entities.size() - indexBeginSrc;

		// remove components & entityData
		m_FreeEntityData.reserve(count);
		for (size_t i = 0; i < count; i++) {
			Entity e = m_Entities[indexBeginDest + i];
			EntityData& ed = m_EntityData[e];
			RemoveComponents(ed);
			ed.handleIndex = 0;
			ed.parent = SV_INVALID_ENTITY;
			ed.childsCount = 0u;
			m_FreeEntityData.emplace_back(e);
		}

		// remove from entities & update indices
		if (cpyCant != 0) memcpy(&m_Entities[indexBeginDest], &m_Entities[indexBeginSrc], cpyCant * sizeof(Entity));
		m_Entities.resize(m_Entities.size() - count);
		for (size_t i = indexBeginDest; i < m_Entities.size(); ++i) {
			m_EntityData[m_Entities[i]].handleIndex = i;
		}

	}

	void Scene::ClearScene()
	{
		m_Entities.clear();
		m_EntityData.clear();
		m_FreeEntityData.clear();
		for (ui32 i = 0; i < ECS::GetComponentsCount(); ++i) {
			m_Components[i].clear();
		}
	}

	SV::Entity Scene::DuplicateEntity(SV::Entity duplicated)
	{
		return DuplicateEntity(duplicated, m_EntityData[duplicated].parent);
	}

	void Scene::SetLayer(SV::Entity entity, SV::SceneLayer* layer) noexcept
	{
		m_EntityData[entity].layer = layer;
	}
	SV::SceneLayer* Scene::GetLayer(SV::Entity entity) const noexcept
	{
		return m_EntityData[entity].layer;
	}

	SV::Entity Scene::DuplicateEntity(SV::Entity duplicated, SV::Entity parent)
	{
		Entity copy;

		if (parent == SV_INVALID_ENTITY) copy = CreateEntity();
		else copy = CreateEntity(parent);

		EntityData& duplicatedEd = m_EntityData[duplicated];
		EntityData& copyEd = m_EntityData[copy];

		for (auto& it : duplicatedEd.indices) {
			CompID ID = it.first;
			size_t SIZE = ECS::GetComponentSize(ID);

			auto& list = m_Components[ID];

			size_t index = list.size();
			list.resize(index + SIZE);

			BaseComponent* comp = GetComponent(duplicated, ID);
			BaseComponent* newComp = reinterpret_cast<BaseComponent*>(&list[index]);
			ECS::CopyComponent(ID, comp, newComp);

			newComp->entity = copy;
			copyEd.indices[ID] = index;
		}

		copyEd.transform = duplicatedEd.transform;
		copyEd.transform.entity = copy;

		for (ui32 i = 0; i < m_EntityData[duplicated].childsCount; ++i) {
			Entity toCopy = m_Entities[m_EntityData[duplicated].handleIndex + i + 1];
			DuplicateEntity(toCopy, copy);
			i += m_EntityData[toCopy].childsCount;
		}

		return copy;
	}

	void Scene::GetEntitySons(SV::Entity parent, SV::Entity** sonsArray, ui32* size) noexcept
	{
		EntityData& ed = m_EntityData[parent];
		*size = ed.childsCount;
		if (sonsArray && ed.childsCount != 0)* sonsArray = &m_Entities[ed.handleIndex + 1];
	}

	SV::Entity Scene::GetEntityParent(SV::Entity entity) {
		return m_EntityData[entity].parent;
	}

	SV::Transform& Scene::GetTransform(SV::Entity entity)
	{
		return m_EntityData[entity].transform;
	}
	SV::SceneLayer* Scene::GetLayerOf(SV::Entity entity)
	{
		return m_EntityData[entity].layer;
	}

	bool Scene::IsEmpty(SV::Entity entity)
	{
		return m_EntityData[entity].indices.empty();
	}

	////////////////////////////////COMPONENTS////////////////////////////////
	void Scene::AddComponent(Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		ECS::MoveComponent(componentID, comp, (BaseComponent*)(&list[index]));
		((BaseComponent*)& list[index])->entity = entity;
		((BaseComponent*)& list[index])->pScene = this;

		// set index in entity
		m_EntityData[entity].indices[componentID] = index;
	}

	void Scene::AddComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		BaseComponent* comp = (BaseComponent*)& list[index];
		ECS::ConstructComponent(componentID, comp, entity, this);

		// set index in entity
		m_EntityData[entity].indices[componentID] = index;
	}

	void Scene::AddComponents(std::vector<Entity>& entities, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the components
		list.resize(list.size() + componentSize * entities.size());

		size_t currentIndex;
		Entity currentEntity;
		for (ui32 i = 0; i < entities.size(); ++i) {
			currentIndex = index + (i * componentSize);
			currentEntity = entities[i];

			ECS::CopyComponent(componentID, comp, (BaseComponent*)(&list[currentIndex]));
			// set entity in component
			BaseComponent* component = (BaseComponent*)(&list[currentIndex]);
			component->entity = currentEntity;
			component->pScene = this;
			// set index in entity
			m_EntityData[currentEntity].indices[componentID] = currentIndex;
		}
	}

	void Scene::RemoveComponent(Entity entity, CompID componentID, size_t componentSize) noexcept
	{
		EntityData& entityData = m_EntityData[entity];
		auto it = entityData.indices.find(componentID);
		if (it == entityData.indices.end()) return;

		size_t index = (*it).second;
		entityData.indices.erase(it);

		auto& list = m_Components[componentID];

		ECS::DestroyComponent(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

		// if the component isn't the last element
		if (index != list.size() - componentSize) {
			// set back data in index
			memcpy(&list[index], &list[list.size() - componentSize], componentSize);

			Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
			m_EntityData[otherEntity].indices[componentID] = index;
		}

		list.resize(list.size() - componentSize);
	}

	void Scene::RemoveComponents(EntityData& entityData) noexcept
	{
		CompID componentID;
		size_t componentSize, index;
		for (auto& it : entityData.indices) {
			componentID = it.first;
			componentSize = ECS::GetComponentSize(componentID);
			index = it.second;
			auto& list = m_Components[componentID];

			ECS::DestroyComponent(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

			if (index != list.size() - componentSize) {
				// set back data in index
				memcpy(&list[index], &list[list.size() - componentSize], componentSize);

				Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
				m_EntityData[otherEntity].indices[componentID] = index;
			}

			list.resize(list.size() - componentSize);
		}
		entityData.indices.clear();
	}

	////////////////////////////////SYSTEMS////////////////////////////////

	void Scene::UpdateSystem(System* system, float dt)
	{
		if (system->GetExecuteType() == SV_ECS_SYSTEM_MULTITHREADED) {			
			UpdateCollectiveSystem(system, dt);
		}
		else {
			if (system->IsIndividualSystem()) UpdateIndividualSystem(system, dt);
			else UpdateCollectiveSystem(system, dt);
		}
	}

	void Scene::UpdateSystems(System** systems, ui32 cant, float dt)
	{
		System* pSystem;

		// update safe systems
		for (ui32 i = 0; i < cant; ++i) {
			pSystem = systems[i];
			if (pSystem->GetExecuteType() == SV_ECS_SYSTEM_SAFE) {
				if (pSystem->IsIndividualSystem()) UpdateIndividualSystem(pSystem, dt);
				else UpdateCollectiveSystem(pSystem, dt);
			}
		}

		// update multithreaded systems
		for (ui32 i = 0; i < cant; ++i) {
			pSystem = systems[i];
			if (pSystem->GetExecuteType() == SV_ECS_SYSTEM_MULTITHREADED) {
				UpdateCollectiveSystem(pSystem, dt);
			}
		}

		// update parallel systems
		for (ui32 i = 0; i < cant; ++i) {
			pSystem = systems[i];
			if (pSystem->GetExecuteType() == SV_ECS_SYSTEM_PARALLEL) {
				if (pSystem->IsIndividualSystem())
					SV::Task::Execute([pSystem, dt, this]() {

					UpdateIndividualSystem(pSystem, dt);

				});
				else SV::Task::Execute([pSystem, dt, this]() {

					UpdateCollectiveSystem(pSystem, dt);

				});
			}
		}
	}

	size_t Scene::GetSortestComponentList(std::vector<CompID>& IDs)
	{
		ui32 index = 0;
		for (ui32 i = 1; i < IDs.size(); ++i) {
			if (m_Components[IDs[i]].size() < m_Components[index].size()) {
				index = i;
			}
		}
		return index;
	}

	void Scene::UpdateIndividualSystem(System* system, float dt)
	{
		// system requisites
		auto& request = system->GetRequestedComponents();
		auto& optional = system->GetOptionalComponents();
		ui32 cantOfComponents = Entity(request.size() + optional.size());

		if (request.size() == 0) return;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		else if (request.size() == 1 && optional.size() == 0) {

			CompID compID = request[0];
			auto& list = m_Components[compID];
			if (list.empty()) return;

			ui32 compSize = ECS::GetComponentSize(compID);
			ui32 bytesCount = ui32(list.size());

			for (ui32 i = 0; i < bytesCount; i += compSize) {
				BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
				system->UpdateEntity(comp->entity, &comp, *this, dt);
			}

			return;
		}

		// for optimization, choose the sortest component list
		ui32 indexOfBestList = GetSortestComponentList(request);
		CompID idOfBestList = request[indexOfBestList];

		// if one request is empty, exit
		auto& list = m_Components[idOfBestList];
		if (list.size() == 0) return;
		size_t sizeOfBestList = ECS::GetComponentSize(idOfBestList);

		// reserve memory for the pointers
		std::vector<BaseComponent*> components;
		components.resize(cantOfComponents);

		// for all the entities
		BaseComponent* compOfBestList;

		for (size_t i = 0; i < list.size(); i += sizeOfBestList) {

			// allocate the best component
			compOfBestList = (BaseComponent*)(&list[i]);
			components[indexOfBestList] = compOfBestList;

			// entity
			Entity entity = compOfBestList->entity;
			EntityData& entityData = m_EntityData[entity];
			bool isValid = true;

			// allocate requested components
			ui32 j;
			for (j = 0; j < request.size(); ++j) {
				if (j == indexOfBestList) continue;

				BaseComponent* comp = GetComponent(entityData, request[j]);
				if (!comp) {
					isValid = false;
					break;
				}
				components[j] = comp;
			}

			if (!isValid) continue;
			// allocate optional components
			for (j = 0; j < optional.size(); ++j) {

				BaseComponent* comp = GetComponent(entityData, optional[j]);
				components[j + request.size()] = comp;
			}

			// if the entity is valid, call update
			system->UpdateEntity(entity, components.data(), *this, dt);
		}
		
	}

	void Scene::UpdateCollectiveSystem(System* system, float dt)
	{
		std::vector<BaseComponent**> componentsList;
		// system requisites
		auto& request = system->GetRequestedComponents();
		auto& optional = system->GetOptionalComponents();
		ui32 cantOfComponents = ui32(request.size() + optional.size());

		if (request.size() == 0) return;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		else if (request.size() == 1 && optional.size() == 0) {

			CompID compID = request[0];
			auto& list = m_Components[compID];
			if (list.empty()) return;

			ui32 compSize = ECS::GetComponentSize(compID);
			ui32 bytesCount = ui32(list.size());
			ui32 numOfEntities = bytesCount / compSize;
			componentsList.resize(numOfEntities);

			BaseComponent** compAllocation = new BaseComponent*[numOfEntities];

			for (ui32 i = 0; i < bytesCount; i += compSize) {
				BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
				ui32 index = i / compSize;
				componentsList[index] = compAllocation + index;
				componentsList[index][0] = comp;
			}

			system->UpdateEntities(componentsList, *this, dt);
			
			delete[] compAllocation;

			return;
		}

		// For optimization, choose the sortest component list
		ui32 indexOfBestList = GetSortestComponentList(request);
		CompID idOfBestList = request[indexOfBestList];
		
		// If one request is empty, exit
		auto& list = m_Components[idOfBestList];
		if (list.size() == 0) return;
		size_t sizeOfBestList = ECS::GetComponentSize(idOfBestList);

		// Allocate dynamic memory
		BaseComponent* compOfBestList;
		BaseComponent** compAllocation = new BaseComponent*[list.size() / sizeOfBestList * size_t(cantOfComponents)];

		// for all the entities
		for (size_t i = 0; i < list.size(); i += sizeOfBestList) {

			// Dynamic allocation offset
			ui32 entityComponentIndex = (i / sizeOfBestList) * cantOfComponents;

			// Put the component of the best component type
			compOfBestList = (BaseComponent*)(&list[i]);
			compAllocation[indexOfBestList + entityComponentIndex] = compOfBestList;
			EntityData& entityData = m_EntityData[compOfBestList->entity];

			bool isValid = true;

			// Add requested components
			ui32 j;
			for (j = 0; j < request.size(); ++j) {
				if (j == indexOfBestList) continue;

				BaseComponent* comp = GetComponent(entityData, request[j]);

				if (!comp) {
					isValid = false;
					break;
				}

				compAllocation[j + entityComponentIndex] = comp;
			}

			if (!isValid) continue;

			// Add optional components
			for (j = 0; j < optional.size(); ++j) {
				BaseComponent* comp = GetComponent(entityData, optional[j]);
				compAllocation[j + request.size() + entityComponentIndex] = comp;
			}

			// Add ptr of ptrs of components
			componentsList.emplace_back(compAllocation + entityComponentIndex);
		}

		if (system->IsIndividualSystem()) {
			//jshTask::Async(componentsList.size(), jshTask::ThreadCount(), [&componentsList, system, dt](ThreadArgs args) {
			//
			//	system->UpdateEntity(componentsList[args.index][0]->entity, componentsList[args.index], dt);
			//
			//});
		}
		else {
			system->UpdateEntities(componentsList, *this, dt);
		}

		// free dynamic memory
		delete[] compAllocation;

	}
	

	//////////////////////////////////LAYERS///////////////////////////////////////
	void Scene::CreateLayer(const char* name, ui16 value)
	{
		SceneLayer layer;
		layer.value = value;
		layer.name = name;

		auto it = m_Layers.find(name);
		if (it != m_Layers.end()) {
			SV::LogW("Repeated Layer '%s'", name);
			return;
		}

		m_Layers[name] = std::make_unique<SceneLayer>(layer);

		// SET IDS
		// sort
		std::vector<SceneLayer*> layers;
		layers.reserve(m_Layers.size());
		for (auto& it : m_Layers) {
			layers.emplace_back(it.second.get());
		}
		std::sort(layers.data(), layers.data() + layers.size() - 1u, [](SceneLayer* layer0, SceneLayer* layer1) {
			return (*layer0) < (*layer1);
		});

		// set
		for (ui16 id = 0; id < layers.size(); ++id) {
			layers[id]->ID = id;
		}
	}
	SV::SceneLayer* Scene::GetLayer(const char* name)
	{
		auto it = m_Layers.find(name);
		if (it == m_Layers.end()) {
			SV::LogE("Layer not found '%s'", name);
			return nullptr;
		}
		return (*it).second.get();
	}
	void Scene::DestroyLayer(const char* name)
	{
		auto it = m_Layers.find(name);
		if (it == m_Layers.end()) {
			SV::LogE("Layer not found '%s'", name);
		}
		m_Layers.erase(it);
	}
	ui32 Scene::GetLayerCount()
	{
		return ui32(m_Layers.size());
	}

	// STATIC
	namespace ECS {
#define SV_ECS_MAX_COMPONENTS 128
		struct ComponentData {
			const char* name;
			size_t size;
			CreateComponentFunction createFn;
			DestoryComponentFunction destroyFn;
			MoveComponentFunction moveFn;
			CopyComponentFunction copyFn;
		};
		ComponentData g_ComponentData[SV_ECS_MAX_COMPONENTS];
		ui16 g_CompCount = 0u;

		namespace _internal {
			CompID RegisterComponent(ui32 size, const std::type_info& typeInfo, CreateComponentFunction createFn, DestoryComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn)
			{
				SV_ASSERT(g_CompCount < SV_ECS_MAX_COMPONENTS);

				CompID ID = g_CompCount++;

				ComponentData& data = g_ComponentData[ID];
				data.size = size;
				data.name = typeInfo.name();
				data.createFn = createFn;
				data.destroyFn = destroyFn;
				data.moveFn = moveFn;
				data.copyFn = copyFn;

				size_t len = strlen(data.name);
				char c = data.name[--len];
				while (c != ' ' && c != ':') {
					c = data.name[len--];
				}

				data.name += len + 2;

				return ID;
			}
		}

		ui16 GetComponentsCount()
		{
			return g_CompCount;
		}
		size_t GetComponentSize(CompID ID)
		{
			return g_ComponentData[ID].size;
		}
		const char* GetComponentName(CompID ID)
		{
			return g_ComponentData[ID].name;
		}
		void ConstructComponent(CompID ID, SV::BaseComponent* ptr, SV::Entity entity, SV::Scene* pScene)
		{
			g_ComponentData[ID].createFn(ptr, entity, pScene);
		}
		void DestroyComponent(CompID ID, SV::BaseComponent* ptr)
		{
			g_ComponentData[ID].destroyFn(ptr);
		}

		void MoveComponent(CompID ID, SV::BaseComponent* from, SV::BaseComponent* to)
		{
			g_ComponentData[ID].moveFn(from, to);
		}

		void CopyComponent(CompID ID, SV::BaseComponent* from, SV::BaseComponent* to)
		{
			g_ComponentData[ID].copyFn(from, to);
		}

		std::map<std::string, CompID> g_ComponentNames;
		bool GetComponentID(const char* name, CompID* id)
		{
			auto it = g_ComponentNames.find(name);
			if (it == g_ComponentNames.end()) {
				for (CompID i = 0; i < GetComponentsCount(); ++i) {
					if (std::strcmp(g_ComponentData[i].name, name) == 0) {
						g_ComponentNames[name] = i;
						*id = i;
						return true;
					}
				}
				*id = 0;
				return false;
			}
			else {
				*id = (*it).second;
				return true;
			}
		}

	}

}