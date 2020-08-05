#include "core.h"

#include "Scene.h"

using namespace sv;

///////////////////////////////////// COMPONENTS MANAGEMENT ////////////////////////////////////////

namespace _sv {

	struct ComponentData {
		const char* name;
		size_t size;
		CreateComponentFunction createFn;
		DestoryComponentFunction destroyFn;
		MoveComponentFunction moveFn;
		CopyComponentFunction copyFn;
	};

	static ComponentData	g_ComponentData[SV_ECS_MAX_COMPONENTS];
	static ui16				g_CompCount = 0u;

	CompID ecs_components_register(ui32 size, const std::type_info& typeInfo, CreateComponentFunction createFn, DestoryComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn)
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

namespace sv {

	using namespace _sv;

	ui16 ecs_components_get_count()
	{
		return g_CompCount;
	}
	size_t ecs_components_get_size(CompID ID)
	{
		return g_ComponentData[ID].size;
	}
	const char* ecs_components_get_name(CompID ID)
	{
		return g_ComponentData[ID].name;
	}
	std::map<std::string, CompID> g_ComponentNames;
	bool ecs_components_get_id(const char* name, CompID* id)
	{
		auto it = g_ComponentNames.find(name);
		if (it == g_ComponentNames.end()) {
			for (CompID i = 0; i < ecs_components_get_count(); ++i) {
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
	void ecs_components_create(CompID ID, BaseComponent* ptr, Entity entity)
	{
		g_ComponentData[ID].createFn(ptr, entity);
	}
	void ecs_components_destroy(CompID ID, BaseComponent* ptr)
	{
		g_ComponentData[ID].destroyFn(ptr);
	}

	void ecs_components_move(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_ComponentData[ID].moveFn(from, to);
	}

	void ecs_components_copy(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_ComponentData[ID].copyFn(from, to);
	}

}

///////////////////////////////////////// ECS //////////////////////////////////////////////////////

namespace _sv {

	static Scene_internal* g_pScene = nullptr;

	void scene_ecs_entitydata_index_add(EntityData& ed, CompID ID, size_t index)
	{
		ed.indices.push_back(std::make_pair(ID, index));
	}

	void scene_ecs_entitydata_index_set(EntityData& ed, CompID ID, size_t index)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				it->second = index;
				return;
			}
		}
	}

