#include "core.h"

#include "entity_system_internal.h"
#include "main/task_system.h"
#include "utils/allocator.h"

namespace sv {

	static InstanceAllocator<ECS_internal>	g_ECSAllocator;

#define parseECS() ECS_internal& ecs = *reinterpret_cast<ECS_internal*>(ecs_)

	void ecs_create(ECS** ecs_)
	{
		ECS_internal& ecs = *g_ECSAllocator.create();

		*ecs_ = reinterpret_cast<ECS*>(&ecs);
	}

	void ecs_destroy(ECS* ecs_)
	{
		if (ecs_ == nullptr) return;
		parseECS();

		for (ui16 i = 0; i < ecs_register_count(ecs_); ++i) {
			ecs_allocator_component_destroy(ecs_, ecs.components[i]);
		}
		ecs.components.clear();
		ecs.entities.clear();
		ecs_allocator_entity_clear(ecs.entityData);

		g_ECSAllocator.destroy(&ecs);
	}

	void ecs_clear(ECS* ecs_)
	{
		parseECS();

		for (ui16 i = 0; i < ecs_register_count(ecs_); ++i) {
			ecs_allocator_component_destroy(ecs_, ecs.components[i]);
		}
		ecs.entities.clear();
		ecs_allocator_entity_clear(ecs.entityData);
	}

