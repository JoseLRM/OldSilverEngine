#include "SilverEngine/core.h"

#include "entity_system_internal.h"
#include "SilverEngine/task_system.h"

namespace sv {

	static std::vector<ComponentRegister>			g_Registers;
	static std::unordered_map<std::string, CompID>	g_CompNames;

	void ecs_create(ECS** ecs_)
	{
		ECS_internal& ecs = *new ECS_internal();

		// Allocate components
		ecs.components.resize(g_Registers.size());

		for (CompID id = 0u; id < g_Registers.size(); ++id) {

			componentAllocatorCreate(reinterpret_cast<ECS*>(&ecs), id);
		}

		*ecs_ = reinterpret_cast<ECS*>(&ecs);
	}

	void ecs_destroy(ECS* ecs_)
	{
		if (ecs_ == nullptr) return;
		parseECS();

		for (CompID i = 0; i < ecs_component_register_count(); ++i) {
			if (ecs_component_exist(i))
				componentAllocatorDestroy(ecs_, i);
		}
		ecs.components.clear();
		ecs.entities.clear();
		entityClear(ecs.entityData);

		// Deallocate
		delete& ecs;
	}

	void ecs_clear(ECS* ecs_)
	{
		parseECS();

		for (CompID i = 0; i < ecs_component_register_count(); ++i) {
			if (ecs_component_exist(i))
				componentAllocatorDestroy(ecs_, i);
		}
		ecs.entities.clear();
		entityClear(ecs.entityData);
	}

	Result ecs_serialize(ECS* ecs_, ArchiveO& archive)
	{
		parseECS();

		// Registers
		{
			u32 registersCount = 0u;
			for (CompID id = 0u; id < g_Registers.size(); ++id) {
				if (ecs_component_exist(id))
					++registersCount;
			}
			archive << registersCount;

			for (CompID id = 0u; id < g_Registers.size(); ++id) {

				if (ecs_component_exist(id))
					archive << ecs_component_name(id) << ecs_component_size(id);

			}
		}

		// Entity data
		{
			u32 entityCount = u32(ecs.entities.size());
			u32 entityDataCount = u32(ecs.entityData.size);

			archive << entityCount << entityDataCount;

			for (u32 i = 0; i < entityDataCount; ++i) {

				Entity entity = i + 1u;
				EntityData& ed = ecs.entityData[entity];

				if (ed.handleIndex != u64_max) {

					EntityTransform& transform = ecs.entityData.get_transform(entity);

					archive << i << ed.childsCount << ed.handleIndex << transform.localPosition;
					archive << transform.localRotation << transform.localScale;
				}
			}
		}

		// Components
		{
			for (CompID compID = 0u; compID < g_Registers.size(); ++compID) {

				if (!ecs_component_exist(compID)) continue;

				u32 compSize = ecs_component_size(compID);

				archive << componentAllocatorCount(ecs_, compID);

				ComponentIterator it(ecs_, compID, false);
				ComponentIterator end(ecs_, compID, true);

				while (!it.equal(end)) {
					BaseComponent* component = it.get_ptr();
					archive << component->entity;
					ecs_component_serialize(compID, component, archive);

					it.next();
				}

			}
		}

		return Result_Success;
	}