	bool scene_ecs_entitydata_index_get(EntityData& ed, CompID ID, size_t& index)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				index = it->second;
				return true;
			}
		}
		return false;
	}

	void scene_ecs_entitydata_index_remove(EntityData& ed, CompID ID)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				ed.indices.erase(it);
				return;
			}
		}
	}

	void scene_ecs_entitydata_reserve(ui32 count)
	{
		ui32 freeEntityDataCount = ui32(g_pScene->ecs.freeEntityData.size());

		if (freeEntityDataCount < count) {
			g_pScene->ecs.entityData.reserve(count - freeEntityDataCount);
		}
	}

	Entity scene_ecs_entity_new()
	{
		Entity entity;
		ECS& ecs = g_pScene->ecs;
		if (ecs.freeEntityData.empty()) {
			entity = Entity(ecs.entityData.size());
			ecs.entityData.emplace_back();
		}
		else {
			entity = ecs.freeEntityData.back();
			ecs.freeEntityData.pop_back();
		}
		ecs.entityData[entity].transform = {};
		return entity;
	}

	void scene_ecs_entitydata_update_childs(Entity entity, i32 count)
	{
		std::vector<Entity> parentsToUpdate;
		parentsToUpdate.emplace_back(entity);
		while (!parentsToUpdate.empty()) {

			Entity parentToUpdate = parentsToUpdate.back();
			parentsToUpdate.pop_back();
			EntityData& parentToUpdateEd = g_pScene->ecs.entityData[parentToUpdate];
			parentToUpdateEd.childsCount += count;

			if (parentToUpdateEd.parent != SV_INVALID_ENTITY) parentsToUpdate.emplace_back(parentToUpdateEd.parent);
		}
	}

	void scene_ecs_component_add(sv::Entity entity, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize)
	{
		auto& list = g_pScene->ecs.components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		ecs_components_move(componentID, comp, (BaseComponent*)(&list[index]));
		((BaseComponent*)& list[index])->entity = entity;

		// set index in entity
		scene_ecs_entitydata_index_add(g_pScene->ecs.entityData[entity], componentID, index);
	}

	void scene_ecs_components_add(sv::Entity* entities, ui32 count, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize)
	{
		auto& list = g_pScene->ecs.components[componentID];
		size_t index = list.size();

		// allocate the components
		list.resize(list.size() + componentSize * ui64(count));

		size_t currentIndex;
		Entity currentEntity;
		for (ui32 i = 0; i < count; ++i) {
			currentIndex = index + (i * componentSize);
			currentEntity = entities[i];

			if (comp) {
				ecs_components_copy(componentID, comp, (BaseComponent*)(&list[currentIndex]));
			}
			else {
				ecs_components_create(componentID, (BaseComponent*)(&list[currentIndex]), currentEntity);
			}

			// set entity in component
			BaseComponent* component = (BaseComponent*)(&list[currentIndex]);
			component->entity = currentEntity;
			// set index in entity
			scene_ecs_entitydata_index_add(g_pScene->ecs.entityData[currentEntity], componentID, currentIndex);
		}
	}

	std::vector<EntityData>& scene_ecs_get_entity_data()
	{
		return g_pScene->ecs.entityData;
	}

	std::vector<sv::Entity>& scene_ecs_get_entities()
	{
		return g_pScene->ecs.entities;
	}

	std::vector<ui8>& scene_ecs_get_components(sv::CompID ID)
	{
		return g_pScene->ecs.components[ID];
	}

}

namespace sv {

	using namespace _sv;

	Scene scene_create()
	{
		Scene_internal* scene = new Scene_internal();

		scene->ecs.components.reserve(ecs_components_get_count());
		for (CompID id = 0; id < ecs_components_get_count(); ++id) {
			scene->ecs.components.push_back(std::vector<ui8>());
		}

		scene->ecs.entityData.emplace_back();
		scene->ecs.entities.emplace_back(SV_INVALID_ENTITY);

		return scene;
	}

	void scene_destroy(Scene& scene_)
	{
		Scene_internal* scene = reinterpret_cast<Scene_internal*>(scene_);
		scene->physicsWorld; // TODO: Clear physics world

		delete scene;
		scene_ = nullptr;
	}

	void scene_clear(Scene scene_)
	{
		Scene_internal* scene = reinterpret_cast<Scene_internal*>(scene_);

		scene->mainCamera = SV_INVALID_ENTITY;
		scene->physicsWorld; // TODO: Clear physics world

		for (ui16 i = 0; i < ecs_components_get_count(); ++i) {
			scene->ecs.components[i].clear();
		}
		scene->ecs.entities.resize(1);
		scene->ecs.entityData.resize(1);
		scene->ecs.freeEntityData.clear();
	}

	Entity scene_camera_get()
	{
		return g_pScene->mainCamera;
	}

	void scene_camera_set(Entity entity)
	{
		g_pScene->mainCamera = entity;
	}

	void scene_bind(Scene scene)
	{
		g_pScene = reinterpret_cast<Scene_internal*>(scene);
	}
	
	Scene scene_get()
	{
		return g_pScene;
	}

	Entity scene_ecs_entity_create(Entity parent)
	{
		ECS& ecs = g_pScene->ecs;
		scene_ecs_entitydata_reserve(1u);
		Entity entity = scene_ecs_entity_new();

		if (parent == SV_INVALID_ENTITY) {
			ecs.entityData[entity].handleIndex = ecs.entities.size();
			ecs.entities.emplace_back(entity);
		}
		else {
			scene_ecs_entitydata_update_childs(parent, 1u);

			// Set parent and handleIndex
			EntityData& parentData = ecs.entityData[parent];
			size_t index = parentData.handleIndex + size_t(parentData.childsCount);

			EntityData& entityData = ecs.entityData[entity];
			entityData.handleIndex = index;
			entityData.parent = parent;

			// Special case, the parent and childs are in back of the list
			if (index == ecs.entities.size()) {
				ecs.entities.emplace_back(entity);
			}
			else {
				ecs.entities.insert(ecs.entities.begin() + index, entity);

				for (size_t i = index + 1; i < ecs.entities.size(); ++i) {
					ecs.entityData[ecs.entities[i]].handleIndex++;
				}
			}
		}

		return entity;
	}

