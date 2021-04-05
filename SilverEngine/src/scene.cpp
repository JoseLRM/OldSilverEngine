#include "scene.h"
#include "renderer.h"
#include "dev.h"
//#include "audio.h"

#define PREFAB_COMPONENT_FLAG SV_BIT(31ULL)

namespace sv {

    struct EntityTransform {

	XMFLOAT3 localPosition = { 0.f, 0.f, 0.f };
	XMFLOAT4 localRotation = { 0.f, 0.f, 0.f, 1.f };
	XMFLOAT3 localScale = { 1.f, 1.f, 1.f };

	XMFLOAT4X4 worldMatrix;

	bool modified = true;

    };

    struct EntityData {

	size_t handleIndex = u64_max;
	Entity parent = SV_ENTITY_NULL;
	u32 childsCount = 0u;
	u32 flags = 0u;
	List<std::pair<CompID, BaseComponent*>> components;
	std::string name;

    };

    struct EntityDataAllocator {

	EntityData* data = nullptr;
	EntityTransform* transformData = nullptr;

	EntityData* accessData = nullptr;
	EntityTransform* accessTransformData = nullptr;

	u32 size = 0u;
	u32 capacity = 0u;
	List<Entity> freeList;

	inline EntityData& operator[](Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessData[entity]; }
	inline EntityTransform& get_transform(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL); return accessTransformData[entity]; }

    };

    struct Prefab_internal {

	List<Entity> entities;
	List<std::pair<CompID, BaseComponent*>> components;

    };

    struct ComponentPool {

	u8* data;
	size_t		size;
	size_t		freeCount;

    };

    struct ComponentAllocator {

	List<ComponentPool> pools;

    };

    struct Scene_internal : public Scene {

	// TODO: Use List
	std::vector<Entity>	 entities;
	EntityDataAllocator	 entityData;
	List<ComponentAllocator> components;

    };

    constexpr u32 ECS_COMPONENT_POOL_SIZE = 100u;
    constexpr u32 ECS_ENTITY_ALLOC_SIZE = 100u;

    struct ComponentRegister {

	std::string						name;
	u32								size;
	u32 version;
	CreateComponentFunction			createFn = nullptr;
	DestroyComponentFunction		destroyFn = nullptr;
	MoveComponentFunction			moveFn = nullptr;
	CopyComponentFunction			copyFn = nullptr;
	SerializeComponentFunction		serializeFn = nullptr;
	DeserializeComponentFunction	deserializeFn = nullptr;

    };

    static List<ComponentRegister>	           g_Registers;
    static std::unordered_map<std::string, CompID> g_CompNames;

