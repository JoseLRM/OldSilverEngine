#include "core.h"

#include "entity_system.h"
#include "task_system.h"

namespace sv {

	///////////////////////////////////// COMPONENTS TYPES ////////////////////////////////////////

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

	///////////////////////////////////// ENTITIES ////////////////////////////////////////

	void ecs_entitydata_index_add(EntityData& ed, CompID ID, size_t index)
	{
		ed.indices.push_back(std::make_pair(ID, index));
	}

	void ecs_entitydata_index_set(EntityData& ed, CompID ID, size_t index)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				it->second = index;
				return;
			}
		}
	}

	bool ecs_entitydata_index_get(EntityData& ed, CompID ID, size_t& index)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				index = it->second;
				return true;
			}
		}
		return false;
	}

	void ecs_entitydata_index_remove(EntityData& ed, CompID ID)
	{
		for (auto it = ed.indices.begin(); it != ed.indices.end(); ++it) {
			if (it->first == ID) {
				ed.indices.erase(it);
				return;
			}
		}
	}

	void ecs_entitydata_reserve(ui32 count, ECS& ecs)
	{
		ui32 freeEntityDataCount = ui32(ecs.freeEntityData.size());

		if (freeEntityDataCount < count) {
			ecs.entityData.reserve(count - freeEntityDataCount);
		}
	}

	Entity ecs_entity_new(ECS& ecs)
	{
		Entity entity;
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

	void ecs_entitydata_update_childs(Entity entity, i32 count, ECS& ecs)
	{
		std::vector<Entity> parentsToUpdate;
		parentsToUpdate.emplace_back(entity);
		while (!parentsToUpdate.empty()) {

			Entity parentToUpdate = parentsToUpdate.back();
			parentsToUpdate.pop_back();
			EntityData& parentToUpdateEd = ecs.entityData[parentToUpdate];
			parentToUpdateEd.childsCount += count;

			if (parentToUpdateEd.parent != SV_ENTITY_NULL) parentsToUpdate.emplace_back(parentToUpdateEd.parent);
		}
	}

	void ecs_component_add(sv::Entity entity, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize, ECS& ecs)
	{
		auto& list = ecs.components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		ecs_components_move(componentID, comp, (BaseComponent*)(&list[index]));
		((BaseComponent*)& list[index])->entity = entity;

		// set index in entity
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, index);
	}

	void ecs_components_add(sv::Entity* entities, ui32 count, sv::BaseComponent* comp, sv::CompID componentID, size_t componentSize, ECS& ecs)
	{
		auto& list = ecs.components[componentID];
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
			ecs_entitydata_index_add(ecs.entityData[currentEntity], componentID, currentIndex);
		}
	}

	

	ECS ecs_create()
	{
		ECS ecs;

		ecs.components.reserve(ecs_components_get_count());
		for (CompID id = 0; id < ecs_components_get_count(); ++id) {
			ecs.components.push_back(std::vector<ui8>());
		}

		ecs.entityData.emplace_back();
		ecs.entities.emplace_back(SV_ENTITY_NULL);

		return ecs;
	}

	void ecs_destroy(ECS& ecs)
	{
		ecs.components.clear();
		ecs.entities.clear();
		ecs.entityData.clear();
		ecs.freeEntityData.clear();
	}

	void ecs_clear(ECS& ecs)
	{
		for (ui16 i = 0; i < ecs_components_get_count(); ++i) {
			ecs.components[i].clear();
		}
		ecs.entities.resize(1);
		ecs.entityData.resize(1);
		ecs.freeEntityData.clear();
	}

	Entity ecs_entity_create(Entity parent, ECS& ecs)
	{
		ecs_entitydata_reserve(1u, ecs);
		Entity entity = ecs_entity_new(ecs);

		if (parent == SV_ENTITY_NULL) {
			ecs.entityData[entity].handleIndex = ecs.entities.size();
			ecs.entities.emplace_back(entity);
		}
		else {
			ecs_entitydata_update_childs(parent, 1u, ecs);

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

	void ecs_entity_destroy(Entity entity, ECS& ecs)
	{
		EntityData& entityData = ecs.entityData[entity];
		ui32 count = entityData.childsCount + 1;

		// notify parents
		if (entityData.parent != SV_ENTITY_NULL) {
			ecs_entitydata_update_childs(entityData.parent, -i32(count), ecs);
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
			ecs_components_remove(e, ecs);
			ed.handleIndex = 0;
			ed.parent = SV_ENTITY_NULL;
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

	Entity ecs_entity_duplicate_recursive(Entity duplicated, Entity parent, ECS& ecs)
	{
		Entity copy;

		if (parent == SV_ENTITY_NULL) copy = ecs_entity_create(SV_ENTITY_NULL, ecs);
		else copy = ecs_entity_create(parent, ecs);

		EntityData& duplicatedEd = ecs.entityData[duplicated];
		EntityData& copyEd = ecs.entityData[copy];

		for (ui32 i = 0; i < duplicatedEd.indices.size(); ++i) {
			CompID ID = duplicatedEd.indices[i].first;
			size_t SIZE = ecs_components_get_size(ID);

			auto& list = ecs.components[ID];

			size_t index = list.size();
			list.resize(index + SIZE);

			BaseComponent* comp = ecs_component_get_by_id(duplicated, ID, ecs);
			BaseComponent* newComp = reinterpret_cast<BaseComponent*>(&list[index]);
			ecs_components_copy(ID, comp, newComp);

			newComp->entity = copy;
			ecs_entitydata_index_add(copyEd, ID, index);
		}

		copyEd.transform = duplicatedEd.transform;

		for (ui32 i = 0; i < ecs.entityData[duplicated].childsCount; ++i) {
			Entity toCopy = ecs.entities[ecs.entityData[duplicated].handleIndex + i + 1];
			ecs_entity_duplicate_recursive(toCopy, copy, ecs);
			i += ecs.entityData[toCopy].childsCount;
		}

		return copy;
	}

	Entity ecs_entity_duplicate(Entity duplicated, ECS& ecs)
	{
		return ecs_entity_duplicate_recursive(duplicated, ecs.entityData[duplicated].parent, ecs);
	}

	bool ecs_entity_is_empty(Entity entity, ECS& ecs)
	{
		return ecs.entityData[entity].indices.empty();
	}

	void ecs_entity_get_childs(Entity parent, Entity const** childsArray, ui32* size, ECS& ecs)
	{
		const EntityData& ed = ecs.entityData[parent];
		*size = ed.childsCount;
		if (childsArray && ed.childsCount != 0)* childsArray = &ecs.entities[ed.handleIndex + 1];
	}

	Entity ecs_entity_get_parent(Entity entity, ECS& ecs)
	{
		return ecs.entityData[entity].parent;
	}

	Transform ecs_entity_get_transform(Entity entity, ECS& ecs)
	{
		return Transform(entity, &ecs.entityData[entity].transform, &ecs);
	}

	void ecs_entities_create(ui32 count, Entity parent, Entity* entities, ECS& ecs)
	{
		ecs_entitydata_reserve(count, ecs);

		size_t entityIndex = 0u;

		if (parent == SV_ENTITY_NULL) {

			entityIndex = ecs.entities.size();
			ecs.entities.reserve(count);

			// Create entities
			for (size_t i = 0; i < count; ++i) {
				Entity entity = ecs_entity_new(ecs);
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
			ecs_entitydata_update_childs(parent, count, ecs);

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
				Entity entity = ecs_entity_new(ecs);

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

	void ecs_entities_destroy(Entity* entities, ui32 count, ECS& ecs)
	{
		log_error("TODO->ecs_entities_destroy");
	}

	void ecs_component_add_by_id(sv::Entity entity, sv::CompID componentID, ECS& ecs)
	{
		size_t componentSize = ecs_components_get_size(componentID);

		auto& list = ecs.components[componentID];
		size_t index = list.size();

		// allocate the component
		list.resize(list.size() + componentSize);
		BaseComponent* comp = (BaseComponent*)& list[index];
		ecs_components_create(componentID, comp, entity);

		// set index in entity
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, index);
	}

	sv::BaseComponent* ecs_component_get_by_id(Entity e, CompID componentID, ECS& ecs)
	{
		size_t index;
		if (ecs_entitydata_index_get(ecs.entityData[e], componentID, index)) {
			return (BaseComponent*)(&(ecs.components[componentID][index]));
		}
		return nullptr;
	}
	sv::BaseComponent* ecs_component_get_by_id(EntityData& e, sv::CompID componentID, ECS& ecs)
	{
		size_t index;
		if (ecs_entitydata_index_get(e, componentID, index)) {
			return (BaseComponent*)(&(ecs.components[componentID][index]));
		}
		return nullptr;
	}

	void ecs_component_remove_by_id(sv::Entity entity, sv::CompID componentID, ECS& ecs)
	{
		size_t componentSize = ecs_components_get_size(componentID);
		EntityData& entityData = ecs.entityData[entity];

		// Get the index
		size_t index;
		if (!ecs_entitydata_index_get(entityData, componentID, index)) return;

		// Remove index from index list
		ecs_entitydata_index_remove(entityData, componentID);

		auto& list = ecs.components[componentID];

		ecs_components_destroy(componentID, reinterpret_cast<BaseComponent*>(&list[index]));

		// if the component isn't the last element
		if (index != list.size() - componentSize) {
			// set back data in index
			memcpy(&list[index], &list[list.size() - componentSize], componentSize);

			Entity otherEntity = ((BaseComponent*)(&list[index]))->entity;
			ecs_entitydata_index_set(ecs.entityData[otherEntity], componentID, index);
		}

		list.resize(list.size() - componentSize);
	}

	void ecs_components_remove(Entity entity, ECS& ecs)
	{
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
				ecs_entitydata_index_set(ecs.entityData[otherEntity], componentID, index);
			}

			list.resize(list.size() - componentSize);
		}
		entityData.indices.clear();
	}

	void UpdateLinearSystem(const SystemDesc& desc, float dt, ECS& ecs);
	void LinearSystem_OneRequest(SystemFunction system, CompID compID, float dt, ECS& ecs);
	void LinearSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt, ECS& ecs);
	void UpdateMultithreadedSystem(const SystemDesc& desc, float dt, ECS& ecs);
	void MultithreadedSystem_OneRequest(SystemFunction system, CompID compID, float dt, ECS& ecs);
	void PartialSystem_OneRequest(SystemFunction system, CompID compID, size_t offset, size_t size, float dt, ECS& ecs);
	void MultithreadedSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt, ECS& ecs);
	void PartialSystem(SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt, ECS& ecs);
	ui32 GetSortestComponentList(CompID* compIDs, ui32 count, ECS& ecs);

	void ecs_system_execute(const SystemDesc* desc, ui32 count, float dt, ECS& ecs)
	{
		if (count == 0) return;

		if (count == 1) {
			if (desc[0].executionMode == SystemExecutionMode_Multithreaded) {
				UpdateMultithreadedSystem(desc[0], dt, ecs);
			}
			else {
				UpdateLinearSystem(*desc, dt, ecs);
			}
		}
		else {

			ThreadContext ctx;

			for (ui32 i = 0; i < count; ++i) {
				if (desc[i].executionMode == SystemExecutionMode_Parallel) {
					task_execute([desc, i, dt, &ecs]() {
						UpdateLinearSystem(desc[i], dt, ecs);
					}, &ctx);
				}
			}

			for (ui32 i = 0; i < count; ++i) {

				if (desc[i].executionMode == SystemExecutionMode_Multithreaded) {
					UpdateMultithreadedSystem(desc[i], dt, ecs);
				}
				else if (desc[i].executionMode == SystemExecutionMode_Safe) {
					UpdateLinearSystem(desc[i], dt, ecs);
				}

			}

			task_wait(ctx);
		}
	}

	void UpdateLinearSystem(const SystemDesc& desc, float dt, ECS& ecs)
	{
		if (desc.requestedComponentsCount == 0) return;

		// system requisites
		CompID* request = desc.pRequestedComponents;
		CompID* optional = desc.pOptionalComponents;
		ui32 requestCount = desc.requestedComponentsCount;
		ui32 optionalCount = desc.optionalComponentsCount;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		if (optionalCount == 0 && requestCount == 1) {
			LinearSystem_OneRequest(desc.system, request[0], dt, ecs);
		}
		else {
			LinearSystem(desc.system, request, requestCount, optional, optionalCount, dt, ecs);
		}
	}

	void LinearSystem_OneRequest(SystemFunction system, CompID compID, float dt, ECS& ecs)
	{
		auto& list = ecs.components[compID];
		if (list.empty()) return;

		ui32 compSize = ecs_components_get_size(compID);
		ui32 bytesCount = ui32(list.size());

		for (ui32 i = 0; i < bytesCount; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(comp->entity, &comp, ecs, dt);
		}
	}

	void LinearSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt, ECS& ecs)
	{
		// for optimization, choose the sortest component list
		ui32 indexOfBestList = GetSortestComponentList(request, requestCount, ecs);
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

				BaseComponent* comp = ecs_component_get_by_id(entityData, request[j], ecs);
				if (!comp) {
					isValid = false;
					break;
				}
				components[j] = comp;
			}

			if (!isValid) continue;
			// allocate optional components
			for (j = 0; j < optionalCount; ++j) {

				BaseComponent* comp = ecs_component_get_by_id(entityData, optional[j], ecs);
				components[j + requestCount] = comp;
			}

			// if the entity is valid, call update
			system(entity, components, ecs, dt);
		}
	}

	void UpdateMultithreadedSystem(const SystemDesc& desc, float dt, ECS& ecs)
	{
		if (desc.requestedComponentsCount == 0) return;

		// system requisites
		CompID* request = desc.pRequestedComponents;
		CompID* optional = desc.pOptionalComponents;
		ui32 requestCount = desc.requestedComponentsCount;
		ui32 optionalCount = desc.optionalComponentsCount;

		// Different algorithm if there are only one request and no optionals (optimization reason)
		if (requestCount == 1 && optionalCount == 0) {
			MultithreadedSystem_OneRequest(desc.system, request[0], dt, ecs);
		}
		else {
			MultithreadedSystem(desc.system, request, requestCount, optional, optionalCount, dt, ecs);
		}
	}

	void MultithreadedSystem_OneRequest(SystemFunction system, CompID compID, float dt, ECS& ecs)
	{
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

			task[i] = [system, compID, offset, currentSize, dt, &ecs]() {
				PartialSystem_OneRequest(system, compID, offset, currentSize, dt, ecs);
			};
		}

		ThreadContext ctx;
		task_execute(task, threadCount, &ctx);
		task_wait(ctx);
	}

	void PartialSystem_OneRequest(SystemFunction system, CompID compID, size_t offset, size_t size, float dt, ECS& ecs)
	{
		std::vector<ui8>& list = ecs.components[compID];
		ui64 compSize = ecs_components_get_size(compID);

		size_t finalIndex = offset + size;

		for (ui64 i = offset; i < finalIndex; i += compSize) {
			BaseComponent* comp = reinterpret_cast<BaseComponent*>(&list[i]);
			system(comp->entity, &comp, ecs, dt);
		}
	}

	void MultithreadedSystem(SystemFunction system, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, float dt, ECS& ecs)
	{
		ui32 bestCompIndex = GetSortestComponentList(request, requestCount, ecs);
		CompID bestCompID = request[bestCompIndex];

		auto& list = ecs.components[bestCompID];
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

			task[i] = [system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt, &ecs]() {
				PartialSystem(system, bestCompIndex, request, requestCount, optional, optionalCount, offset, currentSize, dt, ecs);
			};
		}

		ThreadContext ctx;
		task_execute(task, threadCount, &ctx);
		task_wait(ctx);
	}

	void PartialSystem(SystemFunction system, ui32 bestCompIndex, CompID* request, ui32 requestCount, CompID* optional, ui32 optionalCount, size_t offset, size_t size, float dt, ECS& ecs)
	{
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

				BaseComponent* comp = ecs_component_get_by_id(ed, request[j], ecs);
				if (comp == nullptr) {
					valid = false;
					break;
				}

				components[j] = comp;
			}

			if (!valid) continue;

			// optional
			for (ui32 j = 0; j < optionalCount; ++j) {
				BaseComponent* comp = ecs_component_get_by_id(ed, optional[j], ecs);
				components[j + requestCount] = comp;
			}

			// call
			system(bestComp->entity, components, ecs, dt);
		}
	}

	ui32 GetSortestComponentList(CompID* compIDs, ui32 count, ECS& ecs)
	{
		ui32 index = 0;
		for (ui32 i = 1; i < count; ++i) {
			if (ecs.components[compIDs[i]].size() < ecs.components[index].size()) {
				index = i;
			}
		}
		return index;
	}

}