	void scene_ecs_entity_destroy(Entity entity)
	{
		ECS& ecs = g_pScene->ecs;

		EntityData& entityData = ecs.entityData[entity];
		ui32 count = entityData.childsCount + 1;

		// notify parents
		if (entityData.parent != SV_INVALID_ENTITY) {
			scene_ecs_entitydata_update_childs(entityData.parent, -i32(count));
		}

		// data to remove entities
		size_t indexBeginDest = entityData.handleIndex;
		size_t indexBeginSrc = entityData.handleIndex + count;
		size_t cpyCant = ecs.entities.size() - indexBeginSrc;

		// remove components & entityData
		ecs.freeEntityData.reserve(count);
		for (size_t i = 0; i < count; i++) {
			Entity e = ecs.entities[indexBeginDest + i];
			EntityData& ed = ecs.entityData[e];
			scene_ecs_components_remove(e);
			ed.handleIndex = 0;
			ed.parent = SV_INVALID_ENTITY;
			ed.childsCount = 0u;
			ecs.freeEntityData.emplace_back(e);
		}

		// remove from entities & update indices
		if (cpyCant != 0) memcpy(&ecs.entities[indexBeginDest], &ecs.entities[indexBeginSrc], cpyCant * sizeof(Entity));
		ecs.entities.resize(ecs.entities.size() - count);
		for (size_t i = indexBeginDest; i < ecs.entities.size(); ++i) {
			ecs.entityData[ecs.entities[i]].handleIndex = i;
		}
	}

	Entity scene_ecs_entity_duplicate_recursive(Entity duplicated, Entity parent)
	{
		Entity copy;
		ECS& ecs = g_pScene->ecs;

		if (parent == SV_INVALID_ENTITY) copy = scene_ecs_entity_create();
		else copy = scene_ecs_entity_create(parent);

		EntityData& duplicatedEd = ecs.entityData[duplicated];
		EntityData& copyEd = ecs.entityData[copy];

		for (ui32 i = 0; i < duplicatedEd.indices.size(); ++i) {
			CompID ID = duplicatedEd.indices[i].first;
			size_t SIZE = ecs_components_get_size(ID);

			auto& list = ecs.components[ID];

			size_t index = list.size();
			list.resize(index + SIZE);

			BaseComponent* comp = scene_ecs_component_get_by_id(duplicated, ID);
			BaseComponent* newComp = reinterpret_cast<BaseComponent*>(&list[index]);
			ecs_components_copy(ID, comp, newComp);

			newComp->entity = copy;
			scene_ecs_entitydata_index_add(copyEd, ID, index);
		}

		copyEd.transform = duplicatedEd.transform;

		for (ui32 i = 0; i < ecs.entityData[duplicated].childsCount; ++i) {
			Entity toCopy = ecs.entities[ecs.entityData[duplicated].handleIndex + i + 1];
			scene_ecs_entity_duplicate_recursive(toCopy, copy);
			i += ecs.entityData[toCopy].childsCount;
		}

		return copy;
	}

	Entity scene_ecs_entity_duplicate(Entity duplicated)
	{
		return scene_ecs_entity_duplicate_recursive(duplicated, g_pScene->ecs.entityData[duplicated].parent);
	}

	bool scene_ecs_entity_is_empty(Entity entity)
	{
		return g_pScene->ecs.entityData[entity].indices.empty();
	}