#define PARSE_SCENE() sv::Scene_internal& scene = *reinterpret_cast<sv::Scene_internal*>(scene_)

    Entity	entityAlloc(EntityDataAllocator& allocator);
    void	entityFree(EntityDataAllocator& allocator, Entity entity);
    void	entityClear(EntityDataAllocator& allocator);

    void	componentPoolAlloc(ComponentPool& pool, size_t compSize);						// Allocate components memory
    void	componentPoolFree(ComponentPool& pool);											// Deallocate components memory
    void*	componentPoolGetPtr(ComponentPool& pool, size_t compSize);						// Return new component
    void	componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr);			// Remove component
    bool	componentPoolFull(const ComponentPool& pool, size_t compSize);					// Check if there are free space in the pool
    bool	componentPoolPtrExist(const ComponentPool& pool, void* ptr);					// Check if the pool contains the ptr
    u32		componentPoolCount(const ComponentPool& pool, size_t compSize);					// Return the number of valid components allocated in this pool

    void			componentAllocatorCreate(Scene* scene, CompID ID);									// Create the allocator
    void			componentAllocatorDestroy(Scene* scene, CompID ID);									// Destroy the allocator
    BaseComponent*	componentAlloc(Scene* scene, CompID compID, Entity entity, bool create = true);		// Allocate and create new component
    BaseComponent*	componentAlloc(Scene* scene, CompID compID, BaseComponent* srcComp, Entity entity);	// Allocate and copy new component
    void			componentFree(Scene* scene, CompID compID, BaseComponent* comp);					// Free and destroy component
    u32				componentAllocatorCount(Scene* scene, CompID compId);								// Return the number of valid components in all the pools
    bool			componentAllocatorIsEmpty(Scene* scene, CompID compID);								// Return if the allocator is empty

    Result initialize_scene(Scene** pscene, const char* name)
    {
	Scene_internal& scene = *new Scene_internal();
	Scene* scene_ = reinterpret_cast<Scene*>(&scene);

	// Init
	{
	    // Allocate components
	    scene.components.resize(g_Registers.size());

	    for (CompID id = 0u; id < g_Registers.size(); ++id) {

		componentAllocatorCreate(reinterpret_cast<Scene*>(&scene), id);
	    }

	    scene.name = name;
	}

	svCheck(gui_create(hash_string(name), &scene.gui));
	//svCheck(create_audio_device(&scene.audio_device));

	bool deserialize = false;
	Archive archive;

	// Deserialize
	{
	    // Get filepath
	    char filepath[FILEPATH_SIZE];

	    bool exist = user_get_scene_filepath(name, filepath);

	    if (exist) {

		Result res = archive.openFile(filepath);

		if (result_fail(res)) {

		    SV_LOG_ERROR("Can't deserialize the scene '%s' at '%s'", name, filepath);
		}
		else {
		    u32 version;
		    archive >> version;
		    archive >> scene.main_camera;

		    // ECS
		    {
			scene.entities.clear();
			entityClear(scene.entityData);

			struct Register {
			    std::string name;
			    u32 size;
			    u32 version;
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
				archive >> it->version;

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

			scene.entities.resize(entityCount);
			EntityData* entityData = new EntityData[entityDataCount];
			EntityTransform* entityTransform = new EntityTransform[entityDataCount];

			for (u32 i = 0; i < entityCount; ++i) {

			    Entity entity;
			    archive >> entity;

			    EntityData& ed = entityData[entity];
			    EntityTransform& et = entityTransform[entity];

			    archive >> ed.name;

			    archive >> ed.flags >> ed.childsCount >> ed.handleIndex >>
				et.localPosition >> et.localRotation >> et.localScale;
			}

			// Create entity list and free list
			for (u32 i = 0u; i < entityDataCount; ++i) {

			    EntityData& ed = entityData[i];

			    if (ed.handleIndex != u64_max) {
				scene.entities[ed.handleIndex] = i + 1u;
			    }
			    else scene.entityData.freeList.push_back(i + 1u);
			}

			// Set entity parents
			{
			    EntityData* it = entityData;
			    EntityData* end = it + entityDataCount;

			    while (it != end) {

				if (it->handleIndex != u64_max && it->childsCount) {

				    Entity* eIt = scene.entities.data() + it->handleIndex;
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
			scene.entityData.data = entityData;
			scene.entityData.transformData = entityTransform;
			scene.entityData.accessData = entityData - 1u;
			scene.entityData.accessTransformData = entityTransform - 1u;
			scene.entityData.capacity = entityDataCount;
			scene.entityData.size = entityDataCount;

			// Components
			{
			    for (auto it = registers.rbegin(); it != registers.rend(); ++it) {

				CompID compID = it->ID;
				u32 version = it->version;
				auto& compList = scene.components[compID];
				u32 compSize = get_component_size(compID);
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
					    if (comp->_id != 0u) {
						destroy_component(scene_, compID, comp);
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

				    BaseComponent* comp = componentAlloc(scene_, compID, entity, false);
				    create_component(scene_, compID, comp, entity);
				    deserialize_component(scene_, compID, comp, version , archive);

				    scene.entityData[entity].components.emplace_back(compID, comp);
				}

			    }
			}

			deserialize = true;
		    }
		}
	    }
	}

	// User Init
	Result res = user_initialize_scene(scene_, deserialize ? &archive : nullptr);
	if (result_fail(res)) {
	    // TODO: handle error
	    return res;
	}
	
	*pscene = scene_;
	return Result_Success;
    }

    Result close_scene(Scene* scene_)
    {
	PARSE_SCENE();

	// User close TODO: Handle error
	user_close_scene(scene_);

	gui_destroy(scene.gui);
	//destroy_audio_device(scene.audio_device);

	// Close ECS
	{
	    for (CompID i = 0; i < get_component_register_count(); ++i) {
		if (component_exist(i))
		    componentAllocatorDestroy(scene_, i);
	    }
	    scene.components.clear();
	    scene.entities.clear();
	    entityClear(scene.entityData);
	}
		
	return Result_Success;
    }

    Result set_active_scene(const char* name)
    {
	size_t name_size = strlen(name);

	if (name_size > SCENENAME_SIZE) {
	    SV_LOG_ERROR("The scene name '%s' is to long, max chars = %u", name, SCENENAME_SIZE);
	    return Result_InvalidUsage;
	}
	
	// validate scene
	if (user_validate_scene(name)) {
	    memcpy(engine.next_scene_name, name, name_size + 1u);
	    return Result_Success;
	}
	return Result_NotFound;
    }

    Result save_scene(Scene* scene_, const char* filepath)
    {
	PARSE_SCENE();

	Archive archive;

	archive << Scene::VERSION;
	archive << scene.main_camera;
		
	// ECS
	{
	    // Registers
	    {
		u32 registersCount = 0u;
		for (CompID id = 0u; id < g_Registers.size(); ++id) {
		    if (component_exist(id))
			++registersCount;
		}
		archive << registersCount;

		for (CompID id = 0u; id < g_Registers.size(); ++id) {

		    if (component_exist(id)) {
			archive << get_component_name(id) << get_component_size(id) << get_component_version(id);
		    }

		}
	    }

	    // Entity data
	    {
		u32 entityCount = u32(scene.entities.size());
		u32 entityDataCount = u32(scene.entityData.size);

		archive << entityCount << entityDataCount;

		for (u32 i = 0; i < entityDataCount; ++i) {

		    Entity entity = i + 1u;
		    EntityData& ed = scene.entityData[entity];

		    if (ed.handleIndex != u64_max) {

			EntityTransform& transform = scene.entityData.get_transform(entity);

			archive << i << ed.name << ed.flags << ed.childsCount << ed.handleIndex << transform.localPosition;
			archive << transform.localRotation << transform.localScale;
		    }
		}
	    }

	    // Components
	    {
		for (CompID compID = 0u; compID < g_Registers.size(); ++compID) {

		    if (!component_exist(compID)) continue;

		    archive << componentAllocatorCount(scene_, compID);

		    ComponentIterator it = begin_component_iterator(scene_, compID);
		    ComponentIterator end = end_component_iterator(scene_, compID);

		    while (!it.equal(end)) {

			BaseComponent* component;
			Entity entity;
			it.get(&entity, &component);

			archive << entity;
			serialize_component(scene_, compID, component, archive);

			it.next();
		    }
		}
	    }
	}

	user_serialize_scene(scene_, &archive);
		
	return archive.saveFile(filepath);
    }

    Result clear_scene(Scene* scene_)
    {
	PARSE_SCENE();

	for (CompID i = 0; i < get_component_register_count(); ++i) {
	    if (component_exist(i))
		componentAllocatorDestroy(scene_, i);
	}
	for (CompID id = 0u; id < g_Registers.size(); ++id) {

	    componentAllocatorCreate(reinterpret_cast<Scene*>(&scene), id);
	}

	scene.entities.clear();
	entityClear(scene.entityData);

	// user initialize scene
	svCheck(user_initialize_scene(scene_, nullptr));

	return Result_Success;
    }

    Result create_entity_model(Scene* scene_, Entity parent, const char* folderpath)
    {
	//PARSE_SCENE();
		
	FolderIterator it;
	FolderElement element;

	Result res = folder_iterator_begin(folderpath, &it, &element);

	if (result_okay(res)) {

	    do {

		if (!element.is_file)
		    continue;

		if (element.extension && strcmp(element.extension, "mesh") == 0) {

		    char filepath[FILEPATH_SIZE];
		    sprintf(filepath, "%s/%s", folderpath, element.name);
		    SV_LOG("%s", filepath);

		    MeshAsset mesh;

		    Result res = load_asset_from_file(mesh, filepath);

		    if (result_okay(res)) {
					
			Entity entity = create_entity(scene_, parent);
			MeshComponent* comp = add_component<MeshComponent>(scene_, entity);
			comp->mesh = mesh;

			Transform trans = get_entity_transform(scene_, entity);
			Mesh* m = mesh.get();

			trans.setMatrix(m->model_transform_matrix);

			if (m->model_material_filepath.size()) {
					
			    MaterialAsset mat;
			    res = load_asset_from_file(mat, m->model_material_filepath.c_str());

			    if (result_okay(res)) {
							
				comp->material = mat;
			    }
			}
		    }
		}
		
	    } while(folder_iterator_next(&it, &element));

	    folder_iterator_close(&it);
	    
	}
	else return res;
	
	return Result_Success;
    }

    static void update_camera_matrices(CameraComponent& camera, const v3_f32& position, const v4_f32& rotation)
    {
	// Compute view matrix
	camera.view_matrix = math_matrix_view(position, rotation);

	// Compute projection matrix
	{
#if SV_DEV
	    if (camera.near >= camera.far) {
		SV_LOG_WARNING("Computing the projection matrix. The far must be grater than near");
	    }

	    switch (camera.projection_type)
	    {
	    case ProjectionType_Orthographic:
		break;

	    case ProjectionType_Perspective:
		if (camera.near <= 0.f) {
		    SV_LOG_WARNING("In perspective projection, near must be greater to 0");
		}
		break;
	    }
#endif

	    switch (camera.projection_type)
	    {
	    case ProjectionType_Orthographic:
		camera.projection_matrix = XMMatrixOrthographicLH(camera.width, camera.height, camera.near, camera.far);
		break;

	    case ProjectionType_Perspective:
		camera.projection_matrix = XMMatrixPerspectiveLH(camera.width, camera.height, camera.near, camera.far);
		break;

	    default:
		camera.projection_matrix = XMMatrixIdentity();
		break;
	    }
	}

	// Compute view projection matrix
	camera.view_projection_matrix = camera.view_matrix * camera.projection_matrix;

	// Compute inverse matrices
	camera.inverse_view_matrix = XMMatrixInverse(nullptr, camera.view_matrix);
	camera.inverse_projection_matrix = XMMatrixInverse(nullptr, camera.projection_matrix);
	camera.inverse_view_projection_matrix = camera.inverse_view_matrix * camera.inverse_projection_matrix;
    }

    void update_scene()
    {
	Scene* scene_ = engine.scene;

	if (scene_ == nullptr)
	    return;
	
	PARSE_SCENE();

	if (!entity_exist(scene_, scene.main_camera)) {
	    scene.main_camera = SV_ENTITY_NULL;
	}

	CameraComponent* camera = get_main_camera(scene_);

	// Adjust camera
	if (camera) {
	    v2_u32 size = os_window_size();
	    camera->adjust(f32(size.x) / f32(size.y));
	}

	// Update cameras matrices
	{
	    EntityView<CameraComponent> cameras(scene_);

	    for (ComponentView<CameraComponent> view : cameras) {

		CameraComponent& camera = *view.comp;
		Entity entity = view.entity;

		Transform trans = get_entity_transform(scene_, entity);
		v3_f32 position = trans.getWorldPosition();
		v4_f32 rotation = trans.getWorldRotation();

		update_camera_matrices(camera, position, rotation);
	    }

#if SV_DEV
	    update_camera_matrices(dev.camera, dev.camera.position, dev.camera.rotation);
#endif
	}

#if SV_DEV
	if (dev.game_state != GameState_Play)
	    return;
#endif

	// User update
	if (engine.user.update)
	    engine.user.update();
    }

    CameraComponent* get_main_camera(Scene* scene)
    {
	if (scene->main_camera == SV_ENTITY_NULL || !entity_exist(scene, scene->main_camera)) return nullptr;
	return get_component<CameraComponent>(scene, scene->main_camera);
    }

    Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera)
    {
	Ray ray;

	if (camera->projection_type == ProjectionType_Perspective) {
			
	    // Screen to clip space
	    position *= 2.f;

	    XMVECTOR mouse_world = XMVectorSet(position.x, position.y, 1.f, 1.f);
	    mouse_world = XMVector4Transform(mouse_world, camera->inverse_projection_matrix);
	    mouse_world = XMVectorSetZ(mouse_world, 1.f);
	    mouse_world = XMVector4Transform(mouse_world, camera->inverse_view_matrix);
	    mouse_world = XMVector3Normalize(mouse_world);
			
	    ray.origin = camera_position;
	    ray.direction = v3_f32(mouse_world);
	}
	else {

	    position = position * v2_f32(camera->width, camera->height) + camera_position.getVec2();
	    ray.origin = position.getVec3(camera->near);
	    ray.direction = { 0.f, 0.f, 1.f };
	}

	return ray;
    }

    ////////////////////////////////////////////////// ECS ////////////////////////////////////////////////////////

    // MEMORY

    static Entity entityAlloc(EntityDataAllocator& a)
    {
	if (a.freeList.empty()) {

	    if (a.size == a.capacity) {
		EntityData* newData = new EntityData[a.capacity + ECS_ENTITY_ALLOC_SIZE];
		EntityTransform* newTransformData = new EntityTransform[a.capacity + ECS_ENTITY_ALLOC_SIZE];

		if (a.data) {

		    {
			EntityData* end = a.data + a.capacity;
			while (a.data != end) {

			    *newData = std::move(*a.data);

			    ++newData;
			    ++a.data;
			}
		    }
		    {
			memcpy(newTransformData, a.transformData, a.capacity * sizeof(EntityTransform));
		    }

		    a.data -= a.capacity;
		    newData -= a.capacity;
		    delete[] a.data;
		    delete[] a.transformData;
		}

		a.capacity += ECS_ENTITY_ALLOC_SIZE;
		a.data = newData;
		a.transformData = newTransformData;
		a.accessData = a.data - 1u;
		a.accessTransformData = a.transformData - 1u;
	    }

	    return ++a.size;

	}
	else {
	    Entity result = a.freeList.back();
	    a.freeList.pop_back();
	    return result;
	}
    }

    static void entityFree(EntityDataAllocator& a, Entity entity)
    {
	SV_ASSERT(a.size >= entity);
	a.accessData[entity] = EntityData();
	a.accessTransformData[entity] = EntityTransform();

	if (entity == a.size) {
	    a.size--;
	}
	else {
	    a.freeList.push_back(entity);
	}
    }

    static void entityClear(EntityDataAllocator& a)
    {
	if (a.data) {
	    a.capacity = 0u;
	    delete[] a.data;
	    a.data = nullptr;
	    delete[] a.transformData;
	    a.transformData = nullptr;
	}

	a.accessData = nullptr;
	a.accessTransformData = nullptr;
	a.size = 0u;
	a.freeList.clear();
    }

    // ComponentPool

    static void componentPoolAlloc(ComponentPool& pool, size_t compSize)
    {
	componentPoolFree(pool);
	pool.data = new u8[compSize * ECS_COMPONENT_POOL_SIZE];
	pool.freeCount = 0u;
    }

    static void componentPoolFree(ComponentPool& pool)
    {
	if (pool.data != nullptr) {
	    delete[] pool.data;
	    pool.data = nullptr;
	}
	pool.size = 0u;
    }

    static void* componentPoolGetPtr(ComponentPool& pool, size_t compSize)
    {
	u8* ptr = nullptr;

	if (pool.freeCount) {

	    u8* it = pool.data;
	    u8* end = it + pool.size;

	    while (it != end) {

		if (reinterpret_cast<BaseComponent*>(it)->_id == 0u) {
		    ptr = it;
		    break;
		}
		it += compSize;
	    }
	    SV_ASSERT(ptr != nullptr && "Free component not found!!");
	    --pool.freeCount;
	}
	else {
	    ptr = pool.data + pool.size;
	    pool.size += compSize;
	}

	return ptr;
    }

    static void componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr)
    {
	if (ptr == pool.data + pool.size) {
	    pool.size -= compSize;
	}
	else {
	    ++pool.freeCount;
	}
    }

    static bool componentPoolFull(const ComponentPool& pool, size_t compSize)
    {
	return (pool.size == ECS_COMPONENT_POOL_SIZE * compSize) && pool.freeCount == 0u;
    }

    static bool componentPoolPtrExist(const ComponentPool& pool, void* ptr)
    {
	return ptr >= pool.data && ptr < (pool.data + pool.size);
    }

    static u32 componentPoolCount(const ComponentPool& pool, size_t compSize)
    {
	return u32(pool.size / compSize - pool.freeCount);
    }

    // ComponentAllocator

    SV_AUX ComponentPool& componentAllocatorCreatePool(ComponentAllocator& a, size_t compSize)
    {
	ComponentPool& pool = a.pools.emplace_back();
	componentPoolAlloc(pool, compSize);
	return pool;
    }

    SV_AUX ComponentPool& componentAllocatorPreparePool(ComponentAllocator& a, size_t compSize)
    {
	if (componentPoolFull(a.pools.back(), compSize)) {
	    return componentAllocatorCreatePool(a, compSize);
	}
	return a.pools.back();
    }

    internal void componentAllocatorCreate(Scene* scene_, CompID compID)
    {
	PARSE_SCENE();
	componentAllocatorDestroy(scene_, compID);
	componentAllocatorCreatePool(scene.components[compID], size_t(get_component_size(compID)));
    }

    internal void componentAllocatorDestroy(Scene* scene_, CompID compID)
    {
	PARSE_SCENE();

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

	    u8* ptr = it->data;
	    u8* endPtr = it->data + it->size;

	    while (ptr != endPtr) {

		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
		if (comp->_id != 0u) {
		    destroy_component(scene_, compID, comp);
		}

		ptr += compSize;
	    }

	    componentPoolFree(*it);

	}
	a.pools.clear();
    }

    internal BaseComponent* componentAlloc(Scene* scene_, CompID compID, Entity entity, bool create)
    {
	PARSE_SCENE();

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
	BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

	if (create) create_component(scene_, compID, comp, entity);

	return comp;
    }

    internal BaseComponent* componentAlloc(Scene* scene_, CompID compID, BaseComponent* srcComp, Entity entity)
    {
	PARSE_SCENE();

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
	BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

	create_component(scene_, compID, comp, entity);
	copy_component(scene_, compID, srcComp, comp);

	return comp;
    }

    internal void componentFree(Scene* scene_, CompID compID, BaseComponent* comp)
    {
	PARSE_SCENE();
	SV_ASSERT(comp != nullptr);

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

	    if (componentPoolPtrExist(*it, comp)) {

		destroy_component(scene_, compID, comp);
		componentPoolRmvPtr(*it, compSize, comp);

		break;
	    }
	}
    }

    internal u32 componentAllocatorCount(Scene* scene_, CompID compID)
    {
	PARSE_SCENE();

	const ComponentAllocator& a = scene.components[compID];
	u32 compSize = get_component_size(compID);

	u32 res = 0u;
	for (const ComponentPool& pool : a.pools) {
	    res += componentPoolCount(pool, compSize);
	}
	return res;
    }

    internal bool componentAllocatorIsEmpty(Scene* scene_, CompID compID)
    {
	PARSE_SCENE();

	const ComponentAllocator& a = scene.components[compID];
	u32 compSize = get_component_size(compID);

	for (const ComponentPool& pool : a.pools) {
	    if (componentPoolCount(pool, compSize) > 0u) return false;
	}
	return true;
    }

    ///////////////////////////////////// COMPONENTS REGISTER ////////////////////////////////////////

    CompID register_component(const ComponentRegisterDesc* desc)
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
	reg.version = desc->version;
	reg.createFn = desc->createFn;
	reg.destroyFn = desc->destroyFn;
	reg.moveFn = desc->moveFn;
	reg.copyFn = desc->copyFn;
	reg.serializeFn = desc->serializeFn;
	reg.deserializeFn = desc->deserializeFn;

	g_CompNames[desc->name] = id;

	return id;
    }

    const char* get_component_name(CompID ID)
    {
	return g_Registers[ID].name.c_str();
    }
    u32 get_component_size(CompID ID)
    {
	return g_Registers[ID].size;
    }
    u32 get_component_version(CompID ID)
    {
	return g_Registers[ID].version;
    }
    CompID get_component_id(const char* name)
    {
	auto it = g_CompNames.find(name);
	if (it == g_CompNames.end()) return SV_COMPONENT_ID_INVALID;
	return it->second;
    }
    u32 get_component_register_count()
    {
	return u32(g_Registers.size());
    }

    void create_component(Scene* scene, CompID ID, BaseComponent* ptr, Entity entity)
    {
	g_Registers[ID].createFn(scene, ptr);
	ptr->_id = Entity(entity);
    }

    void destroy_component(Scene* scene, CompID ID, BaseComponent* ptr)
    {
	g_Registers[ID].destroyFn(scene, ptr);
	ptr->_id = 0u;
    }

    void move_component(Scene* scene, CompID ID, BaseComponent* from, BaseComponent* to)
    {
	g_Registers[ID].moveFn(scene, from, to);
    }

    void copy_component(Scene* scene, CompID ID, const BaseComponent* from, BaseComponent* to)
    {
	g_Registers[ID].copyFn(scene, from, to);
    }

    void serialize_component(Scene* scene, CompID ID, BaseComponent* comp, Archive& archive)
    {
	SerializeComponentFunction fn = g_Registers[ID].serializeFn;
	if (fn) fn(scene, comp, archive);
    }

    void deserialize_component(Scene* scene, CompID ID, BaseComponent* comp, u32 version, Archive& archive)
    {
	DeserializeComponentFunction fn = g_Registers[ID].deserializeFn;
	if (fn) fn(scene, comp, version, archive);
    }

    bool component_exist(CompID ID)
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

    SV_INLINE static void update_childs(Scene_internal& scene, Entity entity, i32 count)
    {
	Entity parent = entity;

	while (parent != SV_ENTITY_NULL) {
	    EntityData& parentToUpdate = scene.entityData[parent];
	    parentToUpdate.childsCount += count;
	    parent = parentToUpdate.parent;
	}
    }

    ///////////////////////////////////// ENTITIES ////////////////////////////////////////

    Entity create_entity(Scene* scene_, Entity parent, const char* name)
    {
	PARSE_SCENE();

	Entity entity = entityAlloc(scene.entityData);

	if (parent == SV_ENTITY_NULL) {
	    scene.entityData[entity].handleIndex = scene.entities.size();
	    scene.entities.emplace_back(entity);
	}
	else {
	    update_childs(scene, parent, 1u);

	    // Set parent and handleIndex
	    EntityData& parentData = scene.entityData[parent];
	    size_t index = parentData.handleIndex + size_t(parentData.childsCount);

	    EntityData& entityData = scene.entityData[entity];
	    entityData.handleIndex = index;
	    entityData.parent = parent;

	    // Special case, the parent and childs are in back of the list
	    if (index == scene.entities.size()) {
		scene.entities.emplace_back(entity);
	    }
	    else {
		scene.entities.insert(scene.entities.begin() + index, entity);

		for (size_t i = index + 1; i < scene.entities.size(); ++i) {
		    scene.entityData[scene.entities[i]].handleIndex++;
		}
	    }
	}

	if (name)
	    scene.entityData[entity].name = name;

	return entity;
    }

    void destroy_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	EntityData& entityData = scene.entityData[entity];

	u32 count = entityData.childsCount + 1;

	// notify parents
	if (entityData.parent != SV_ENTITY_NULL) {
	    update_childs(scene, entityData.parent, -i32(count));
	}

	// data to remove entities
	size_t indexBeginDest = entityData.handleIndex;
	size_t indexBeginSrc = entityData.handleIndex + count;
	size_t cpyCant = scene.entities.size() - indexBeginSrc;

	// remove components & entityData
	for (size_t i = 0; i < count; i++) {
	    Entity e = scene.entities[indexBeginDest + i];
	    clear_entity(scene_, e);
	    entityFree(scene.entityData, e);
	}

	// remove from entities & update indices
	if (cpyCant != 0) memcpy(&scene.entities[indexBeginDest], &scene.entities[indexBeginSrc], cpyCant * sizeof(Entity));
	scene.entities.resize(scene.entities.size() - count);
	for (size_t i = indexBeginDest; i < scene.entities.size(); ++i) {
	    scene.entityData[scene.entities[i]].handleIndex = i;
	}
    }

    void clear_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	EntityData& ed = scene.entityData[entity];
	while (!ed.components.empty()) {

	    CompID compID = ed.components.back().first;
	    BaseComponent* comp = ed.components.back().second;
	    ed.components.pop_back();

	    componentFree(scene_, compID, comp);
	}
    }

    static Entity entity_duplicate_recursive(Scene_internal& scene, Entity duplicated, Entity parent)
    {
	Scene* scene_ = reinterpret_cast<Scene*>(&scene);
	Entity copy;

	copy = create_entity(scene_, parent);

	EntityData& duplicatedEd = scene.entityData[duplicated];
	EntityData& copyEd = scene.entityData[copy];

	copyEd.name = duplicatedEd.name;

	scene.entityData.get_transform(copy) = scene.entityData.get_transform(duplicated);
	copyEd.flags = duplicatedEd.flags;

	for (u32 i = 0; i < duplicatedEd.components.size(); ++i) {
	    CompID ID = duplicatedEd.components[i].first;

	    BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
	    BaseComponent* newComp = componentAlloc(scene_, ID, comp, copy);

	    ecs_entitydata_index_add(copyEd, ID, newComp);
	}

	for (u32 i = 0; i < scene.entityData[duplicated].childsCount; ++i) {
	    Entity toCopy = scene.entities[scene.entityData[duplicated].handleIndex + i + 1];
	    entity_duplicate_recursive(scene, toCopy, copy);
	    i += scene.entityData[toCopy].childsCount;
	}

	return copy;
    }

    Entity duplicate_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	Entity res = entity_duplicate_recursive(scene, entity, scene.entityData[entity].parent);
	return res;
    }

    bool entity_is_empty(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return scene.entityData[entity].components.empty();
    }

    bool entity_exist(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	if (entity == SV_ENTITY_NULL || entity > scene.entityData.size) return false;

	EntityData& ed = scene.entityData[entity];
	return ed.handleIndex != u64_max;
    }

    const char* get_entity_name(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	SV_ASSERT(entity_exist(scene_, entity));
	const std::string& name = scene.entityData[entity].name;
	return name.empty() ? "Unnamed" : name.c_str();
    }

    void set_entity_name(Scene* scene_, Entity entity, const char* name)
    {
	PARSE_SCENE();
	SV_ASSERT(entity_exist(scene_, entity));
	scene.entityData[entity].name = name;
    }

    u32 get_entity_childs_count(Scene* scene_, Entity parent)
    {
	PARSE_SCENE();
	return scene.entityData[parent].childsCount;
    }

    void get_entity_childs(Scene* scene_, Entity parent, Entity const** childsArray)
    {
	PARSE_SCENE();

	const EntityData& ed = scene.entityData[parent];
	if (childsArray && ed.childsCount != 0)* childsArray = &scene.entities[ed.handleIndex + 1];
    }

    Entity get_entity_parent(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return scene.entityData[entity].parent;
    }

    Transform get_entity_transform(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return Transform(entity, &scene.entityData.get_transform(entity), scene_);
    }

    u32* get_entity_flags(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return &scene.entityData[entity].flags;
    }

    u32 get_entity_component_count(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return u32(scene.entityData[entity].components.size());
    }

    u32 get_entity_count(Scene* scene_)
    {
	PARSE_SCENE();
	return u32(scene.entities.size());
    }

    Entity get_entity_by_index(Scene* scene_, u32 index)
    {
	PARSE_SCENE();
	return scene.entities[index];
    }

    /////////////////////////////// COMPONENTS /////////////////////////////////////

    BaseComponent* add_component(Scene* scene_, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize)
    {
	PARSE_SCENE();

	comp = componentAlloc(scene_, componentID, comp, Entity(entity));
	ecs_entitydata_index_add(scene.entityData[entity], componentID, comp);

	return comp;
    }

    BaseComponent* add_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	BaseComponent* comp = componentAlloc(scene_, componentID, entity);
	ecs_entitydata_index_add(scene.entityData[entity], componentID, comp);

	return comp;
    }

    BaseComponent* get_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	return ecs_entitydata_index_get(scene.entityData[entity], componentID);
    }

    std::pair<CompID, BaseComponent*> get_component_by_index(Scene* scene_, Entity entity, u32 index)
    {
	PARSE_SCENE();
	return scene.entityData[entity].components[index];
    }

    void remove_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	EntityData& ed = scene.entityData[entity];
	BaseComponent* comp = ecs_entitydata_index_get(ed, componentID);

	componentFree(scene_, componentID, comp);
	ecs_entitydata_index_remove(ed, componentID);
    }

    u32 get_component_count(Scene* scene_, CompID ID)
    {
	return componentAllocatorCount(scene_, ID);
    }

    // Iterators

    ComponentIterator begin_component_iterator(Scene* scene_, CompID comp_id)
    {
	PARSE_SCENE();

	ComponentIterator iterator;
	iterator._scene = scene_;
	iterator.comp_id = comp_id;

	iterator._pool = 0u;
	u8* ptr = nullptr;

	if (!componentAllocatorIsEmpty(scene_, iterator.comp_id)) {

	    const ComponentAllocator& list = scene.components[iterator.comp_id];
	    size_t comp_size = size_t(get_component_size(iterator.comp_id));

	    while (componentPoolCount(list.pools[iterator._pool], comp_size) == 0u) iterator._pool++;

	    ptr = list.pools[iterator._pool].data;

	    while (true) {
		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
		if (comp->_id != 0u) {
		    break;
		}
		ptr += comp_size;
	    }

	}

	iterator._it = reinterpret_cast<BaseComponent*>(ptr);
	return iterator;
    }

    ComponentIterator end_component_iterator(Scene* scene_, CompID comp_id)
    {
	PARSE_SCENE();

	ComponentIterator iterator;
	iterator._scene = scene_;
	iterator.comp_id = comp_id;

	const ComponentAllocator& list = scene.components[iterator.comp_id];

	iterator._pool = u32(list.pools.size()) - 1u;
	u8* ptr = nullptr;

	if (!componentAllocatorIsEmpty(scene_, iterator.comp_id)) {

	    size_t compSize = size_t(get_component_size(iterator.comp_id));

	    while (componentPoolCount(list.pools[iterator._pool], compSize) == 0u) iterator._pool--;

	    ptr = list.pools[iterator._pool].data + list.pools[iterator._pool].size;
	}

	iterator._it = reinterpret_cast<BaseComponent*>(ptr);
	return iterator;
    }

    void ComponentIterator::get(Entity* entity, BaseComponent** comp)
    {
	*comp = _it;
	*entity = Entity(_it->_id);
    }

    bool ComponentIterator::equal(const ComponentIterator& other) const noexcept
    {
	return _it == other._it;
    }

    void ComponentIterator::next()
    {
	Scene_internal& scene = *reinterpret_cast<Scene_internal*>(_scene);

	auto& list = scene.components[comp_id];

	size_t comp_size = size_t(get_component_size(comp_id));
	u8* ptr = reinterpret_cast<u8*>(_it);
	u8* endPtr = list.pools[_pool].data + list.pools[_pool].size;

	do {
	    ptr += comp_size;
	    if (ptr == endPtr) {
		auto& list = scene.components[comp_id];

		do {

		    if (++_pool == list.pools.size()) break;

		} while (componentPoolCount(list.pools[_pool], comp_size) == 0u);

		if (_pool == list.pools.size()) break;

		ComponentPool& compPool = list.pools[_pool];

		ptr = compPool.data;
		endPtr = ptr + compPool.size;
	    }
	} while (reinterpret_cast<BaseComponent*>(ptr)->_id == 0u);

	_it = reinterpret_cast<BaseComponent*>(ptr);
    }

    void ComponentIterator::last()
    {
	// TODO:
	SV_LOG_ERROR("TODO-> This may fail");
	Scene_internal& scene = *reinterpret_cast<Scene_internal*>(_scene);

	auto& list = scene.components[comp_id];

	size_t compSize = size_t(get_component_size(comp_id));
	u8* ptr = reinterpret_cast<u8*>(_it);
	u8* beginPtr = list.pools[_pool].data;

	do {
	    ptr -= compSize;
	    if (ptr == beginPtr) {

		while (list.pools[--_pool].size == 0u);

		ComponentPool& compPool = list.pools[_pool];

		beginPtr = compPool.data;
		ptr = beginPtr + compPool.size;
	    }
	} while (reinterpret_cast<BaseComponent*>(ptr)->_id == 0u);

	_it = reinterpret_cast<BaseComponent*>(ptr);
    }

    //////////////////////////////////////////// TRANSFORM ///////////////////////////////////////////////////////////

