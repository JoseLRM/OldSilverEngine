#include "core.h"

#include "Scene.h"

namespace SV {

	using namespace _internal;

	///////////////////////////SCENE////////////////////////

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

	void Scene::ClearScene()
	{
		m_Entities.clear();
		m_EntityData.clear();
		m_FreeEntityData.clear();
		for (ui32 i = 0; i < ECS::GetComponentsCount(); ++i) {
			m_Components[i].clear();
		}
	}

	Scene::~Scene()
	{
		Close();
	}

	//////////////////////////////////ENTITIES////////////////////////////////

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
	void Scene::CreateEntities(ui32 count, SV::Entity parent, Entity* entities) noexcept
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
			for (size_t i = 0; i < count; ++i) {
				entities[i] = m_Entities[entityIndex + i];
			}
		}
	}

	SV::Entity Scene::DuplicateEntity(SV::Entity duplicated)
	{
		return DuplicateEntity(duplicated, m_EntityData[duplicated].parent);
	}

	bool Scene::IsEmpty(SV::Entity entity)
	{
		return m_EntityData[entity].indices.Empty();
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

	void Scene::GetEntityChilds(SV::Entity parent, SV::Entity const** childsArray, ui32* size) const noexcept
	{
		const EntityData& ed = m_EntityData[parent];
		*size = ed.childsCount;
		if (childsArray && ed.childsCount != 0) *childsArray = &m_Entities[ed.handleIndex + 1];
	}

	SV::Entity Scene::GetEntityParent(SV::Entity entity) {
		return m_EntityData[entity].parent;
	}

	SV::Transform Scene::GetTransform(SV::Entity entity)
	{
		return SV::Transform(entity, &m_EntityData[entity].transform, this);
	}

	void Scene::SetLayer(SV::Entity entity, SV::SceneLayer* layer) noexcept
	{
		m_EntityData[entity].layer = layer;
	}

	SV::SceneLayer* Scene::GetLayer(SV::Entity entity) const noexcept
	{
		return m_EntityData[entity].layer;
	}

	SV::SceneLayer* Scene::GetLayerOf(SV::Entity entity)
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

		for (ui32 i = 0; i < duplicatedEd.indices.Size(); ++i) {
			CompID ID = duplicatedEd.indices[i];
			size_t SIZE = ECS::GetComponentSize(ID);

			auto& list = m_Components[ID];

			size_t index = list.size();
			list.resize(index + SIZE);

			BaseComponent* comp = GetComponent(duplicated, ID);
			BaseComponent* newComp = reinterpret_cast<BaseComponent*>(&list[index]);
			ECS::CopyComponent(ID, comp, newComp);

			newComp->entity = copy;
			copyEd.indices.AddIndex(ID, index);
		}

		copyEd.transform = duplicatedEd.transform;

		for (ui32 i = 0; i < m_EntityData[duplicated].childsCount; ++i) {
			Entity toCopy = m_Entities[m_EntityData[duplicated].handleIndex + i + 1];
			DuplicateEntity(toCopy, copy);
			i += m_EntityData[toCopy].childsCount;
		}

		return copy;
	}

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
		m_EntityData[entity].transform = {};
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

	////////////////////////////////COMPONENTS////////////////////////////////

	BaseComponent* Scene::GetComponent(SV::Entity e, CompID componentID) noexcept
	{
		SV::EntityData& entity = m_EntityData[e];
		size_t index;
		if (entity.indices.GetIndex(componentID, index)) {
			return (BaseComponent*)(&(m_Components[componentID][index]));
		}
		return nullptr;
	}
	BaseComponent* Scene::GetComponent(const SV::EntityData& entity, CompID componentID) noexcept
	{
		size_t index;
		if (entity.indices.GetIndex(componentID, index)) {
			return (BaseComponent*)(&(m_Components[componentID][index]));
		}
		return nullptr;
	}

	void Scene::AddComponent(Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		ECS::MoveComponent(componentID, comp, (BaseComponent*)(&list[index]));
		((BaseComponent*)& list[index])->entity = entity;

		// set index in entity
		m_EntityData[entity].indices.AddIndex(componentID, index);
	}

	void Scene::AddComponent(SV::Entity entity, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		BaseComponent* comp = (BaseComponent*)& list[index];
		ECS::ConstructComponent(componentID, comp, entity);

		// set index in entity
		m_EntityData[entity].indices.AddIndex(componentID, index);
	}

	void Scene::AddComponents(SV::Entity* entities, ui32 count, BaseComponent* comp, CompID componentID, size_t componentSize) noexcept
	{
		auto& list = m_Components[componentID];
		size_t index = list.size();

		// allocate the components
		list.resize(list.size() + componentSize * ui64(count));

		size_t currentIndex;
		Entity currentEntity;
		for (ui32 i = 0; i < count; ++i) {
			currentIndex = index + (i * componentSize);
			currentEntity = entities[i];

			if (comp) {
				ECS::CopyComponent(componentID, comp, (BaseComponent*)(&list[currentIndex]));
			}
			else {
				ECS::ConstructComponent(componentID, (BaseComponent*)(&list[currentIndex]), currentEntity);
			}

			// set entity in component
			BaseComponent* component = (BaseComponent*)(&list[currentIndex]);
			component->entity = currentEntity;
			// set index in entity
			m_EntityData[currentEntity].indices.AddIndex(componentID, currentIndex);
		}
	}

	void Scene::RemoveComponent(Entity entity, CompID componentID, size_t componentSize) noexcept
	{
		EntityData& entityData = m_EntityData[entity];

		// Get the index
		size_t index;
		if (!entityData.indices.GetIndex(componentID, index)) return;

		// Remove index from index list
		entityData.indices.RemoveIndex(componentID);

		auto& list = m_Components[componentID];

		ECS::DestroyComponent(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

		// if the component isn't the last element
		if (index != list.size() - componentSize) {
			// set back data in index
			memcpy(&list[index], &list[list.size() - componentSize], componentSize);

			Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
			m_EntityData[otherEntity].indices.AddIndex(componentID, index);
		}

		list.resize(list.size() - componentSize);
	}

	void Scene::RemoveComponents(EntityData& entityData) noexcept
	{
		CompID componentID;
		size_t componentSize, index;
		for (ui32 i = 0; i < entityData.indices.Size(); ++i) {
			componentID = entityData.indices[i];
			componentSize = ECS::GetComponentSize(componentID);
			entityData.indices.GetIndex(componentID, index);
			auto& list = m_Components[componentID];

			ECS::DestroyComponent(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

			if (index != list.size() - componentSize) {
				// set back data in index
				memcpy(&list[index], &list[list.size() - componentSize], componentSize);

				Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
				m_EntityData[otherEntity].indices.AddIndex(componentID, index);
			}

			list.resize(list.size() - componentSize);
		}
		entityData.indices.Clear();
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

	////////////////////////////////SYSTEMS////////////////////////////////

	void Scene::ExecuteSystems(const SV_ECS_SYSTEM_DESC* desc, ui32 count, float dt)
	{
		if (count == 0) return;

		if (count == 1) {
			if (desc[0].executionMode == SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED) {
				UpdateMultithreadedSystem(desc[0], dt);
			}
			else {
				UpdateLinearSystem(*desc, dt);
			}
		}
		else {

			ThreadContext ctx;

			for (ui32 i = 0; i < count; ++i) {
				if (desc[i].executionMode == SV_ECS_SYSTEM_EXECUTION_MODE_PARALLEL) {
					Task::Execute([this, desc, i, dt]() {
						UpdateLinearSystem(desc[i], dt);
					}, &ctx);
				}
			}

			for (ui32 i = 0; i < count; ++i) {

				if (desc[i].executionMode == SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED) {
					UpdateMultithreadedSystem(desc[i], dt);
				}
				else if (desc[i].executionMode == SV_ECS_SYSTEM_EXECUTION_MODE_SAFE) {
					UpdateLinearSystem(desc[i], dt);
				}

			}

			Task::Wait(ctx);
		}
	}

	void Scene::UpdateLinearSystem(const SV_ECS_SYSTEM_DESC& desc, float dt)
	{
		if (desc.requestedComponentsCount == 0) return;

		// system requisites
		CompID* request = desc.pRequestedComponents;
		CompID* optional = desc.pOptionalComponents;
		ui32 requestCount = desc.requestedComponentsCount;
		ui32 optionalCount = desc.optionalComponentsCount;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		if (optionalCount == 0 && requestCount == 1) {
			LinearSystem_OneRequest(desc.system, request[0], dt);
		}
		else {
			LinearSystem(desc.system, request, requestCount, optional, optionalCount, dt);
		}
	}

	void Scene::LinearSystem_OneRequest(SV::SystemFunction system, CompID compID, float dt)
	{
		auto& list = m_Components[compID];
		if (list.empty()) return;

		ui32 compSize = ECS::GetComponentSize(compID);
		ui32 bytesCount = ui32(list.size());

		for (ui32 i = 0; i < bytesCount; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(*this, comp->entity, &comp, dt);
		}
	}

	void Scene::LinearSystem(SV::SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt)
	{
		// for optimization, choose the sortest component list
		ui32 indexOfBestList = GetSortestComponentList(request, requestCount);
		CompID idOfBestList = request[indexOfBestList];

		// if one request is empty, exit
		auto& list = m_Components[idOfBestList];
		if (list.size() == 0) return;
		size_t sizeOfBestList = ECS::GetComponentSize(idOfBestList);

		// reserve memory for the pointers
		BaseComponent* components[SV_ECS_REQUEST_COMPONENTS_COUNT];

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
			for (j = 0; j < requestCount; ++j) {
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
			for (j = 0; j < optionalCount; ++j) {

				BaseComponent* comp = GetComponent(entityData, optional[j]);
				components[j + requestCount] = comp;
			}

			// if the entity is valid, call update
			system(*this, entity, components, dt);
		}
	}

	void Scene::UpdateMultithreadedSystem(const SV_ECS_SYSTEM_DESC& desc, float dt)
	{
		if (desc.requestedComponentsCount == 0) return;

		// system requisites
		CompID* request = desc.pRequestedComponents;
		CompID* optional = desc.pOptionalComponents;
		ui32 requestCount = desc.requestedComponentsCount;
		ui32 optionalCount = desc.optionalComponentsCount;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		if (requestCount == 1 && optionalCount == 0) {
			MultithreadedSystem_OneRequest(desc.system, request[0], dt);
		}
		else {
			MultithreadedSystem(desc.system, request, requestCount, optional, optionalCount, dt);
		}
	}

	void Scene::MultithreadedSystem_OneRequest(SV::SystemFunction system, CompID compID, float dt)
	{
		auto& list = m_Components[compID];
		size_t compSize = ECS::GetComponentSize(compID);

		SV::TaskFunction task[20];
		ui32 threadCount = Task::GetThreadCount();

		size_t count = list.size() / compSize;
		if (count < threadCount) threadCount = count;
		
		size_t size = (count / ui64(threadCount)) * compSize;

		for (ui32 i = 0; i < threadCount; ++i) {

			size_t currentSize = size;
			size_t offset = size * i;

			if (i + 1 == threadCount && count % 2 == 1) {
				currentSize += compSize;
			}

			task[i] = [this, system, compID, offset, currentSize, dt]() {
				PartialSystem_OneRequest(system, compID, offset, currentSize, dt);
			};
		}
		
		SV::ThreadContext ctx;
		Task::Execute(task, threadCount, &ctx);
		Task::Wait(ctx);
	}
	
	void Scene::PartialSystem_OneRequest(SV::SystemFunction system, CompID compID, size_t offset, size_t size, float dt)
	{
		std::vector<ui8>& list = m_Components[compID];
		ui64 compSize = ECS::GetComponentSize(compID);

		size_t finalIndex = offset + size;

		for (ui64 i = offset; i < finalIndex; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(*this, comp->entity, &comp, dt);
		}
	}

	void Scene::MultithreadedSystem(SV::SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt)
	{
		ui32 bestCompIndex = GetSortestComponentList(request, requestCount);
		CompID bestCompID = request[bestCompIndex];

		auto& list = m_Components[bestCompID];
		size_t compSize = ECS::GetComponentSize(bestCompID);

		SV::TaskFunction task[20];
		ui32 threadCount = Task::GetThreadCount();

		size_t count = list.size() / compSize;
		if (count < threadCount) threadCount = count;

		size_t size = (count / ui64(threadCount)) * compSize;

		for (ui32 i = 0; i < threadCount; ++i) {

			size_t currentSize = size;
			size_t offset = size * i;

			if (i + 1 == threadCount && count % 2 == 1) {
				currentSize += compSize;
			}

			task[i] = [this, system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt]() {
				PartialSystem(system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt);
			};
		}

		SV::ThreadContext ctx;
		Task::Execute(task, threadCount, &ctx);
		Task::Wait(ctx);
	}

	void Scene::PartialSystem(SV::SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt)
	{
		CompID bestCompID = request[bestCompIndex];
		size_t sizeOfBestComp = ECS::GetComponentSize(bestCompID);
		auto& bestCompList = m_Components[bestCompID];
		if (bestCompList.size() == 0) return;

		size_t finalSize = offset + size;

		BaseComponent* components[SV_ECS_REQUEST_COMPONENTS_COUNT];

		for (size_t i = offset; i < finalSize; i += sizeOfBestComp) {
			BaseComponent* bestComp = reinterpret_cast<BaseComponent*>(&bestCompList[i]);
			components[bestCompIndex] = bestComp;

			SV::EntityData& ed = m_EntityData[bestComp->entity];

			bool valid = true;

			// requested
			for (ui32 j = 0; j < requestCount; ++j) {
				if (j == bestCompIndex) continue;

				BaseComponent* comp = GetComponent(ed, request[j]);
				if (comp == nullptr) {
					valid = false;
					break;
				}

				components[j] = comp;
			}

			if (!valid) continue;

			// optional
			for (ui32 j = 0; j < optionalCount; ++j) {
				BaseComponent* comp = GetComponent(ed, optional[j]);
				components[j + requestCount] = comp;
			}

			// call
			system(*this, bestComp->entity, components, dt);
		}
	}

	ui32 Scene::GetSortestComponentList(CompID* compIDs, ui32 count)
	{
		ui32 index = 0;
		for (ui32 i = 1; i < count; ++i) {
			if (m_Components[compIDs[i]].size() < m_Components[index].size()) {
				index = i;
			}
		}
		return index;
	}

	// STATIC
	namespace ECS {

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
		void ConstructComponent(CompID ID, SV::BaseComponent* ptr, SV::Entity entity)
		{
			g_ComponentData[ID].createFn(ptr, entity);
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