	void scene_ecs_entity_get_childs(Entity parent, Entity const** childsArray, ui32* size)
	{
		const EntityData& ed = g_pScene->ecs.entityData[parent];
		*size = ed.childsCount;
		if (childsArray && ed.childsCount != 0)* childsArray = &g_pScene->ecs.entities[ed.handleIndex + 1];
	}

	Entity scene_ecs_entity_get_parent(Entity entity)
	{
		return g_pScene->ecs.entityData[entity].parent;
	}

	Transform scene_ecs_entity_get_transform(Entity entity)
	{
		return Transform(entity, &g_pScene->ecs.entityData[entity].transform);
	}

	void scene_ecs_entities_create(ui32 count, Entity parent, Entity* entities)
	{
		scene_ecs_entitydata_reserve(count);

		ECS& ecs = g_pScene->ecs;
		size_t entityIndex = 0u;

		if (parent == SV_INVALID_ENTITY) {

			entityIndex = ecs.entities.size();
			ecs.entities.reserve(count);

			// Create entities
			for (size_t i = 0; i < count; ++i) {
				Entity entity = scene_ecs_entity_new();
				ecs.entityData[entity].handleIndex = entityIndex + i;
				ecs.entities.emplace_back(entity);
			}
		}
		else {
			// Indices and reserve memory
			EntityData& parentData = ecs.entityData[parent];
			entityIndex = size_t(parentData.handleIndex) + size_t(parentData.childsCount) + 1u;

			size_t lastEntitiesSize = ecs.entities.size();
			ecs.entities.resize(lastEntitiesSize + count);

			// Update childs count
			scene_ecs_entitydata_update_childs(parent, count);

			// if the parent and sons aren't in back of the list
			if (entityIndex != lastEntitiesSize) {
				size_t newDataIndex = entityIndex + size_t(parentData.childsCount);
				// move old entities
				for (size_t i = ecs.entities.size() - 1; i >= newDataIndex; --i) {
					ecs.entities[i] = ecs.entities[i - size_t(count)];
					ecs.entityData[ecs.entities[i]].handleIndex = i;
				}
			}

			// creating entities
			for (size_t i = 0; i < count; ++i) {
				Entity entity = scene_ecs_entity_new();

				EntityData& entityData = ecs.entityData[entity];
				entityData.handleIndex = entityIndex + i;
				entityData.parent = parent;
				ecs.entities[entityData.handleIndex] = entity;
			}
		}

		// Allocate entities in user list
		if (entities) {
			for (size_t i = 0; i < count; ++i) {
				entities[i] = ecs.entities[entityIndex + i];
			}
		}
	}

	void scene_ecs_entities_destroy(Entity* entities, ui32 count)
	{
		// TODO:
	}

	void scene_ecs_component_add_by_id(sv::Entity entity, sv::CompID componentID)
	{
		size_t componentSize = ecs_components_get_size(componentID);

		auto& list = g_pScene->ecs.components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		BaseComponent* comp = (BaseComponent*)& list[index];
		ecs_components_create(componentID, comp, entity);

		// set index in entity
		scene_ecs_entitydata_index_add(g_pScene->ecs.entityData[entity], componentID, index);
	}

	sv::BaseComponent* scene_ecs_component_get_by_id(Entity e, CompID componentID)
	{
		size_t index;
		if (scene_ecs_entitydata_index_get(g_pScene->ecs.entityData[e], componentID, index)) {
			return (BaseComponent*)(&(g_pScene->ecs.components[componentID][index]));
		}
		return nullptr;
	}
	sv::BaseComponent* scene_ecs_component_get_by_id(EntityData& e, sv::CompID componentID)
	{
		size_t index;
		if (scene_ecs_entitydata_index_get(e, componentID, index)) {
			return (BaseComponent*)(&(g_pScene->ecs.components[componentID][index]));
		}
		return nullptr;
	}