	Result ecs_serialize(ECS* ecs_, ArchiveO& archive)
	{
		parseECS();

		// Registers
		{
			ui32 registersCount = ecs_register_count(ecs_);
			archive << registersCount;

			while (registersCount-- != 0u) {

				ComponentRegister& compData = ecs.registers[registersCount];
				archive << compData.name << compData.size;

			}
		}
		
		// Entity data
		{
			ui32 entityCount = ui32(ecs.entities.size());
			ui32 entityDataCount = ui32(ecs.entityData.size);

			archive << entityCount << entityDataCount;

			for (ui32 i = 0; i < entityDataCount; ++i) {

				Entity entity = i + 1u;
				EntityData& ed = ecs.entityData[entity];

				if (ed.handleIndex != ui64_max) {

					EntityTransform& transform = ecs.entityData.get_transform(entity);

					archive << i << ed.childsCount << ed.handleIndex << transform.localPosition;
					archive << transform.localRotation << transform.localScale;
				}
			}
		}

		// Components
		{
			ui32 registersCount = ecs_register_count(ecs_);

			for (ui32 i = 0u; i < registersCount; ++i) {

				auto& compList = ecs.components[i];
				CompID compID = CompID(i);
				ui32 compSize = ecs_register_sizeof(ecs_, compID);

				archive << ecs_allocator_component_count(ecs_, compList);

				ComponentIterator it(ecs_, compID, false);
				ComponentIterator end(ecs_, compID, true);

				while (!it.equal(end)) {
					BaseComponent* component = it.get_ptr();
					archive << component->entity;
					ecs_register_serialize(ecs_, compID, component, archive);

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
		ecs_allocator_entity_clear(ecs.entityData);

		struct Register {
			std::string name;
			ui32 size;
			CompID ID;
		};

		// Registers
		std::vector<Register> registers;

		{
			ui32 registersCount;
			archive >> registersCount;
			registers.resize(registersCount);
			
			for (auto it = registers.rbegin(); it != registers.rend(); ++it) {
				
				archive >> it->name;
				archive >> it->size;

			}
		}

		// Registers ID
		CompID invalidCompID = CompID(ecs.registers.size());
		for (ui32 i = 0; i < registers.size(); ++i) {

			Register& reg = registers[i];
			reg.ID = invalidCompID;

			for (ui32 j = 0; j < ecs.registers.size(); ++j) {
				ComponentRegister& cData = ecs.registers[j];

				if (strcmp(cData.name.c_str(), reg.name.c_str()) == 0) {
					reg.ID = j;
					break;
				}
			}

			if (reg.ID == invalidCompID) {
				SV_LOG_ERROR("Component '%s' doesn't exist", reg.name.c_str());
				return Result_InvalidFormat;
			}

		}

		// Entity data
		ui32 entityCount;
		ui32 entityDataCount;

		archive >> entityCount;
		archive >> entityDataCount;

		ecs.entities.resize(entityCount);
		EntityData* entityData = new EntityData[entityDataCount];
		EntityTransform* entityTransform = new EntityTransform[entityDataCount];

		for (ui32 i = 0; i < entityCount; ++i) {

			Entity entity;
			archive >> entity;

			EntityData& ed = entityData[entity];
			EntityTransform& et = entityTransform[entity];

			archive >> ed.childsCount >> ed.handleIndex >> 
				et.localPosition >> et.localRotation >> et.localScale;
		}

		// Create entity list and free list
		{
			for (ui32 i = 0u; i < entityDataCount; ++i) {

				EntityData& ed = entityData[i];

				if (ed.handleIndex != ui64_max) {
					ecs.entities[ed.handleIndex] = i + 1u;
				}
				else ecs.entityData.freeList.push_back(i + 1u);
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
			for (ui32 i = 0; i < registers.size(); ++i) {

				CompID compID = registers[i].ID;
				auto& compList = ecs.components[compID];
				ui32 compSize = ecs_register_sizeof(ecs_, compID);
				ui32 compCount;
				archive >> compCount;

				if (compCount == 0u) continue;

				// Allocate component data
				{
					ui32 poolCount = compCount / ECS_COMPONENT_POOL_SIZE + (compCount % ECS_COMPONENT_POOL_SIZE == 0u) ? 0u : 1u;
					ui32 lastPoolCount = ui32(compList.pools.size());
					
					compList.pools.resize(poolCount);

					if (poolCount > lastPoolCount) {
						for (ui32 j = lastPoolCount; j < poolCount; ++j) {
							ecs_allocator_component_pool_alloc(compList.pools[j], compSize);
						}
					}

					for (ui32 j = 0; j < lastPoolCount; ++j) {

						ComponentPool& pool = compList.pools[j];
						ui8* it = pool.data;
						ui8* end = pool.data + pool.size;
						while (it != end) {

							BaseComponent* comp = reinterpret_cast<BaseComponent*>(it);
							if (comp->entity != SV_ENTITY_NULL) {
								ecs_register_destroy(ecs_, compID, comp);
							}

							it += compSize;
						}

						pool.size = 0u;
						pool.freeList.clear();
					}
				}

				while (compCount-- != 0u) {

					Entity entity;
					archive >> entity;

					BaseComponent* comp = ecs_allocator_component_alloc(ecs_, compList, entity);
					ecs_register_deserialize(ecs_, compID, comp, archive);

					ecs.entityData[entity].components.emplace_back(compID, comp);
				}

			}
		}

		return Result_Success;
	}

	///////////////////////////////////// COMPONENTS REGISTER ////////////////////////////////////////

	CompID ecs_register(ECS* ecs_, const ComponentRegisterDesc* desc)
	{
		parseECS();
		CompID ID = CompID(ecs.registers.size());
		ComponentRegister& reg = ecs.registers.emplace_back();

		reg.size = desc->size;
		reg.name = desc->name;
		reg.createFn = desc->createFn;
		reg.destroyFn = desc->destroyFn;
		reg.moveFn = desc->moveFn;
		reg.copyFn = desc->copyFn;
		reg.serializeFn = desc->serializeFn;
		reg.deserializeFn = desc->deserializeFn;

		ComponentAllocator& compAlloc = ecs.components.emplace_back();
		ecs_allocator_component_create(reinterpret_cast<ECS*>(&ecs), compAlloc, ID);

		return ID;
	}

	ui32 ecs_register_count(ECS* ecs_)
	{
		parseECS();
		return ui32(ecs.registers.size());
	}
	ui32 ecs_register_sizeof(ECS* ecs_, CompID ID)
	{
		parseECS();
		return ecs.registers[ID].size;
	}
	const char* ecs_register_nameof(ECS* ecs_, CompID ID)
	{
		parseECS();
		return ecs.registers[ID].name.c_str();
	}
	
	void ecs_register_create(ECS* ecs_, CompID ID, BaseComponent* ptr, Entity entity)
	{
		parseECS();
		ecs.registers[ID].createFn(ptr);
		ptr->entity = entity;
	}

	void ecs_register_destroy(ECS* ecs_, CompID ID, BaseComponent* ptr)
	{
		parseECS();
		ecs.registers[ID].destroyFn(ptr);
		ptr->entity = SV_ENTITY_NULL;
	}

	void ecs_register_move(ECS* ecs_, CompID ID, BaseComponent* from, BaseComponent* to)
	{
		parseECS();
		ecs.registers[ID].moveFn(from, to);
	}

	void ecs_register_copy(ECS* ecs_, CompID ID, BaseComponent* from, BaseComponent* to)
	{
		parseECS();
		ecs.registers[ID].copyFn(from, to);
	}

	void ecs_register_serialize(ECS* ecs_, CompID ID, BaseComponent* comp, ArchiveO& archive)
	{
		parseECS();
		SerializeComponentFunction fn = ecs.registers[ID].serializeFn;
		if (fn) fn(comp, archive);
	}

	void ecs_register_deserialize(ECS* ecs_, CompID ID, BaseComponent* comp, ArchiveI& archive)
	{
		parseECS();
		DeserializeComponentFunction fn = ecs.registers[ID].deserializeFn;
		if (fn) fn(comp, archive);
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

		Entity entity = ecs_allocator_entity_alloc(ecs.entityData);

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
			ecs_entity_clear(ecs_, e);
			ecs_allocator_entity_free(ecs.entityData, e);
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

			ecs_allocator_component_free(ecs_, ecs.components[compID], comp);
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

		for (ui32 i = 0; i < duplicatedEd.components.size(); ++i) {
			CompID ID = duplicatedEd.components[i].first;
			size_t SIZE = ecs_register_sizeof(ecs_, ID);

			BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
			BaseComponent* newComp = ecs_allocator_component_alloc(ecs_, ecs.components[ID], comp);

			newComp->entity = copy;
			ecs_entitydata_index_add(copyEd, ID, newComp);
		}

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
		if (entity == SV_ENTITY_NULL || entity > ecs.entityData.size) return false;

		EntityData& ed = ecs.entityData[entity];
		return ed.handleIndex != ui64_max;
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
		return Transform(entity, &ecs.entityData.get_transform(entity), ecs_);
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
				Entity entity = ecs_allocator_entity_alloc(ecs.entityData);
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
				Entity entity = ecs_allocator_entity_alloc(ecs.entityData);

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
		SV_LOG_ERROR("TODO->ecs_entities_destroy");
	}

	ui32 ecs_entity_count(ECS* ecs_)
	{
		parseECS();
		return ui32(ecs.entities.size());
	}

	Entity ecs_entity_get(ECS* ecs_, ui32 index)
	{
		parseECS();
		return ecs.entities[index];
	}

	/////////////////////////////// COMPONENTS /////////////////////////////////////

	BaseComponent* ecs_component_add(ECS* ecs_, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize)
	{
		parseECS();

		comp = ecs_allocator_component_alloc(ecs_, ecs.components[componentID], comp);
		comp->entity = entity;
		ecs_entitydata_index_add(ecs.entityData[entity], componentID, comp);
		return comp;
	}

	BaseComponent* ecs_component_add_by_id(ECS* ecs_, Entity entity, CompID componentID)
	{
		parseECS();

		BaseComponent* comp = ecs_allocator_component_alloc(ecs_, ecs.components[componentID], entity);
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
		ecs_allocator_component_free(ecs_, ecs.components[componentID], comp);
		ecs_entitydata_index_remove(ed, componentID);
	}

	ui32 ecs_component_count(ECS* ecs_, CompID ID)
	{
		parseECS();
		return ecs_allocator_component_count(ecs_, ecs.components[ID]);
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

		size_t compSize = size_t(ecs_register_sizeof(ecs_, compID));
		ui8* ptr = reinterpret_cast<ui8*>(it);
		ui8* endPtr = list.pools[pool].data + list.pools[pool].size;

		do {
			ptr += compSize;
			if (ptr == endPtr) {
				auto& list = ecs.components[compID];

				do {

					if (++pool == list.pools.size()) break;

				} while (list.pools[pool].size == 0u);

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
		parseECS();
		auto& list = ecs.components[compID];

		size_t compSize = size_t(ecs_register_sizeof(ecs_, compID));
		ui8* ptr = reinterpret_cast<ui8*>(it);
		ui8* beginPtr = list.pools[pool].data;

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

		auto& list = ecs.components[compID];

		pool = 0u;
		ui8* ptr = nullptr;

		if (!ecs_allocator_component_empty(ecs_, list)) {

			ui32 compSize = ecs_register_sizeof(ecs_, compID);

			while (ecs_allocator_component_pool_count(list.pools[pool]) == 0u) pool++;

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

		auto& list = ecs.components[compID];

		pool = ui32(list.pools.size()) - 1u;
		ui8* ptr = nullptr;
		ui32 compSize = ecs_register_sizeof(ecs_, compID);

		if (!ecs_allocator_component_empty(ecs_, list)) {

			while (ecs_allocator_component_pool_count(list.pools[pool]) == 0u) pool--;

			ptr = list.pools[pool].data + list.pools[pool].size;

		}

		it = reinterpret_cast<BaseComponent*>(ptr);
	}

}