	Result ecs_deserialize(ECS* ecs_, ArchiveI& archive)
	{
		parseECS();

		ecs.entities.clear();
		entityClear(ecs.entityData);

		struct Register {
			std::string name;
			u32 size;
			CompID ID;
		};

		// Registers
		std::vector<Register> registers;

		{
			u32 registersCount;
			archive >> registersCount;
			registers.resize(registersCount);

			for (auto it = registers.rbegin(); it != registers.rend(); ++it) {

				archive >> it->name;
				archive >> it->size;

			}
		}

		// Registers ID
		CompID invalidCompID = CompID(g_Registers.size());
		for (u32 i = 0; i < registers.size(); ++i) {

			Register& reg = registers[i];
			reg.ID = invalidCompID;

			auto it = g_CompNames.find(reg.name.c_str());
			if (it != g_CompNames.end()) {
				reg.ID = it->second;
			}

			if (reg.ID == invalidCompID) {
				SV_LOG_ERROR("Component '%s' doesn't exist", reg.name.c_str());
				return Result_InvalidFormat;
			}

		}

		// Entity data
		u32 entityCount;
		u32 entityDataCount;

		archive >> entityCount;
		archive >> entityDataCount;

		ecs.entities.resize(entityCount);
		EntityData* entityData = new EntityData[entityDataCount];
		EntityTransform* entityTransform = new EntityTransform[entityDataCount];

		for (u32 i = 0; i < entityCount; ++i) {

			Entity entity;
			archive >> entity;

			EntityData& ed = entityData[entity];
			EntityTransform& et = entityTransform[entity];

			archive >> ed.childsCount >> ed.handleIndex >>
				et.localPosition >> et.localRotation >> et.localScale;
		}

		// Create entity list and free list
		for (u32 i = 0u; i < entityDataCount; ++i) {

			EntityData& ed = entityData[i];

			if (ed.handleIndex != u64_max) {
				ecs.entities[ed.handleIndex] = i + 1u;
			}
			else ecs.entityData.freeList.push_back(i + 1u);
		}

		// Set entity parents
		{
			EntityData* it = entityData;
			EntityData* end = it + entityDataCount;

			while (it != end) {

				if (it->handleIndex != u64_max && it->childsCount) {

					Entity* eIt = ecs.entities.data() + it->handleIndex;
					Entity* eEnd = eIt + it->childsCount;
					Entity parent = *eIt++;

					while (eIt <= eEnd) {

						EntityData& ed = entityData[*eIt - 1u];
						ed.parent = parent;
						if (ed.childsCount) {
							eIt += ed.childsCount + 1u;
						}
						else ++eIt;
					}
				}

				++it;
			}
		}

		// Set entity data
		ecs.entityData.data = entityData;
		ecs.entityData.transformData = entityTransform;
		ecs.entityData.accessData = entityData - 1u;
		ecs.entityData.accessTransformData = entityTransform - 1u;
		ecs.entityData.capacity = entityDataCount;
		ecs.entityData.size = entityDataCount;

		// Components
		{
			for (auto it = registers.rbegin(); it != registers.rend(); ++it) {

				CompID compID = it->ID;
				auto& compList = ecs.components[compID];
				u32 compSize = ecs_component_size(compID);
				u32 compCount;
				archive >> compCount;

				if (compCount == 0u) continue;

				// Allocate component data
				{
					u32 poolCount = compCount / ECS_COMPONENT_POOL_SIZE + (compCount % ECS_COMPONENT_POOL_SIZE == 0u) ? 0u : 1u;
					u32 lastPoolCount = u32(compList.pools.size());

					compList.pools.resize(poolCount);

					if (poolCount > lastPoolCount) {
						for (u32 j = lastPoolCount; j < poolCount; ++j) {
							componentPoolAlloc(compList.pools[j], compSize);
						}
					}

					for (u32 j = 0; j < lastPoolCount; ++j) {

						ComponentPool& pool = compList.pools[j];
						u8* it = pool.data;
						u8* end = pool.data + pool.size;
						while (it != end) {

							BaseComponent* comp = reinterpret_cast<BaseComponent*>(it);
							if (comp->entity != SV_ENTITY_NULL) {
								ecs_component_destroy(compID, comp);
							}

							it += compSize;
						}

						pool.size = 0u;
						pool.freeCount = 0u;
					}
				}

				while (compCount-- != 0u) {

					Entity entity;
					archive >> entity;

					BaseComponent* comp = componentAlloc(ecs_, compID, entity, false);
					ecs_component_deserialize(compID, comp, archive);
					comp->entity = entity;

					ecs.entityData[entity].components.emplace_back(compID, comp);
				}

			}
		}

		return Result_Success;
	}

	///////////////////////////////////// COMPONENTS REGISTER ////////////////////////////////////////

	CompID ecs_component_register(const ComponentRegisterDesc* desc)
	{
		// Check if is available
		{
			if (desc->componentSize < sizeof(u32)) {
				SV_LOG_ERROR("Can't register a component type with size of %u", desc->componentSize);
				return SV_COMPONENT_ID_INVALID;
			}

			auto it = g_CompNames.find(desc->name);

			if (it != g_CompNames.end()) {

				SV_LOG_ERROR("Can't register a component type with name '%s', it currently exists", desc->name);
				return SV_COMPONENT_ID_INVALID;
			}
		}

		CompID id = CompID(g_Registers.size());
		ComponentRegister& reg = g_Registers.emplace_back();
		reg.name = desc->name;
		reg.size = desc->componentSize;
		reg.createFn = desc->createFn;
		reg.destroyFn = desc->destroyFn;
		reg.moveFn = desc->moveFn;
		reg.copyFn = desc->copyFn;
		reg.serializeFn = desc->serializeFn;
		reg.deserializeFn = desc->deserializeFn;

		g_CompNames[desc->name] = id;

		return id;
	}

