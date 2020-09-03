#include "core.h"

#include "entity_system_internal.h"
#include "task_system.h"

namespace sv {

#define parseECS() ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(ecs_)

	ECS* ecs_create()
	{
		ECS_internal& ecs = *new ECS_internal();

		ecs.components.resize(ecs_register_count());
		for (CompID i = 0u; i < ecs.components.size(); ++i) {
			ecs.components[i].create(i);
		}

		ecs.entities.emplace_back(SV_ENTITY_NULL);

		return &ecs;
	}

	void ecs_destroy(ECS* ecs_)
	{
		if (ecs_ == nullptr) return;
		parseECS();

		ecs.components.clear();
		ecs.entities.clear();

		delete &ecs;
	}

	void ecs_clear(ECS* ecs_)
	{
		parseECS();

		for (ui16 i = 0; i < ecs_register_count(); ++i) {
			ecs.components[i].destroy();
		}
		ecs.entities.resize(1);
		ecs.entityData.clear();
	}

	///////////////////////////////////// COMPONENTS REGISTER ////////////////////////////////////////

	struct ComponentData {
		const char* name;
		ui32 size;
		CreateComponentFunction createFn;
		DestroyComponentFunction destroyFn;
		MoveComponentFunction moveFn;
		CopyComponentFunction copyFn;
	};

	static std::vector<ComponentData> g_ComponentData;
	static std::mutex g_ComponentRegisterMutex;

	CompID ecs_register(ui32 size, const char* name, CreateComponentFunction createFn, DestroyComponentFunction destroyFn, MoveComponentFunction moveFn, CopyComponentFunction copyFn)
	{
		g_ComponentRegisterMutex.lock();
		CompID ID = g_ComponentData.size();
		ComponentData& data = g_ComponentData.emplace_back();
		g_ComponentRegisterMutex.unlock();

		data.size = size;
		data.name = name;
		data.createFn = createFn;
		data.destroyFn = destroyFn;
		data.moveFn = moveFn;
		data.copyFn = copyFn;

		return ID;
	}

	ui32 ecs_register_count()
	{
		return ui32(g_ComponentData.size());
	}
	ui32 ecs_register_sizeof(CompID ID)
	{
		return g_ComponentData[ID].size;
	}
	const char* ecs_register_nameof(CompID ID)
	{
		return g_ComponentData[ID].name;
	}
	
	void ecs_register_create(CompID ID, BaseComponent* ptr, Entity entity)
	{
		g_ComponentData[ID].createFn(ptr, entity);
	}
	void ecs_register_destroy(CompID ID, BaseComponent* ptr)
	{
		g_ComponentData[ID].destroyFn(ptr);
		ptr->entity = SV_ENTITY_NULL;
	}

	void ecs_register_move(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_ComponentData[ID].moveFn(from, to);
	}

	void ecs_register_copy(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_ComponentData[ID].copyFn(from, to);
	}

	///////////////////////////////////// HELPER FUNCTIONS ////////////////////////////////////////

	void ecs_entitydata_index_add(EntityData& ed, CompID ID, BaseComponent* compPtr)
	{
		ed.components.push_back(std::make_pair(ID, compPtr));
	}