	void scene_ecs_component_remove_by_id(sv::Entity entity, sv::CompID componentID)
	{
		ECS& ecs = g_pScene->ecs;
		size_t componentSize = ecs_components_get_size(componentID);
		EntityData& entityData = ecs.entityData[entity];

		// Get the index
		size_t index;
		if (!scene_ecs_entitydata_index_get(entityData, componentID, index)) return;

		// Remove index from index list
		scene_ecs_entitydata_index_remove(entityData, componentID);

		auto& list = ecs.components[componentID];

		ecs_components_destroy(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

		// if the component isn't the last element
		if (index != list.size() - componentSize) {
			// set back data in index
			memcpy(&list[index], &list[list.size() - componentSize], componentSize);

			Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
			scene_ecs_entitydata_index_set(ecs.entityData[otherEntity], componentID, index);
		}

		list.resize(list.size() - componentSize);
	}

	void scene_ecs_components_remove(Entity entity)
	{
		ECS& ecs = g_pScene->ecs;
		EntityData& entityData = ecs.entityData[entity];

		CompID componentID;
		size_t componentSize, index;
		for (ui32 i = 0; i < entityData.indices.size(); ++i) {
			componentID = entityData.indices[i].first;
			componentSize = ecs_components_get_size(componentID);
			index = entityData.indices[i].second;
			auto& list = ecs.components[componentID];

			ecs_components_destroy(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

			if (index != list.size() - componentSize) {
				// set back data in index
				memcpy(&list[index], &list[list.size() - componentSize], componentSize);

				Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
				scene_ecs_entitydata_index_set(ecs.entityData[otherEntity], componentID, index);
			}

			list.resize(list.size() - componentSize);
		}
		entityData.indices.clear();
	}

	void UpdateLinearSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);
	void LinearSystem_OneRequest(SystemFunction system, CompID compID, float dt);
	void LinearSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);
	void UpdateMultithreadedSystem(const SV_ECS_SYSTEM_DESC& desc, float dt);
	void MultithreadedSystem_OneRequest(SystemFunction system, CompID compID, float dt);
	void PartialSystem_OneRequest(SystemFunction system, CompID compID, size_t offset, size_t size, float dt);
	void MultithreadedSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt);
	void PartialSystem(SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt);
	ui32 GetSortestComponentList(CompID* compIDs, ui32 count);

	void scene_ecs_system_execute(const SV_ECS_SYSTEM_DESC* desc, ui32 count, float dt)
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
					task_execute([desc, i, dt]() {
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

			task_wait(ctx);
		}
	}

	void UpdateLinearSystem(const SV_ECS_SYSTEM_DESC& desc, float dt)
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

	void LinearSystem_OneRequest(SystemFunction system, CompID compID, float dt)
	{
		auto& list = g_pScene->ecs.components[compID];
		if (list.empty()) return;

		ui32 compSize = ecs_components_get_size(compID);
		ui32 bytesCount = ui32(list.size());

		for (ui32 i = 0; i < bytesCount; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(comp->entity, &comp, dt);
		}
	}

	void LinearSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt)
	{
		ECS& ecs = g_pScene->ecs;

		// for optimization, choose the sortest component list
		ui32 indexOfBestList = GetSortestComponentList(request, requestCount);
		CompID idOfBestList = request[indexOfBestList];

		// if one request is empty, exit
		auto& list = ecs.components[idOfBestList];
		if (list.size() == 0) return;
		size_t sizeOfBestList = ecs_components_get_size(idOfBestList);

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
			EntityData& entityData = ecs.entityData[entity];
			bool isValid = true;

			// allocate requested components
			ui32 j;
			for (j = 0; j < requestCount; ++j) {
				if (j == indexOfBestList) continue;

				BaseComponent* comp = scene_ecs_component_get_by_id(entityData, request[j]);
				if (!comp) {
					isValid = false;
					break;
				}
				components[j] = comp;
			}

			if (!isValid) continue;
			// allocate optional components
			for (j = 0; j < optionalCount; ++j) {

				BaseComponent* comp = scene_ecs_component_get_by_id(entityData, optional[j]);
				components[j + requestCount] = comp;
			}

			// if the entity is valid, call update
			system(entity, components, dt);
		}
	}

	void UpdateMultithreadedSystem(const SV_ECS_SYSTEM_DESC& desc, float dt)
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

	void MultithreadedSystem_OneRequest(SystemFunction system, CompID compID, float dt)
	{
		ECS& ecs = g_pScene->ecs;

		auto& list = ecs.components[compID];
		size_t compSize = ecs_components_get_size(compID);

		TaskFunction task[20];
		ui32 threadCount = task_thread_count();

		size_t count = list.size() / compSize;
		if (count < threadCount) threadCount = count;
		
		size_t size = (count / ui64(threadCount)) * compSize;

		for (ui32 i = 0; i < threadCount; ++i) {

			size_t currentSize = size;
			size_t offset = size * i;

			if (i + 1 == threadCount && count % 2 == 1) {
				currentSize += compSize;
			}

			task[i] = [system, compID, offset, currentSize, dt]() {
				PartialSystem_OneRequest(system, compID, offset, currentSize, dt);
			};
		}
		
		ThreadContext ctx;
		task_execute(task, threadCount, &ctx);
		task_wait(ctx);
	}
	
	void PartialSystem_OneRequest(SystemFunction system, CompID compID, size_t offset, size_t size, float dt)
	{
		std::vector<ui8>& list = g_pScene->ecs.components[compID];
		ui64 compSize = ecs_components_get_size(compID);

		size_t finalIndex = offset + size;

		for (ui64 i = offset; i < finalIndex; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(comp->entity, &comp, dt);
		}
	}

	void MultithreadedSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt)
	{
		ui32 bestCompIndex = GetSortestComponentList(request, requestCount);
		CompID bestCompID = request[bestCompIndex];

		auto& list = g_pScene->ecs.components[bestCompID];
		size_t compSize = ecs_components_get_size(bestCompID);

		TaskFunction task[20];
		ui32 threadCount = task_thread_count();

		size_t count = list.size() / compSize;
		if (count < threadCount) threadCount = count;

		size_t size = (count / ui64(threadCount)) * compSize;

		for (ui32 i = 0; i < threadCount; ++i) {

			size_t currentSize = size;
			size_t offset = size * i;

			if (i + 1 == threadCount && count % 2 == 1) {
				currentSize += compSize;
			}

			task[i] = [system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt]() {
				PartialSystem(system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt);
			};
		}

		ThreadContext ctx;
		task_execute(task, threadCount, &ctx);
		task_wait(ctx);
	}

	void PartialSystem(SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt)
	{
		ECS& ecs = g_pScene->ecs;

		CompID bestCompID = request[bestCompIndex];
		size_t sizeOfBestComp = ecs_components_get_size(bestCompID);
		auto& bestCompList = ecs.components[bestCompID];
		if (bestCompList.size() == 0) return;

		size_t finalSize = offset + size;

		BaseComponent* components[SV_ECS_REQUEST_COMPONENTS_COUNT];

		for (size_t i = offset; i < finalSize; i += sizeOfBestComp) {
			BaseComponent* bestComp = reinterpret_cast<BaseComponent*>(&bestCompList[i]);
			components[bestCompIndex] = bestComp;

			EntityData& ed = ecs.entityData[bestComp->entity];

			bool valid = true;

			// requested
			for (ui32 j = 0; j < requestCount; ++j) {
				if (j == bestCompIndex) continue;

				BaseComponent* comp = scene_ecs_component_get_by_id(ed, request[j]);
				if (comp == nullptr) {
					valid = false;
					break;
				}

				components[j] = comp;
			}

			if (!valid) continue;

			// optional
			for (ui32 j = 0; j < optionalCount; ++j) {
				BaseComponent* comp = scene_ecs_component_get_by_id(ed, optional[j]);
				components[j + requestCount] = comp;
			}

			// call
			system(bestComp->entity, components, dt);
		}
	}

	ui32 GetSortestComponentList(CompID* compIDs, ui32 count)
	{
		ui32 index = 0;
		for (ui32 i = 1; i < count; ++i) {
			if (g_pScene->ecs.components[compIDs[i]].size() < g_pScene->ecs.components[index].size()) {
				index = i;
			}
		}
		return index;
	}

}