	const char* ecs_component_name(CompID ID)
	{
		return g_Registers[ID].name.c_str();
	}
	u32 ecs_component_size(CompID ID)
	{
		return g_Registers[ID].size;
	}
	CompID ecs_component_id(const char* name)
	{
		auto it = g_CompNames.find(name);
		if (it == g_CompNames.end()) return SV_COMPONENT_ID_INVALID;
		return it->second;
	}
	u32 ecs_component_register_count()
	{
		return u32(g_Registers.size());
	}

	void ecs_component_create(CompID ID, BaseComponent* ptr, Entity entity)
	{
		g_Registers[ID].createFn(ptr);
		ptr->entity = entity;
	}

	void ecs_component_destroy(CompID ID, BaseComponent* ptr)
	{
		g_Registers[ID].destroyFn(ptr);
		ptr->entity = SV_ENTITY_NULL;
	}

	void ecs_component_move(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_Registers[ID].moveFn(from, to);
	}

	void ecs_component_copy(CompID ID, BaseComponent* from, BaseComponent* to)
	{
		g_Registers[ID].copyFn(from, to);
	}

	void ecs_component_serialize(CompID ID, BaseComponent* comp, ArchiveO& archive)
	{
		SerializeComponentFunction fn = g_Registers[ID].serializeFn;
		if (fn) fn(comp, archive);
	}

	void ecs_component_deserialize(CompID ID, BaseComponent* comp, ArchiveI& archive)
	{
		DeserializeComponentFunction fn = g_Registers[ID].deserializeFn;
		if (fn) fn(comp, archive);
	}

	bool ecs_component_exist(CompID ID)
	{
		return g_Registers.size() > ID && g_Registers[ID].createFn != nullptr;
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

	Entity ecs_entity_create(ECS* ecs_, Entity parent, const char* name)
	{
		parseECS();

		Entity entity = entityAlloc(ecs.entityData);

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

		if (name)
			ecs.entityData[entity].name = name;

		return entity;
	}

	void ecs_entity_destroy(ECS* ecs_, Entity entity)
	{
		parseECS();

		EntityData& entityData = ecs.entityData[entity];

		u32 count = entityData.childsCount + 1;

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
			ecs_entity_clear(ecs_, e);
			entityFree(ecs.entityData, e);
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

			componentFree(ecs_, compID, comp);
		}
	}

	Entity ecs_entity_duplicate_recursive(ECS_internal& ecs, Entity duplicated, Entity parent)
	{
		ECS* ecs_ = reinterpret_cast<ECS*>(&ecs);
		Entity copy;

		if (parent == SV_ENTITY_NULL) copy = ecs_entity_create(ecs_);
		else copy = ecs_entity_create(ecs_, parent);

		EntityData& duplicatedEd = ecs.entityData[duplicated];
		EntityData& copyEd = ecs.entityData[copy];

		ecs.entityData.get_transform(copy) = ecs.entityData.get_transform(duplicated);

		for (u32 i = 0; i < duplicatedEd.components.size(); ++i) {
			CompID ID = duplicatedEd.components[i].first;
			size_t SIZE = ecs_component_size(ID);

			BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
			BaseComponent* newComp = componentAlloc(ecs_, ID, comp);

			newComp->entity = copy;
			ecs_entitydata_index_add(copyEd, ID, newComp);
		}

		for (u32 i = 0; i < ecs.entityData[duplicated].childsCount; ++i) {
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
		if (entity == SV_ENTITY_NULL || entity > ecs.entityData.size) return false;

		EntityData& ed = ecs.entityData[entity];
		return ed.handleIndex != u64_max;
	}

	std::string& ecs_entity_name(ECS* ecs_, Entity entity)
	{
		parseECS();
		SV_ASSERT(ecs_entity_exist(ecs_, entity));
		return ecs.entityData[entity].name;
	}

	u32 ecs_entity_childs_count(ECS* ecs_, Entity parent)
	{
		parseECS();
		return ecs.entityData[parent].childsCount;
	}

	void ecs_entity_childs_get(ECS* ecs_, Entity parent, Entity const** childsArray)
	{
		parseECS();

		const EntityData& ed = ecs.entityData[parent];
		if (childsArray && ed.childsCount != 0)* childsArray = &ecs.entities[ed.handleIndex + 1];
	}

	Entity ecs_entity_parent_get(ECS* ecs_, Entity entity)
	{
		parseECS();
		return ecs.entityData[entity].parent;
	}

	Transform ecs_entity_transform_get(ECS* ecs_, Entity entity)
	{
		parseECS();
		return Transform(entity, &ecs.entityData.get_transform(entity), ecs_);
	}

        u32 ecs_entity_flags(ECS* ecs_, Entity entity)
        {
	    parseECS();
            return &ecs.entityData[entity].flags;
        }

	u32 ecs_entity_component_count(ECS* ecs_, Entity entity)
	{
		parseECS();
		return u32(ecs.entityData[entity].components.size());
	}

	void ecs_entities_create(ECS* ecs_, u32 count, Entity parent, Entity* entities)
	{
		parseECS();

		size_t entityIndex = 0u;

		if (parent == SV_ENTITY_NULL) {

			entityIndex = ecs.entities.size();
			ecs.entities.reserve(count);

			// Create entities
			for (size_t i = 0; i < count; ++i) {
				Entity entity = entityAlloc(ecs.entityData);
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
				Entity entity = entityAlloc(ecs.entityData);

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

	void ecs_entities_destroy(ECS* ecs, Entity const* entities, u32 count)
	{
		SV_LOG_ERROR("TODO->ecs_entities_destroy");
	}

	u32 ecs_entity_count(ECS* ecs_)
	{
		parseECS();
		return u32(ecs.entities.size());
	}

	Entity ecs_entity_get(ECS* ecs_, u32 index)
	{
		parseECS();
		return ecs.entities[index];
	}

	/////////////////////////////// COMPONENTS /////////////////////////////////////

	BaseComponent* ecs_component_add(ECS* ecs_, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize)
	{
		parseECS();

		comp = componentAlloc(ecs_, componentID, comp);
		comp->entity = entity;
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, comp);

		return comp;
	}

	BaseComponent* ecs_component_add_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		BaseComponent* comp = componentAlloc(ecs_, componentID, entity);
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, comp);

		return comp;
	}

	BaseComponent* ecs_component_get_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		return ecs_entitydata_index_get(ecs.entityData[entity], componentID);
	}