#define parse() sv::EntityTransform* t = reinterpret_cast<sv::EntityTransform*>(trans)

    Transform::Transform(Entity entity, void* transform, Scene* scene)
	: entity(entity), trans(transform), scene(scene) {}

    const v3_f32& Transform::getLocalPosition() const noexcept
    {
	parse();
	return *(v3_f32*)& t->localPosition;
    }

    const v4_f32& Transform::getLocalRotation() const noexcept
    {
	parse();
	return *(v4_f32*)& t->localRotation;
    }

    v3_f32 Transform::getLocalEulerRotation() const noexcept
    {
	parse();
	v3_f32 euler;

	// roll (x-axis rotation)

	XMFLOAT4 q = t->localRotation;
	float sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
	euler.x = std::atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	float sinp = 2.f * (q.w * q.y - q.z * q.x);
	if (std::abs(sinp) >= 1.f)
	    euler.y = std::copysign(PI / 2.f, sinp); // use 90 degrees if out of range
	else
	    euler.y = std::asin(sinp);

	// yaw (z-axis rotation)
	float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	euler.z = std::atan2(siny_cosp, cosy_cosp);

	if (euler.x < 0.f) {
	    euler.x = 2.f * PI + euler.x;
	}
	if (euler.y < 0.f) {
	    euler.y = 2.f * PI + euler.y;
	}
	if (euler.z < 0.f) {
	    euler.z = 2.f * PI + euler.z;
	}

	return euler;
    }

    const v3_f32& Transform::getLocalScale() const noexcept
    {
	parse();
	return *(v3_f32*)& t->localScale;
    }

    XMVECTOR Transform::getLocalPositionDXV() const noexcept
    {
	parse();
	return XMLoadFloat3(&t->localPosition);
    }

    XMVECTOR Transform::getLocalRotationDXV() const noexcept
    {
	parse();
	return XMLoadFloat4(&t->localRotation);
    }

    XMVECTOR Transform::getLocalScaleDXV() const noexcept
    {
	parse();
	return XMLoadFloat3(&t->localScale);
    }

    XMMATRIX Transform::getLocalMatrix() const noexcept
    {
	parse();
	return XMMatrixScalingFromVector(getLocalScaleDXV()) * XMMatrixRotationQuaternion(XMLoadFloat4(&t->localRotation))
	    * XMMatrixTranslation(t->localPosition.x, t->localPosition.y, t->localPosition.z);
    }

    v3_f32 Transform::getWorldPosition() noexcept
    {
	parse();
	if (t->modified) updateWorldMatrix();
	return *(v3_f32*)& t->worldMatrix._41;
    }

    v4_f32 Transform::getWorldRotation() noexcept
    {
	parse();
	if (t->modified) updateWorldMatrix();
	return *(v4_f32*)& getWorldRotationDXV();
    }

    v3_f32 Transform::getWorldEulerRotation() noexcept
    {
	XMFLOAT4X4 rm;
	XMStoreFloat4x4(&rm, XMMatrixTranspose(XMMatrixRotationQuaternion(getWorldRotationDXV())));

	v3_f32 euler;

	if (rm._13 < 1.f) {
	    if (rm._13 > -1.f) {
		euler.y = asin(rm._13);
		euler.x = atan2(-rm._23, rm._33);
		euler.z = atan2(-rm._12, rm._11);
	    }
	    else {
		euler.y = -PI / 2.f;
		euler.x = -atan2(rm._21, rm._22);
		euler.z = 0.f;
	    }
	}
	else {
	    euler.y = PI / 2.f;
	    euler.x = atan2(rm._21, rm._22);
	    euler.z = 0.f;
	}

	if (euler.x < 0.f) {
	    euler.x = 2 * PI + euler.x;
	}
	if (euler.z < 0.f) {
	    euler.z = 2 * PI + euler.z;
	}

	return euler;
    }

    v3_f32 Transform::getWorldScale() noexcept
    {
	parse();
	if (t->modified) updateWorldMatrix();
	return { (*(v3_f32*)& t->worldMatrix._11).length(), (*(v3_f32*)& t->worldMatrix._21).length(), (*(v3_f32*)& t->worldMatrix._31).length() };
    }

    XMVECTOR Transform::getWorldPositionDXV() noexcept
    {
	parse();
	if (t->modified) updateWorldMatrix();

	v3_f32 position = getWorldPosition();
	return XMVectorSet(position.x, position.y, position.z, 0.f);
    }

    XMVECTOR Transform::getWorldRotationDXV() noexcept
    {
	parse();
	if (t->modified) updateWorldMatrix();
	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t->worldMatrix));

	return rotation;
    }

    XMVECTOR Transform::getWorldScaleDXV() noexcept
    {
	parse();

	if (t->modified) updateWorldMatrix();
	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t->worldMatrix));

	return XMVectorAbs(scale);
    }

    XMMATRIX Transform::getWorldMatrix() noexcept
    {
	parse();

	if (t->modified) updateWorldMatrix();
	return XMLoadFloat4x4(&t->worldMatrix);
    }

    XMMATRIX Transform::getParentMatrix() const noexcept
    {
	Scene_internal& s = *reinterpret_cast<Scene_internal*>(scene);
	EntityData& entityData = s.entityData[entity];
	Entity parent = entityData.parent;

	if (parent) {
	    Transform parentTransform(parent, &s.entityData.get_transform(parent), scene);
	    return parentTransform.getWorldMatrix();
	}
	else return XMMatrixIdentity();
    }

    void Transform::setPosition(const v3_f32& position) noexcept
    {
	notify();

	parse();
	t->localPosition = *(XMFLOAT3*)& position;
    }

    void Transform::setPositionX(float x) noexcept
    {
	notify();

	parse();
	t->localPosition.x = x;
    }

    void Transform::setPositionY(float y) noexcept
    {
	notify();

	parse();
	t->localPosition.y = y;
    }

    void Transform::setPositionZ(float z) noexcept
    {
	notify();

	parse();
	t->localPosition.z = z;
    }

    void Transform::setRotation(const v4_f32& rotation) noexcept
    {
	notify();

	parse();
	t->localRotation = *(XMFLOAT4*)& rotation;
    }

    void Transform::setEulerRotation(const v3_f32& r) noexcept
    {
	notify();

	float cy = cos(r.z * 0.5f);
	float sy = sin(r.z * 0.5f);
	float cp = cos(r.y * 0.5f);
	float sp = sin(r.y * 0.5f);
	float cr = cos(r.x * 0.5f);
	float sr = sin(r.x * 0.5f);

	v4_f32 q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = sr * cp * cy - cr * sp * sy;
	q.y = cr * sp * cy + sr * cp * sy;
	q.z = cr * cp * sy - sr * sp * cy;

	parse();
	XMStoreFloat4(&t->localRotation, XMVectorSet(q.x, q.y, q.z, q.w));
    }

    void Transform::setRotationX(float x) noexcept
    {
	notify();

	parse();
	t->localRotation.x = x;
    }

    void Transform::setRotationY(float y) noexcept
    {
	notify();

	parse();
	t->localRotation.y = y;
    }

    void Transform::setRotationZ(float z) noexcept
    {
	notify();

	parse();
	t->localRotation.z = z;
    }

    void Transform::setRotationW(float w) noexcept
    {
	notify();

	parse();
	t->localRotation.w = w;
    }

    void Transform::rotateRollPitchYaw(f32 roll, f32 pitch, f32 yaw)
    {
	notify();
	parse();

	XMVECTOR quaternion = XMLoadFloat4(&t->localRotation);
	XMVECTOR roll_quat = XMQuaternionRotationAxis(XMVectorSet(0.f, 0.f, 1.f, 0.f), roll);
	XMVECTOR pitch_quat = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), pitch);
	XMVECTOR yaw_quat = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), yaw);

	// TODO
    }

    void Transform::setScale(const v3_f32& scale) noexcept
    {
	parse();

	notify();
	t->localScale = *(XMFLOAT3*)& scale;
    }

    void Transform::setScaleX(float x) noexcept
    {
	notify();

	parse();
	t->localScale.x = x;
    }

    void Transform::setScaleY(float y) noexcept
    {
	notify();

	parse();
	t->localScale.y = y;
    }

    void Transform::setScaleZ(float z) noexcept
    {
	notify();

	parse();
	t->localScale.z = z;
    }

    void Transform::setMatrix(const XMMATRIX& matrix) noexcept
    {
	notify();
	parse();

	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, matrix);

	XMStoreFloat3(&t->localScale, scale);
	XMStoreFloat4(&t->localRotation, rotation);
	XMStoreFloat3(&t->localPosition, position);
    }

    void Transform::updateWorldMatrix()
    {
	parse();
	Scene_internal& s = *reinterpret_cast<Scene_internal*>(scene);

	t->modified = false;

	XMMATRIX m = getLocalMatrix();

	EntityData& entityData = s.entityData[entity];
	Entity parent = entityData.parent;

	if (parent != SV_ENTITY_NULL) {
	    Transform parentTransform(parent, &s.entityData.get_transform(parent), scene);
	    XMMATRIX mp = parentTransform.getWorldMatrix();
	    m = m * mp;
	}
	XMStoreFloat4x4(&t->worldMatrix, m);
    }

    void Transform::notify()
    {
	parse();

	if (!t->modified) {

	    t->modified = true;

	    Scene_internal& s = *reinterpret_cast<Scene_internal*>(scene);
	    auto& list = s.entityData;
	    EntityData& entityData = list[entity];

	    if (entityData.childsCount == 0) return;

	    auto& entities = s.entities;
	    for (u32 i = 0; i < entityData.childsCount; ++i) {
		EntityTransform& et = list.get_transform(entities[entityData.handleIndex + 1 + i]);
		et.modified = true;
	    }

	}
    }

    //////////////////////////////////////////// COMPONENTS ////////////////////////////////////////////////////////

    void SpriteComponent::serialize(Archive& archive)
    {
	archive << texture << texcoord << color << layer;
    }

    void SpriteComponent::deserialize(u32 version, Archive& archive)
    {
	archive >> texture >> texcoord >> color >> layer;
    }

    void CameraComponent::serialize(Archive& archive)
    {
	archive << projection_type << near << far << width << height;
    }

    void CameraComponent::deserialize(u32 version, Archive& archive)
    {
	archive >> (u32&)projection_type >> near >> far >> width >> height;
    }

    void MeshComponent::serialize(Archive& archive)
    {
	archive << mesh << material;
    }

    void MeshComponent::deserialize(u32 version, Archive& archive)
    {
	archive >> mesh >> material;
    }

    void LightComponent::serialize(Archive& archive)
    {
	archive << light_type << color << intensity << range << smoothness;
    }

    void LightComponent::deserialize(u32 version, Archive& archive)
    {
	archive >> (u32&)light_type >> color >> intensity >> range >> smoothness;
    }

}