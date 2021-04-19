#include "core/scene.h"
#include "core/renderer.h"

#include "debug/console.h"

#define PREFAB_COMPONENT_FLAG SV_BIT(31ULL)

namespace sv {

    CompID SpriteComponent::ID = SV_COMPONENT_ID_INVALID;
    CompID CameraComponent::ID = SV_COMPONENT_ID_INVALID;
    CompID MeshComponent::ID = SV_COMPONENT_ID_INVALID;
    CompID LightComponent::ID = SV_COMPONENT_ID_INVALID;
    CompID BodyComponent::ID = SV_COMPONENT_ID_INVALID;
    
    struct EntityInternal {

	size_t handleIndex = u64_max;
	Entity parent = SV_ENTITY_NULL;
	u32 childsCount = 0u;
	List<CompRef> components;
	u64 flags = 0u;
	std::string name;

    };

    struct EntityDataAllocator {

	EntityInternal* internal = nullptr;
	Transform* transforms = nullptr;

	u32 size = 0u;
	u32 capacity = 0u;
	List<Entity> freelist;

	EntityInternal& getInternal(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL && entity <= size); return internal[entity - 1u]; }
	Transform& getTransform(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL && entity <= size); return transforms[entity - 1u]; }
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
    
    SV_INTERNAL void entity_clear(EntityDataAllocator& allocator);

    void componentPoolAlloc(ComponentPool& pool, size_t compSize);             // Allocate components memory
    void componentPoolFree(ComponentPool& pool);                               // Deallocate components memory
    void* componentPoolGetPtr(ComponentPool& pool, size_t compSize);	       // Return new component
    void componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr); // Remove component
    bool componentPoolFull(const ComponentPool& pool, size_t compSize);	       // Check if there are free space in the pool
    bool componentPoolPtrExist(const ComponentPool& pool, void* ptr);	       // Check if the pool contains the ptr
    u32	componentPoolCount(const ComponentPool& pool, size_t compSize);	       // Return the number of valid components allocated in this pool

    void componentAllocatorCreate(Scene* scene, CompID ID);						// Create the allocator
    void componentAllocatorDestroy(Scene* scene, CompID ID);						// Destroy the allocator
    BaseComponent* componentAlloc(Scene* scene, CompID compID, Entity entity, bool create = true);	// Allocate and create new component
    BaseComponent* componentAlloc(Scene* scene, CompID compID, BaseComponent* srcComp, Entity entity);	// Allocate and copy new component
    void componentFree(Scene* scene, CompID compID, BaseComponent* comp);				// Free and destroy component
    u32	componentAllocatorCount(Scene* scene, CompID compId);						// Return the number of valid components in all the pools
    bool componentAllocatorIsEmpty(Scene* scene, CompID compID);					// Return if the allocator is empty

    bool initialize_scene(Scene** pscene, const char* name)
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

	    sprintf(scene.name, "%s", name);
	}

	SV_CHECK(gui_create(hash_string(name), &scene.gui));
	//SV_CHECK(create_audio_device(&scene.audio_device));

	bool deserialize = false;
	Archive archive;

	// Deserialize
	{
	    // Get filepath
	    char filepath[FILEPATH_SIZE];

	    bool exist = user_get_scene_filepath(name, filepath);

	    if (exist) {

		bool res = archive.openFile(filepath);

		if (!res) {

		    SV_LOG_ERROR("Can't deserialize the scene '%s' at '%s'", name, filepath);
		}
		else {
		    u32 version;
		    archive >> version;
		    archive >> scene.main_camera;

		    archive >> scene.gravity;
		    archive >> scene.air_friction;

		    // ECS
		    {
			scene.entities.clear();
			entity_clear(scene.entityData);

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
				return false;
			    }

			}

			// Entity data
			u32 entityCount;
			u32 entityDataCount;

			archive >> entityCount;
			archive >> entityDataCount;

			scene.entities.resize(entityCount);
			EntityInternal* entity_internal = new EntityInternal[entityDataCount];
			Transform* entity_transform = new Transform[entityDataCount];

			for (u32 i = 0; i < entityCount; ++i) {

			    Entity entity;
			    archive >> entity;

			    EntityInternal& ed = entity_internal[entity];
			    Transform& et = entity_transform[entity];

			    archive >> ed.name;

			    archive >> ed.childsCount >> ed.handleIndex >> et.position >> et.rotation >> et.scale >> ed.flags;
			}

			// Create entity list and free list
			for (u32 i = 0u; i < entityDataCount; ++i) {

			    EntityInternal& ed = entity_internal[i];

			    if (ed.handleIndex != u64_max) {
				scene.entities[ed.handleIndex] = i + 1u;
			    }
			    else scene.entityData.freelist.push_back(i + 1u);
			}

			// Set entity parents
			{
			    EntityInternal* it = entity_internal;
			    EntityInternal* end = it + entityDataCount;

			    while (it != end) {

				if (it->handleIndex != u64_max && it->childsCount) {

				    Entity* eIt = scene.entities.data() + it->handleIndex;
				    Entity* eEnd = eIt + it->childsCount;
				    Entity parent = *eIt++;

				    while (eIt <= eEnd) {

					EntityInternal& ed = entity_internal[*eIt - 1u];
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
			scene.entityData.internal = entity_internal;
			scene.entityData.transforms = entity_transform;
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
				    u32 poolCount = compCount / ECS_COMPONENT_POOL_SIZE + ((compCount % ECS_COMPONENT_POOL_SIZE == 0u) ? 0u : 1u);
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

				    scene.entityData.getInternal(entity).components.push_back({compID, comp});
				}

			    }
			}

			deserialize = true;
		    }
		}
	    }
	}

	// User Init
	if (!user_initialize_scene(scene_, deserialize ? &archive : nullptr)) {
	    // TODO: handle error
	    return false;
	}
	
	*pscene = scene_;
	return true;
    }

    bool close_scene(Scene* scene_)
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
	    entity_clear(scene.entityData);
	}
		
	return true;
    }

    bool set_active_scene(const char* name)
    {
	size_t name_size = strlen(name);

	if (name_size > SCENENAME_SIZE) {
	    SV_LOG_ERROR("The scene name '%s' is to long, max chars = %u", name, SCENENAME_SIZE);
	    return false;
	}
	
	// validate scene
	if (user_validate_scene(name)) {
	    memcpy(engine.next_scene_name, name, name_size + 1u);
	    return true;
	}
	return false;
    }

    SV_API bool save_scene(Scene* scene)
    {
	char filepath[FILEPATH_SIZE];
	if (user_get_scene_filepath(scene->name, filepath)) {

	    return save_scene(scene, filepath);
	}
	else return false;
    }

    bool save_scene(Scene* scene_, const char* filepath)
    {
	PARSE_SCENE();

	Archive archive;

	archive << Scene::VERSION;
	archive << scene.main_camera;

	archive << scene.gravity;
	archive << scene.air_friction;
		
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
		    EntityInternal& ed = scene.entityData.getInternal(entity);

		    if (ed.handleIndex != u64_max) {

			Transform& transform = scene.entityData.getTransform(entity);
			
			archive << i << ed.name << ed.childsCount << ed.handleIndex << transform.position << transform.rotation << transform.scale << ed.flags;
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

    bool clear_scene(Scene* scene_)
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
	entity_clear(scene.entityData);

	// user initialize scene
	SV_CHECK(user_initialize_scene(scene_, nullptr));

	return true;
    }

    bool create_entity_model(Scene* scene_, Entity parent, const char* folderpath)
    {
	//PARSE_SCENE();
		
	FolderIterator it;
	FolderElement element;

	bool res = folder_iterator_begin(folderpath, &it, &element);

	if (res) {
	    
	    do {

		if (!element.is_file)
		    continue;

		if (element.extension && strcmp(element.extension, "mesh") == 0) {

		    char filepath[FILEPATH_SIZE];
		    sprintf(filepath, "%s/%s", folderpath, element.name);
		    SV_LOG("%s", filepath);

		    MeshAsset mesh;

		    bool res = load_asset_from_file(mesh, filepath);

		    if (res) {
					
			Entity entity = create_entity(scene_, parent);
			MeshComponent* comp = add_component<MeshComponent>(scene_, entity);
			comp->mesh = mesh;

			Mesh* m = mesh.get();

			set_entity_matrix(scene_, entity, m->model_transform_matrix);

			if (m->model_material_filepath.size()) {
					
			    MaterialAsset mat;
			    res = load_asset_from_file(mat, m->model_material_filepath.c_str());

			    if (res) {
							
				comp->material = mat;
			    }
			}
		    }
		}
		
	    } while(folder_iterator_next(&it, &element));

	    folder_iterator_close(&it);
	    
	}
	else return res;
	
	return true;
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

    void update_physics()
    {
	f32 dt = engine.deltatime;

	Scene* scene_ = engine.scene;
	Scene& scene = *engine.scene;
    
	EntityView<BodyComponent> bodies(scene_);
	    
	for (ComponentView<BodyComponent> view : bodies) {

	    BodyComponent& body = *view.comp;
		
	    if (body.body_type == BodyType_Static)
		continue;

	    v2_f32 position = get_entity_position2D(scene_, view.entity) + body.offset;
	    v2_f32 scale = get_entity_scale2D(scene_, view.entity) * body.size;

	    // Reset values
	    body.in_ground = false;
	
	    // Gravity
	    body.vel += scene.gravity * dt;
	
	    BodyComponent* vertical_collision = nullptr;
	    f32 vertical_depth = f32_max;
	
	    BodyComponent* horizontal_collision = nullptr;
	    f32 horizontal_depth = f32_max;
	    f32 vertical_offset = 0.f; // Used to avoid the small bumps

	    v2_f32 next_pos = position;
	    v2_f32 next_vel = body.vel;
	
	    // Simulate collisions
	    {
		// Detection

		v2_f32 final_pos = position + body.vel * dt;

		v2_f32 step = final_pos - next_pos;
		f32 adv = SV_MIN(scale.x, scale.y);
		u32 step_count = SV_MIN(u32(step.length() / adv) + 1u, 5u);

		step /= f32(step_count);

		foreach(i, step_count) {

		    next_pos += step;
	    
		    EntityView<BodyComponent> bodies0(scene_);
		    for (ComponentView<BodyComponent> v : bodies0) {

			BodyComponent& b = *v.comp;

			if (b.body_type == BodyType_Static) {

			    v2_f32 p = get_entity_position2D(scene_, v.entity) + b.offset;
			    v2_f32 s = get_entity_scale2D(scene_, v.entity) * b.size;

			    v2_f32 to = p - next_pos;
			    to.x = abs(to.x);
			    to.y = abs(to.y);
			
			    v2_f32 min_distance = (s + scale) * 0.5f;
			    min_distance.x = abs(min_distance.x);
			    min_distance.y = abs(min_distance.y);
			
			    if (to.x < min_distance.x && to.y < min_distance.y) {

				v2_f32 depth = min_distance - to;

				// Vertical collision
				if (depth.x > depth.y) {

				    if (depth.y < abs(vertical_depth)) {
			    
					vertical_collision = &b;
					vertical_depth = depth.y * ((next_pos.y < p.y) ? -1.f : 1.f);
				    }
				}

				// Horizontal collision
				else {

				    if (depth.x < abs(horizontal_depth)) {
				    
					horizontal_collision = &b;
					horizontal_depth = depth.x * ((next_pos.x < p.x) ? -1.f : 1.f);

					if (depth.y < scale.y * 0.05f && next_pos.y > p.y) {
					    vertical_offset = depth.y;
					}
				    }
				}
			    }
			}
		    }

		    if (vertical_collision || horizontal_collision)
			break;
		}

		// Solve collisions
		if (vertical_collision) {

		    if (vertical_depth > 0.f) {

			body.in_ground = true;
			// Ground friction
			next_vel.x *= pow(1.f - vertical_collision->friction, dt);
		    }

		    next_pos.y += vertical_depth;
		    next_vel.y = next_vel.y * -body.bounciness;

		    BodyCollisionEvent event;
		    event.body0 = &body;
		    event.body1 = vertical_collision;

		    event_dispatch("on_body_collision", event);
		}

		if (horizontal_collision) {
		
		    next_pos.x += horizontal_depth;
		    if (vertical_offset != 0.f)
			next_pos.y += vertical_offset;
		    else
			next_vel.x = next_vel.x * -body.bounciness;

		    BodyCollisionEvent event;
		    event.body0 = &body;
		    event.body1 = horizontal_collision;

		    event_dispatch("on_body_collision", event);
		}

		// Air friction
		next_vel *= pow(1.f - scene.air_friction, dt);
	    }
	
	    position = next_pos;
	    body.vel = next_vel;

	    set_entity_position2D(scene_, view.entity, position - body.offset);
	}
    }

    void _update_scene()
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

		v3_f32 position = get_entity_world_position(scene_, entity);
		v4_f32 rotation = get_entity_world_rotation(scene_, entity);

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

	update_physics();
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

    static void entity_clear(EntityDataAllocator& a)
    {
	if (a.internal) {
	    a.capacity = 0u;
	    delete[] a.internal;
	    a.internal = nullptr;
	    delete[] a.transforms;
	    a.transforms = nullptr;
	}

	a.size = 0u;
	a.freelist.clear();
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

    SV_INTERNAL void componentAllocatorCreate(Scene* scene_, CompID compID)
    {
	PARSE_SCENE();
	componentAllocatorDestroy(scene_, compID);
	componentAllocatorCreatePool(scene.components[compID], size_t(get_component_size(compID)));
    }

    SV_INTERNAL void componentAllocatorDestroy(Scene* scene_, CompID compID)
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

    SV_INTERNAL BaseComponent* componentAlloc(Scene* scene_, CompID compID, Entity entity, bool create)
    {
	PARSE_SCENE();

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
	BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

	if (create) create_component(scene_, compID, comp, entity);

	return comp;
    }

    SV_INTERNAL BaseComponent* componentAlloc(Scene* scene_, CompID compID, BaseComponent* srcComp, Entity entity)
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

    SV_INTERNAL void componentFree(Scene* scene_, CompID compID, BaseComponent* comp)
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

    SV_INTERNAL u32 componentAllocatorCount(Scene* scene_, CompID compID)
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

    SV_INTERNAL bool componentAllocatorIsEmpty(Scene* scene_, CompID compID)
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
	CompID id;
	
	// Check if is available
	{
	    if (desc->componentSize < sizeof(u32)) {
		SV_LOG_ERROR("Can't register a component type with size of %u", desc->componentSize);
		return SV_COMPONENT_ID_INVALID;
	    }

	    auto it = g_CompNames.find(desc->name);

	    if (it != g_CompNames.end()) {
		
		id = it->second;
	    }
	    else id = CompID(g_Registers.size());
	}

	ComponentRegister& reg = (id == CompID(g_Registers.size())) ? g_Registers.emplace_back() : g_Registers[id];
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

    void invalidate_component_callbacks(CompID id)
    {
	ComponentRegister& r = g_Registers[id];

	r.createFn = nullptr;
	r.destroyFn = nullptr;
	r.moveFn = nullptr;
	r.copyFn = nullptr;
	r.serializeFn = nullptr;
	r.deserializeFn = nullptr;
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

    void ecs_entitydata_index_add(EntityInternal& ed, CompID ID, BaseComponent* compPtr)
    {
	ed.components.push_back(CompRef{ ID, compPtr });
    }

    void ecs_entitydata_index_set(EntityInternal& ed, CompID ID, BaseComponent* compPtr)
    {
	for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
	    if (it->id == ID) {
		it->ptr = compPtr;
		return;
	    }
	}
    }

    BaseComponent* ecs_entitydata_index_get(EntityInternal& ed, CompID ID)
    {
	for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
	    if (it->id == ID) {
		return it->ptr;
	    }
	}
	return nullptr;
    }

    void ecs_entitydata_index_remove(EntityInternal& ed, CompID ID)
    {
	for (auto it = ed.components.begin(); it != ed.components.end(); ++it) {
	    if (it->id == ID) {
		ed.components.erase(it);
		return;
	    }
	}
    }

    SV_INLINE static void update_childs(Scene_internal& scene, Entity entity, i32 count)
    {
	Entity parent = entity;

	while (parent != SV_ENTITY_NULL) {
	    EntityInternal& parentToUpdate = scene.entityData.getInternal(parent);
	    parentToUpdate.childsCount += count;
	    parent = parentToUpdate.parent;
	}
    }

    ///////////////////////////////////// ENTITIES ////////////////////////////////////////

    Entity create_entity(Scene* scene_, Entity parent, const char* name)
    {
	PARSE_SCENE();

	Entity entity;

	{
	    auto& a = scene.entityData;
	
	    if (a.freelist.empty()) {

		if (a.size == a.capacity) {
		    EntityInternal* new_internal = new EntityInternal[a.capacity + ECS_ENTITY_ALLOC_SIZE];
		    Transform* new_transforms = new Transform[a.capacity + ECS_ENTITY_ALLOC_SIZE];

		    if (a.internal) {

			{
			    EntityInternal* end = a.internal + a.capacity;
			    while (a.internal != end) {

				*new_internal = std::move(*a.internal);

				++new_internal;
				++a.internal;
			    }
			}
			
			memcpy(new_transforms, a.transforms, a.capacity * sizeof(Transform));
			
			a.internal -= a.capacity;
			new_internal -= a.capacity;
			delete[] a.internal;
			delete[] a.transforms;
		    }

		    a.capacity += ECS_ENTITY_ALLOC_SIZE;
		    a.internal = new_internal;
		    a.transforms = new_transforms;
		}

		entity = ++a.size;

	    }
	    else {
		Entity result = a.freelist.back();
		a.freelist.pop_back();
		entity = result;
	    }
	}

	if (parent == SV_ENTITY_NULL) {
	    scene.entityData.getInternal(entity).handleIndex = scene.entities.size();
	    scene.entities.emplace_back(entity);
	}
	else {
	    update_childs(scene, parent, 1u);

	    // Set parent and handleIndex
	    EntityInternal& parentData = scene.entityData.getInternal(parent);
	    size_t index = parentData.handleIndex + size_t(parentData.childsCount);

	    EntityInternal& entityData = scene.entityData.getInternal(entity);
	    entityData.handleIndex = index;
	    entityData.parent = parent;

	    // Special case, the parent and childs are in back of the list
	    if (index == scene.entities.size()) {
		scene.entities.emplace_back(entity);
	    }
	    else {
		scene.entities.insert(scene.entities.begin() + index, entity);

		for (size_t i = index + 1; i < scene.entities.size(); ++i) {
		    scene.entityData.getInternal(scene.entities[i]).handleIndex++;
		}
	    }
	}

	if (name)
	    scene.entityData.getInternal(entity).name = name;

	return entity;
    }

    void destroy_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	EntityInternal& entityData = scene.entityData.getInternal(entity);

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

	    { // Free the entity memory
		auto& a = scene.entityData;
		
		SV_ASSERT(a.size >= entity);
		a.getInternal(entity) = EntityInternal();
		a.getTransform(entity) = Transform();

		if (entity == a.size) {
		    a.size--;
		}
		else {
		    a.freelist.push_back(entity);
		}
	    }
	}

	// remove from entities & update indices
	if (cpyCant != 0) memcpy(&scene.entities[indexBeginDest], &scene.entities[indexBeginSrc], cpyCant * sizeof(Entity));
	scene.entities.resize(scene.entities.size() - count);
	for (size_t i = indexBeginDest; i < scene.entities.size(); ++i) {
	    scene.entityData.getInternal(scene.entities[i]).handleIndex = i;
	}
    }

    void clear_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	EntityInternal& ed = scene.entityData.getInternal(entity);
	while (!ed.components.empty()) {

	    CompRef ref = ed.components.back();
	    ed.components.pop_back();

	    componentFree(scene_, ref.id, ref.ptr);
	}
    }

    static Entity entity_duplicate_recursive(Scene_internal& scene, Entity duplicated, Entity parent)
    {
	Scene* scene_ = reinterpret_cast<Scene*>(&scene);
	Entity copy;

	copy = create_entity(scene_, parent);

	EntityInternal& duplicatedEd = scene.entityData.getInternal(duplicated);
	EntityInternal& copyEd = scene.entityData.getInternal(copy);

	copyEd.name = duplicatedEd.name;

	scene.entityData.getTransform(copy) = scene.entityData.getTransform(duplicated);
	copyEd.flags = duplicatedEd.flags;

	for (u32 i = 0; i < duplicatedEd.components.size(); ++i) {
	    CompID ID = duplicatedEd.components[i].id;

	    BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
	    BaseComponent* newComp = componentAlloc(scene_, ID, comp, copy);

	    ecs_entitydata_index_add(copyEd, ID, newComp);
	}

	for (u32 i = 0; i < scene.entityData.getInternal(duplicated).childsCount; ++i) {
	    Entity toCopy = scene.entities[scene.entityData.getInternal(duplicated).handleIndex + i + 1];
	    entity_duplicate_recursive(scene, toCopy, copy);
	    i += scene.entityData.getInternal(toCopy).childsCount;
	}

	return copy;
    }

    Entity duplicate_entity(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();

	Entity res = entity_duplicate_recursive(scene, entity, scene.entityData.getInternal(entity).parent);
	return res;
    }

    bool entity_is_empty(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return scene.entityData.getInternal(entity).components.empty();
    }

    bool entity_exist(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	if (entity == SV_ENTITY_NULL || entity > scene.entityData.size) return false;

	EntityInternal& ed = scene.entityData.getInternal(entity);
	return ed.handleIndex != u64_max;
    }

    const char* get_entity_name(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	SV_ASSERT(entity_exist(scene_, entity));
	const std::string& name = scene.entityData.getInternal(entity).name;
	return name.empty() ? "Unnamed" : name.c_str();
    }

    void set_entity_name(Scene* scene_, Entity entity, const char* name)
    {
	PARSE_SCENE();
	SV_ASSERT(entity_exist(scene_, entity));
	scene.entityData.getInternal(entity).name = name;
    }

    u32 get_entity_childs_count(Scene* scene_, Entity parent)
    {
	PARSE_SCENE();
	return scene.entityData.getInternal(parent).childsCount;
    }

    void get_entity_childs(Scene* scene_, Entity parent, Entity const** childsArray)
    {
	PARSE_SCENE();

	const EntityInternal& ed = scene.entityData.getInternal(parent);
	if (childsArray && ed.childsCount != 0)* childsArray = &scene.entities[ed.handleIndex + 1];
    }

    Entity get_entity_parent(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return scene.entityData.getInternal(entity).parent;
    }

    u64* get_entity_flags(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return &scene.entityData.getInternal(entity).flags;
    }

    u32 get_entity_component_count(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	return u32(scene.entityData.getInternal(entity).components.size());
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
	ecs_entitydata_index_add(scene.entityData.getInternal(entity), componentID, comp);

	return comp;
    }

    BaseComponent* add_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	BaseComponent* comp = componentAlloc(scene_, componentID, entity);
	ecs_entitydata_index_add(scene.entityData.getInternal(entity), componentID, comp);

	return comp;
    }

    BaseComponent* get_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	return ecs_entitydata_index_get(scene.entityData.getInternal(entity), componentID);
    }

    CompRef get_component_by_index(Scene* scene_, Entity entity, u32 index)
    {
	PARSE_SCENE();
	return scene.entityData.getInternal(entity).components[index];
    }

    void remove_component_by_id(Scene* scene_, Entity entity, CompID componentID)
    {
	PARSE_SCENE();

	EntityInternal& ed = scene.entityData.getInternal(entity);
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

    SV_AUX void update_world_matrix(Scene_internal& s, Transform& t, Entity entity)
    {
	t._modified = false;

	XMMATRIX m = get_entity_matrix((Scene*)&s, entity);

	EntityInternal& entityData = s.entityData.getInternal(entity);
	Entity parent = entityData.parent;

	if (parent != SV_ENTITY_NULL) {
	    
	    XMMATRIX mp = get_entity_world_matrix((Scene*)&s, entity);
	    m = m * mp;
	}
	
	XMStoreFloat4x4(&t.world_matrix, m);
    }

    SV_AUX void notify_transform(Scene_internal& s, Transform& t, Entity entity)
    {
	if (!t._modified) {

	    t._modified = true;

	    auto& list = s.entityData;
	    EntityInternal& entityData = list.getInternal(entity);

	    if (entityData.childsCount == 0) return;

	    auto& entities = s.entities;
	    for (u32 i = 0; i < entityData.childsCount; ++i) {
		Transform& et = list.getTransform(entities[entityData.handleIndex + 1 + i]);
		et._modified = true;
	    }
	}
    }

    void set_entity_transform(Scene* scene_, Entity entity, const Transform& transform)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	t = transform;
	notify_transform(scene, t, entity);
    }
    
    void set_entity_position(Scene* scene_, Entity entity, const v3_f32& position)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	t.position = position;
	notify_transform(scene, t, entity);
    }
    
    void set_entity_rotation(Scene* scene_, Entity entity, const v4_f32& rotation)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	t.rotation = rotation;
	notify_transform(scene, t, entity);
    }
    
    void set_entity_scale(Scene* scene_, Entity entity, const v3_f32& scale)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	t.scale = scale;
	notify_transform(scene, t, entity);
    }
    
    void set_entity_matrix(Scene* scene_, Entity entity, const XMMATRIX& matrix)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	
	notify_transform(scene, t, entity);

	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, matrix);

	t.position.setDX(position);
	t.scale.setDX(scale);
	t.rotation.set_dx(rotation);
    }

    void set_entity_position2D(Scene* scene_, Entity entity, const v2_f32& position)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	t.position.x = position.x;
	t.position.y = position.y;
	notify_transform(scene, t, entity);
    }
    
    Transform* get_entity_transform_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return &t;
    }
    
    v3_f32* get_entity_position_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return &t.position;
    }
    
    v4_f32* get_entity_rotation_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return &t.rotation;
    }
    
    v3_f32* get_entity_scale_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return &t.scale;
    }
    
    v2_f32* get_entity_position2D_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return reinterpret_cast<v2_f32*>(&t.position);
    }
    
    v2_f32* get_entity_scale2D_ptr(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	notify_transform(scene, t, entity);

	return reinterpret_cast<v2_f32*>(&t.scale);
    }
    
    const Transform& get_entity_transform(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t;
    }
    
    v3_f32 get_entity_position(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t.position;
    }
    
    v4_f32 get_entity_rotation(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t.rotation;
    }
    
    v3_f32 get_entity_scale(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t.scale;
    }
    
    v2_f32 get_entity_position2D(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t.position.getVec2();
    }
    
    v2_f32 get_entity_scale2D(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	return t.scale.getVec2();
    }

    XMMATRIX get_entity_matrix(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);

	return XMMatrixScalingFromVector(t.scale.getDX()) * XMMatrixRotationQuaternion(t.rotation.get_dx())
	    * XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
    }
    
    v3_f32 get_entity_world_position(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(scene, t, entity);
	return *(v3_f32*) &t.world_matrix._41;
    }
    
    v4_f32 get_entity_world_rotation(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(scene, t, entity);
	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t.world_matrix));

	return v4_f32(rotation);
    }
    
    v3_f32 get_entity_world_scale(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(scene, t, entity);
	return { (*(v3_f32*)& t.world_matrix._11).length(), (*(v3_f32*)& t.world_matrix._21).length(), (*(v3_f32*)& t.world_matrix._31).length() };
    }
    
    XMMATRIX get_entity_world_matrix(Scene* scene_, Entity entity)
    {
	PARSE_SCENE();
	Transform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(scene, t, entity);
	return XMLoadFloat4x4(&t.world_matrix);
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

    void BodyComponent::serialize(Archive& archive)
    {
	archive << body_type << size << offset << mass << friction << bounciness;
    }

    void BodyComponent::deserialize(u32 version, Archive& archive)
    {
	archive >> (u32&)body_type >> size >> offset >> mass >> friction >> bounciness;
    }
}