	void ecs_entitydata_index_set(EntityData& ed, CompID ID, BaseComponent* compPtr)
	{
		for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
			if (it->first == ID) {
				it->second = compPtr;
				return;
			}
		}
	}

	BaseComponent* ecs_entitydata_index_get(EntityData& ed, CompID ID)
	{
		for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
			if (it->first == ID) {
				return it->second;
			}
		}
		return nullptr;
	}

	void ecs_entitydata_index_remove(EntityData& ed, CompID ID)
	{
		for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
			if (it->first == ID) {
				ed.components.erase(it);
				return;
			}
		}
	}

	void ecs_entitydata_update_childs(ECS_internal& ecs, Entity entity, i32 count)
	{
		Entity parent = entity;

		while (parent != SV_ENTITY_NULL) {
			EntityData& parentToUpdate = ecs.entityData[parent];
			parentToUpdate.childsCount += count;
			parent = parentToUpdate.parent;
		}
	}

	///////////////////////////////////// ENTITIES ////////////////////////////////////////

	Entity ecs_entity_create(ECS* ecs_, Entity parent)
	{
		parseECS();

		Entity entity = ecs.entityData.add();

		if (parent == SV_ENTITY_NULL) {
			ecs.entityData[entity].handleIndex = ecs.entities.size();
			ecs.entities.emplace_back(entity);
		}
		else {
			ecs_entitydata_update_childs(ecs, parent, 1u);

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

	void ecs_entity_destroy(ECS* ecs_, Entity entity)
	{
		parseECS();

		EntityData& entityData = ecs.entityData[entity];
		ui32 count = entityData.childsCount + 1;

		// notify parents
		if (entityData.parent != SV_ENTITY_NULL) {
			ecs_entitydata_update_childs(ecs, entityData.parent, -i32(count));
		}

		// data to remove entities
		size_t indexBeginDest = entityData.handleIndex;
		size_t indexBeginSrc = entityData.handleIndex + count;
		size_t cpyCant = ecs.entities.size() - indexBeginSrc;

		// remove components & entityData
		for (size_t i = 0; i < count; i++) {
			Entity e = ecs.entities[indexBeginDest + i];
			EntityData& ed = ecs.entityData[e];
			ecs_entity_clear(ecs_, entity);
			ecs.entityData.remove(entity);
		}

		// remove from entities & update indices
		if (cpyCant != 0) memcpy(&ecs.entities[indexBeginDest], &ecs.entities[indexBeginSrc], cpyCant * sizeof(Entity));
		ecs.entities.resize(ecs.entities.size() - count);
		for (size_t i = indexBeginDest; i < ecs.entities.size(); ++i) {
			ecs.entityData[ecs.entities[i]].handleIndex = i;
		}
	}

	void ecs_entity_clear(ECS* ecs_, Entity entity)
	{
		parseECS();

		EntityData& ed = ecs.entityData[entity];
		while (!ed.components.empty()) {

			auto [compID, comp] = ed.components.back();
			ed.components.pop_back();

			ecs.components[compID].free_component(comp);
			
		}
	}

	Entity ecs_entity_duplicate_recursive(ECS_internal& ecs, Entity duplicated, Entity parent)
	{
		Entity copy;

		if (parent == SV_ENTITY_NULL) copy = ecs_entity_create(&ecs);
		else copy = ecs_entity_create(&ecs, parent);

		EntityData& duplicatedEd = ecs.entityData[duplicated];
		EntityData& copyEd = ecs.entityData[copy];

		for (ui32 i = 0; i < duplicatedEd.components.size(); ++i) {
			CompID ID = duplicatedEd.components[i].first;
			size_t SIZE = ecs_register_sizeof(ID);

			BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
			BaseComponent* newComp = ecs.components[ID].alloc_component(comp);

			newComp->entity = copy;
			ecs_entitydata_index_add(copyEd, ID, newComp);
		}

		copyEd.transform = duplicatedEd.transform;

		for (ui32 i = 0; i < ecs.entityData[duplicated].childsCount; ++i) {
			Entity toCopy = ecs.entities[ecs.entityData[duplicated].handleIndex + i + 1];
			ecs_entity_duplicate_recursive(ecs, toCopy, copy);
			i += ecs.entityData[toCopy].childsCount;
		}

		return copy;
	}

	Entity ecs_entity_duplicate(ECS* ecs_, Entity entity)
	{
		parseECS();

		Entity res = ecs_entity_duplicate_recursive(ecs, entity, ecs.entityData[entity].parent);
		return res;
	}

	bool ecs_entity_is_empty(ECS* ecs_, Entity entity)
	{
		parseECS();
		return ecs.entityData[entity].components.empty();
	}

	bool ecs_entity_exist(ECS* ecs_, Entity entity)
	{
		parseECS();
		if (entity == SV_ENTITY_NULL || entity >= ecs.entityData.size()) return false;

		EntityData& ed = ecs.entityData[entity];
		return ed.handleIndex == 0u;
	}

	ui32 ecs_entity_childs_count(ECS* ecs_, Entity parent)
	{
		parseECS();
		return ecs.entityData[parent].childsCount;
	}

	void ecs_entity_childs_get(ECS* ecs_, Entity parent, Entity const** childsArray)
	{
		parseECS();

		const EntityData& ed = ecs.entityData[parent];
		if (childsArray && ed.childsCount != 0) *childsArray = &ecs.entities[ed.handleIndex + 1];
	}

	Entity ecs_entity_parent_get(ECS* ecs_, Entity entity)
	{
		parseECS();
		return ecs.entityData[entity].parent;
	}

	Transform ecs_entity_transform_get(ECS* ecs_, Entity entity)
	{
		parseECS();
		return Transform(entity, &ecs.entityData[entity].transform, &ecs);
	}

	ui32 ecs_entity_component_count(ECS* ecs_, Entity entity)
	{
		parseECS();
		return ui32(ecs.entityData[entity].components.size());
	}

	void ecs_entities_create(ECS* ecs_, ui32 count, Entity parent, Entity* entities)
	{
		parseECS();

		size_t entityIndex = 0u;

		if (parent == SV_ENTITY_NULL) {

			entityIndex = ecs.entities.size();
			ecs.entities.reserve(count);

			// Create entities
			for (size_t i = 0; i < count; ++i) {
				Entity entity = ecs.entityData.add();
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
			ecs_entitydata_update_childs(ecs, parent, count);

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
				Entity entity = ecs.entityData.add();

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

	void ecs_entities_destroy(ECS* ecs, Entity const* entities, ui32 count)
	{
		log_error("TODO->ecs_entities_destroy");
	}

	ui32 ecs_entity_count(ECS* ecs_)
	{
		parseECS();
		return ui32(ecs.entities.size()) - 1u;
	}

	Entity ecs_entity_get(ECS* ecs_, ui32 index)
	{
		parseECS();
		return ecs.entities[index + 1u];
	}

	/////////////////////////////// COMPONENTS /////////////////////////////////////

	BaseComponent* ecs_component_add(ECS* ecs_, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize)
	{
		parseECS();

		comp = ecs.components[componentID].alloc_component(comp);
		comp->entity = entity;
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, comp);
		return comp;
	}

	BaseComponent* ecs_component_add_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		BaseComponent* comp = ecs.components[componentID].alloc_component(entity);
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, comp);
		return comp;
	}

	BaseComponent* ecs_component_get_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		return ecs_entitydata_index_get(ecs.entityData[entity], componentID);
	}

	std::pair<CompID, BaseComponent*> ecs_component_get_by_index(ECS* ecs_, Entity entity, ui32 index)
	{
		parseECS();
		return ecs.entityData[entity].components[index];
	}

	void ecs_component_remove_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		EntityData& ed = ecs.entityData[entity];
		BaseComponent* comp = ecs_entitydata_index_get(ed, componentID);
		ecs.components[componentID].free_component(comp);
		ecs_entitydata_index_remove(ed, componentID);
	}

	ui32 ecs_component_count(ECS* ecs_, CompID ID)
	{
		parseECS();
		return ecs.components[ID].size();
	}

	// Iterators

	ComponentIterator::ComponentIterator(ECS* ecs_, CompID compID, bool end) : ecs_(ecs_), compID(compID), pool(0u)
	{
		parseECS();

		if (end) start_end();
		else start_begin();
	}

	BaseComponent* ComponentIterator::get_ptr()
	{
		return it;
	}

	bool ComponentIterator::equal(const ComponentIterator& other) const noexcept
	{
		return it == other.it;
	}

	void ComponentIterator::next()
	{
		parseECS();
		auto& list = ecs.components[compID];

		size_t compSize = size_t(ecs_register_sizeof(compID));
		ui8* ptr = reinterpret_cast<ui8*>(it);
		ui8* endPtr = list.get_pool(pool).get() + list.get_pool(pool).byte_size();

		do {
			ptr += compSize;
			if (ptr == endPtr) {
				auto& list = ecs.components[compID];

				do {

					if (++pool == list.get_pool_count()) break;

				} while (list.get_pool(pool).byte_size() == 0u);

				if (pool == list.get_pool_count()) break;
				ComponentPool& compPool = list.get_pool(pool);

				ptr = compPool.get();
				endPtr = ptr + compPool.byte_size();
			}
		} while (reinterpret_cast<BaseComponent*>(ptr)->entity == SV_ENTITY_NULL);

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

	void ComponentIterator::last()
	{
		parseECS();
		auto& list = ecs.components[compID];

		size_t compSize = size_t(ecs_register_sizeof(compID));
		ui8* ptr = reinterpret_cast<ui8*>(it);
		ui8* beginPtr = list.get_pool(pool).get();

		do {
			ptr -= compSize;
			if (ptr == beginPtr) {
				
				while (list.get_pool(--pool).byte_size() == 0u);

				ComponentPool& compPool = list.get_pool(pool);

				beginPtr = compPool.get();
				ptr = beginPtr + compPool.byte_size();
			}
		} while (reinterpret_cast<BaseComponent*>(ptr)->entity == SV_ENTITY_NULL);

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

	void ComponentIterator::start_begin()
	{
		parseECS();

		auto& list = ecs.components[compID];

		pool = 0u;
		ui8* ptr = nullptr;

		if (!list.empty()) {

			ui32 compSize = ecs_register_sizeof(compID);

			while (list.get_pool(pool).size(compSize) == 0u) pool++;

			ptr = list.get_pool(pool).get();

			while (true) {
				BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
				if (comp->entity != SV_ENTITY_NULL) {
					break;
				}
				ptr += compSize;
			}

		}

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

	void ComponentIterator::start_end()
	{
		parseECS();

		auto& list = ecs.components[compID];

		pool = list.get_pool_count() - 1u;
		ui8* ptr = nullptr;
		ui32 compSize = ecs_register_sizeof(compID);

		if (!list.empty()) {

			while (list.get_pool(pool).size(compSize) == 0u) pool--;

			ptr = list.get_pool(pool).get() + list.get_pool(pool).byte_size() - compSize;

			while (true) {
				BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
				if (comp->entity != SV_ENTITY_NULL) {
					break;
				}
				ptr -= compSize;
			}

		}

		if (ptr == nullptr) it = nullptr;
		else it = reinterpret_cast<BaseComponent*>(ptr + compSize);
	}

}