	std::pair<CompID, BaseComponent*> ecs_component_get_by_index(ECS* ecs_, Entity entity, u32 index)
	{
		parseECS();
		return ecs.entityData[entity].components[index];
	}

	void ecs_component_remove_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		EntityData& ed = ecs.entityData[entity];
		BaseComponent* comp = ecs_entitydata_index_get(ed, componentID);

		componentFree(ecs_, componentID, comp);
		ecs_entitydata_index_remove(ed, componentID);
	}

	u32 ecs_component_count(ECS* ecs_, CompID ID)
	{
		parseECS();
		return componentAllocatorCount(ecs_, ID);
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

		size_t compSize = size_t(ecs_component_size(compID));
		u8* ptr = reinterpret_cast<u8*>(it);
		u8* endPtr = list.pools[pool].data + list.pools[pool].size;

		do {
			ptr += compSize;
			if (ptr == endPtr) {
				auto& list = ecs.components[compID];

				do {

					if (++pool == list.pools.size()) break;

				} while (componentPoolCount(list.pools[pool], compSize) == 0u);

				if (pool == list.pools.size()) break;

				ComponentPool& compPool = list.pools[pool];

				ptr = compPool.data;
				endPtr = ptr + compPool.size;
			}
		} while (reinterpret_cast<BaseComponent*>(ptr)->entity == SV_ENTITY_NULL);

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

	void ComponentIterator::last()
	{
		// TODO:
		SV_LOG_ERROR("TODO-> This may fail");
		parseECS();
		auto& list = ecs.components[compID];

		size_t compSize = size_t(ecs_component_size(compID));
		u8* ptr = reinterpret_cast<u8*>(it);
		u8* beginPtr = list.pools[pool].data;

		do {
			ptr -= compSize;
			if (ptr == beginPtr) {

				while (list.pools[--pool].size == 0u);

				ComponentPool& compPool = list.pools[pool];

				beginPtr = compPool.data;
				ptr = beginPtr + compPool.size;
			}
		} while (reinterpret_cast<BaseComponent*>(ptr)->entity == SV_ENTITY_NULL);

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

	void ComponentIterator::start_begin()
	{
		parseECS();

		pool = 0u;
		u8* ptr = nullptr;

		if (!componentAllocatorIsEmpty(ecs_, compID)) {

			const ComponentAllocator& list = ecs.components[compID];
			size_t compSize = size_t(ecs_component_size(compID));

			while (componentPoolCount(list.pools[pool], compSize) == 0u) pool++;

			ptr = list.pools[pool].data;

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

		const ComponentAllocator& list = ecs.components[compID];

		pool = u32(list.pools.size()) - 1u;
		u8* ptr = nullptr;

		if (!componentAllocatorIsEmpty(ecs_, compID)) {

			size_t compSize = size_t(ecs_component_size(compID));

			while (componentPoolCount(list.pools[pool], compSize) == 0u) pool--;

			ptr = list.pools[pool].data + list.pools[pool].size;
		}

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

}
