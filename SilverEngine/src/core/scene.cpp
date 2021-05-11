#include "core/scene.h"
#include "core/renderer.h"

#include "debug/console.h"

#include "user.h"

#define PREFAB_COMPONENT_FLAG SV_BIT(31ULL)
#define SV_SCENE() sv::Scene& scene = *scene_state->scene;

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

    struct EntityTransform {

	// Local space
	
	v3_f32 position = { 0.f, 0.f, 0.f };
	v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
	v3_f32 scale = { 1.f, 1.f, 1.f };

	// World space
	
	XMFLOAT4X4 world_matrix;

	bool _modified = true;

    };

    struct EntityDataAllocator {

	EntityInternal* internal = nullptr;
	EntityTransform* transforms = nullptr;

	u32 size = 0u;
	u32 capacity = 0u;
	List<Entity> freelist;

	EntityInternal& getInternal(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL && entity <= size); return internal[entity - 1u]; }
	EntityTransform& getTransform(Entity entity) { SV_ASSERT(entity != SV_ENTITY_NULL && entity <= size); return transforms[entity - 1u]; }
    };

    struct ComponentPool {

	u8* data;
	size_t		size;
	size_t		freeCount;

    };

    struct ComponentAllocator {

	List<ComponentPool> pools;

    };

    struct ComponentRegister {

	// TODO: get out of here
	std::string		        name;
	u32				size;
	u32                             version;
	CreateComponentFunction		createFn = nullptr;
	DestroyComponentFunction	destroyFn = nullptr;
	MoveComponentFunction		moveFn = nullptr;
	CopyComponentFunction		copyFn = nullptr;
	SerializeComponentFunction	serializeFn = nullptr;
	DeserializeComponentFunction	deserializeFn = nullptr;

    };

    struct Scene {

	SceneData data;
	
	char name[SCENENAME_SIZE + 1u] = {};

	// ECS
	
	// TODO: Use List
	std::vector<Entity>	 entities;
	EntityDataAllocator	 entityData;
	List<ComponentAllocator> components;
	
    };

    struct BodyCollision {
	BodyComponent* b0;
	BodyComponent* b1;
	Entity e0;
	Entity e1;
	bool leave;
	bool trigger;
    };
    
    struct SceneState {

	static constexpr u32 VERSION = 0u;

	char next_scene_name[SCENENAME_SIZE + 1u] = {};
	Scene* scene = nullptr;

	List<ComponentRegister> registers;
	// TODO: get out of here
	std::unordered_map<std::string, CompID> component_names;

	// Physics

	List<BodyCollision> last_collisions;
	List<BodyCollision> current_collisions;
    };

    static SceneState* scene_state = nullptr;

    constexpr u32 ECS_COMPONENT_POOL_SIZE = 100u;
    constexpr u32 ECS_ENTITY_ALLOC_SIZE = 100u;
    
    SV_INTERNAL void entity_clear(EntityDataAllocator& allocator);

    void componentPoolAlloc(ComponentPool& pool, size_t compSize);             // Allocate components memory
    void componentPoolFree(ComponentPool& pool);                               // Deallocate components memory
    void* componentPoolGetPtr(ComponentPool& pool, size_t compSize);	       // Return new component
    void componentPoolRmvPtr(ComponentPool& pool, size_t compSize, void* ptr); // Remove component
    bool componentPoolFull(const ComponentPool& pool, size_t compSize);	       // Check if there are free space in the pool
    bool componentPoolPtrExist(const ComponentPool& pool, void* ptr);	       // Check if the pool contains the ptr
    u32	componentPoolCount(const ComponentPool& pool, size_t compSize);	       // Return the number of valid components allocated in this pool

    void componentAllocatorCreate(CompID ID);						// Create the allocator
    void componentAllocatorDestroy(CompID ID);						// Destroy the allocator
    BaseComponent* componentAlloc(CompID compID, Entity entity, bool create = true);	// Allocate and create new component
    BaseComponent* componentAlloc(CompID compID, BaseComponent* srcComp, Entity entity);// Allocate and copy new component
    void componentFree(CompID compID, BaseComponent* comp);				// Free and destroy component
    u32	componentAllocatorCount(CompID compId);						// Return the number of valid components in all the pools
    bool componentAllocatorIsEmpty(CompID compID);					// Return if the allocator is empty

    void _initialize_scene()
    {
	scene_state = SV_ALLOCATE_STRUCT(SceneState);

	// TODO register engine components
    }

    SV_AUX void destroy_current_scene()
    {
	if (there_is_scene()) {

	    Scene*& scene = scene_state->scene;

	    event_dispatch("close_scene", nullptr);

	    // TODO: Dispatch events at once
	    for (Entity entity : scene->entities) {

		EntityDestroyEvent e;
		e.entity = entity;
		event_dispatch("on_entity_destroy", &e);
	    }

	    // Close ECS
	    {
		for (CompID i = 0; i < get_component_register_count(); ++i) {
		    if (component_exist(i))
			componentAllocatorDestroy(i);
		}
		scene->components.clear();
		scene->entities.clear();
		entity_clear(scene->entityData);
	    }

	    scene = nullptr;
	}
    }
    
    void _close_scene()
    {
	destroy_current_scene();
	unregister_components();

	SV_FREE_STRUCT(scene_state);
    }

    SV_INLINE void serialize_entity(Serializer& s, Entity entity)
    {
	serialize_u32(s, entity);
    }

    SV_INLINE void deserialize_entity(Deserializer& d, Entity& entity)
    {
	deserialize_u32(d, entity);
    }

    bool _start_scene(const char* name)
    {
	Scene*& scene_ptr = scene_state->scene;
	
	// Close last scene
	destroy_current_scene();

	if (name == nullptr)
	    return true;
	
	scene_ptr = SV_ALLOCATE_STRUCT(Scene);

	Scene& scene = *scene_ptr;

	// Init
	{
	    // Allocate components
	    scene.components.resize(scene_state->registers.size());

	    for (CompID id = 0u; id < scene_state->registers.size(); ++id) {

		componentAllocatorCreate(id);
	    }

	    strcpy(scene.name, name);
	}

	bool deserialize = false;
	Deserializer d;

	// Deserialize
	{
	    // Get filepath
	    char filepath[FILEPATH_SIZE];

	    bool exist = _user_get_scene_filepath(name, filepath);

	    if (exist) {

		bool res = deserialize_begin(d, filepath);

		if (!res) {

		    SV_LOG_ERROR("Can't deserialize the scene '%s' at '%s'", name, filepath);
		}
		else {
		    u32 scene_version;
		    deserialize_u32(d, scene_version);
		    
		    deserialize_entity(d, scene.data.main_camera);
		    deserialize_entity(d, scene.data.player);

		    deserialize_v2_f32(d, scene.data.gravity);
		    deserialize_f32(d, scene.data.air_friction);

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
			    deserialize_u32(d, registersCount);
			    registers.resize(registersCount);

			    for (auto it = registers.rbegin(); it != registers.rend(); ++it) {

				deserialize_string(d, it->name);				
				deserialize_u32(d, it->size);
				deserialize_u32(d, it->version);
			    }
			}

			// Registers ID
			CompID invalidCompID = CompID(scene_state->registers.size());
			for (u32 i = 0; i < registers.size(); ++i) {

			    Register& reg = registers[i];
			    reg.ID = invalidCompID;

			    auto it = scene_state->component_names.find(reg.name.c_str());
			    if (it != scene_state->component_names.end()) {
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

			deserialize_u32(d, entityCount);
			deserialize_u32(d, entityDataCount);

			scene.entities.resize(entityCount);
			EntityInternal* entity_internal = SV_ALLOCATE_STRUCT_ARRAY(EntityInternal, entityDataCount);
			EntityTransform* entity_transform = SV_ALLOCATE_STRUCT_ARRAY(EntityTransform, entityDataCount);

			for (u32 i = 0; i < entityCount; ++i) {

			    Entity entity;
			    deserialize_entity(d, entity);

			    EntityInternal& ed = entity_internal[entity];
			    EntityTransform& et = entity_transform[entity];

			    deserialize_string(d, ed.name);
			    deserialize_u32(d, ed.childsCount);
			    deserialize_size_t(d, ed.handleIndex);
			    deserialize_u64(d, ed.flags);
			    deserialize_v3_f32(d, et.position);
			    deserialize_v4_f32(d, et.rotation);
			    deserialize_v3_f32(d, et.scale);
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
				deserialize_u32(d, compCount);

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
						destroy_component(compID, comp);
					    }

					    it += compSize;
					}

					pool.size = 0u;
					pool.freeCount = 0u;
				    }
				}

				while (compCount-- != 0u) {

				    Entity entity;
				    deserialize_entity(d, entity);

				    BaseComponent* comp = componentAlloc(compID, entity, false);
				    create_component(compID, comp, entity);
				    deserialize_component(compID, comp, d, version);

				    scene.entityData.getInternal(entity).components.push_back({compID, comp});
				}
			    }
			}
		    }

		    deserialize_end(d);

		    deserialize = true;
		}
	    }
	}

	// TODO: dispath all events at once
	for (Entity entity : scene.entities) {

	    EntityCreateEvent e;
	    e.entity = entity;

	    event_dispatch("on_entity_create", &e);
	}

	event_dispatch("initialize_scene", nullptr);
	
	return true;
    }

    void _manage_scenes()
    {	
	if (scene_state->next_scene_name[0] != '\0') {

	    // TODO Handle error
	    _start_scene(scene_state->next_scene_name);
	    strcpy(scene_state->next_scene_name, "");
	}
    }

    SceneData* get_scene_data()
    {
	SV_SCENE();
	return &scene.data;
    }

    const char* get_scene_name()
    {
	SV_SCENE();
	return scene.name;
    }
    bool there_is_scene()
    {
	return scene_state->scene != nullptr;
    }

    bool set_scene(const char* name)
    {
	size_t name_size = strlen(name);

	if (name_size > SCENENAME_SIZE) {
	    SV_LOG_ERROR("The scene name '%s' is to long, max chars = %u", name, SCENENAME_SIZE);
	    return false;
	}
	
	// validate scene
	if (_user_validate_scene(name)) {
	    strcpy(scene_state->next_scene_name, name);
	    return true;
	}
	return false;
    }

    SV_API bool save_scene()
    {
	SV_SCENE();
	
	char filepath[FILEPATH_SIZE];
	if (_user_get_scene_filepath(scene.name, filepath)) {

	    return save_scene(filepath);
	}
	else return false;
    }

    bool save_scene(const char* filepath)
    {
	SV_SCENE();
	    
	Serializer s;

	serialize_begin(s);

	serialize_u32(s, SceneState::VERSION);

	serialize_entity(s, scene.data.main_camera);
	serialize_entity(s, scene.data.player);

	serialize_v2_f32(s, scene.data.gravity);
	serialize_f32(s, scene.data.air_friction);
		
	// ECS
	{
	    // Registers
	    {
		u32 registersCount = 0u;
		for (CompID id = 0u; id < scene_state->registers.size(); ++id) {
		    if (component_exist(id))
			++registersCount;
		}

		serialize_u32(s, registersCount);

		for (CompID id = 0u; id < scene_state->registers.size(); ++id) {

		    if (component_exist(id)) {

			serialize_string(s, get_component_name(id));
			serialize_u32(s, get_component_size(id));
			serialize_u32(s, get_component_version(id));
		    }

		}
	    }

	    // Entity data
	    {
		u32 entityCount = u32(scene.entities.size());
		u32 entityDataCount = u32(scene.entityData.size);

		serialize_u32(s, entityCount);
		serialize_u32(s, entityDataCount);

		for (u32 i = 0; i < entityDataCount; ++i) {

		    Entity entity = i + 1u;
		    EntityInternal& ed = scene.entityData.getInternal(entity);

		    if (ed.handleIndex != u64_max) {

			EntityTransform& transform = scene.entityData.getTransform(entity);

			serialize_entity(s, (Entity)i);
			serialize_string(s, ed.name.c_str());
			serialize_u32(s, ed.childsCount);
			serialize_size_t(s, ed.handleIndex);
			serialize_u64(s, ed.flags);
			serialize_v3_f32(s, transform.position);
			serialize_v4_f32(s, transform.rotation);
			serialize_v3_f32(s, transform.scale);
		    }
		}
	    }

	    // Components
	    {
		for (CompID compID = 0u; compID < scene_state->registers.size(); ++compID) {

		    if (!component_exist(compID)) continue;

		    serialize_u32(s, componentAllocatorCount(compID));

		    BaseComponent* component;
		    Entity entity;
		    ComponentIterator it;
		    
		    if (comp_it_begin(it, entity, component, compID)) {
			do {
			    serialize_entity(s, entity);
			    serialize_component(compID, component, s);
			}
			while (comp_it_next(it, entity, component));
		    }
		}
	    }
	}

	event_dispatch("save_scene", nullptr);
		
	return serialize_end(s, filepath);
    }

    bool clear_scene()
    {
	SV_SCENE();

	// TODO: Dispatch events at once
	for (Entity entity : scene.entities) {

	    EntityDestroyEvent e;
	    e.entity = entity;
	    event_dispatch("on_entity_destroy", &e);
	}
	
	for (CompID i = 0; i < get_component_register_count(); ++i) {
	    if (component_exist(i))
		componentAllocatorDestroy(i);
	}
	for (CompID id = 0u; id < scene_state->registers.size(); ++id) {

	    componentAllocatorCreate(id);
	}

	scene.entities.clear();
	entity_clear(scene.entityData);

	event_dispatch("initialize_scene", nullptr);

	return true;
    }

    bool create_entity_model(Entity parent, const char* folderpath)
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
					
			Entity entity = create_entity(parent);
			MeshComponent* comp = add_component<MeshComponent>(entity);
			comp->mesh = mesh;

			Mesh* m = mesh.get();

			set_entity_matrix(entity, m->model_transform_matrix);

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
	SV_SCENE();
	
	f32 dt = engine.deltatime;

	ComponentIterator it;
	CompView<BodyComponent> view;

	List<BodyCollision>& last_collisions = scene_state->last_collisions;
	List<BodyCollision>& current_collisions = scene_state->current_collisions;

	last_collisions.reset();
	foreach(i, current_collisions.size())
	    last_collisions.push_back(current_collisions[i]);
	
	current_collisions.reset();

	if (comp_it_begin(it, view)) {
	    
	    do {

		BodyComponent& body = *view.comp;
		
		if (body.body_type == BodyType_Static)
		    continue;

		v2_f32 position = get_entity_position2D(view.entity) + body.offset;
		v2_f32 scale = get_entity_scale2D(view.entity) * body.size;

		// Reset values
		body.in_ground = false;
	
		// Gravity
		if (body.body_type == BodyType_Dynamic)
		    body.vel += scene.data.gravity * dt;
	
		CompView<BodyComponent> vertical_collision = {};
		f32 vertical_depth = f32_max;
	
		CompView<BodyComponent> horizontal_collision = {};
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

			ComponentIterator it0;
			CompView<BodyComponent> v;

			if (comp_it_begin(it0, v)) {
			    
			    do {

				BodyComponent& b = *v.comp;

				if (b.body_type == BodyType_Static) {

				    v2_f32 p = get_entity_position2D(v.entity) + b.offset;
				    v2_f32 s = get_entity_scale2D(v.entity) * b.size;

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
			    
						vertical_collision.comp = &b;
						vertical_collision.entity = v.entity;
						vertical_depth = depth.y * ((next_pos.y < p.y) ? -1.f : 1.f);
					    }
					}

					// Horizontal collision
					else {

					    if (depth.x < abs(horizontal_depth)) {
				    
						horizontal_collision.comp = &b;
						horizontal_collision.entity = v.entity;
						horizontal_depth = depth.x * ((next_pos.x < p.x) ? -1.f : 1.f);

						if (depth.y < scale.y * 0.05f && next_pos.y > p.y) {
						    vertical_offset = depth.y;
						}
					    }
					}
				    }
				}
			    }
			    while (comp_it_next(it0, v));
			}
			

			if (vertical_collision.comp || horizontal_collision.comp)
			    break;
		    }

		    // Solve collisions
		    if (vertical_collision.comp) {

			if (!(vertical_collision.comp->flags & BodyComponentFlag_Trigger)) {

			    if (body.body_type == BodyType_Dynamic) {

				if (vertical_depth > 0.f) {

				    body.in_ground = true;
				    // Ground friction
				    next_vel.x *= pow(1.f - vertical_collision.comp->friction, dt);
				}

				next_pos.y += vertical_depth;
				next_vel.y = next_vel.y * -body.bounciness;
			    }
			    else if (body.body_type == BodyType_Projectile) {
				next_pos.y += vertical_depth;
				next_vel.y = -next_vel.y;
			    }
			}

			BodyCollision& col = current_collisions.emplace_back();
			col.b0 = view.comp;
			col.b1 = vertical_collision.comp;
			col.e0 = view.entity;
			col.e1 = vertical_collision.entity;
			col.trigger = vertical_collision.comp->flags & BodyComponentFlag_Trigger;
		    }

		    if (horizontal_collision.comp) {

			if (!(horizontal_collision.comp->flags & BodyComponentFlag_Trigger)) {

			    if (body.body_type == BodyType_Dynamic) {
		
				next_pos.x += horizontal_depth;
				if (vertical_offset != 0.f)
				    next_pos.y += vertical_offset;
				else
				    next_vel.x = next_vel.x * -body.bounciness;

			    }
			    else if (body.body_type == BodyType_Projectile) {
				next_pos.x += horizontal_depth;
				next_vel.x = -next_vel.x;
			    }
			}

			BodyCollision& col = current_collisions.emplace_back();
			col.b0 = view.comp;
			col.b1 = horizontal_collision.comp;
			col.e0 = view.entity;
			col.e1 = horizontal_collision.entity;
			col.trigger = horizontal_collision.comp->flags & BodyComponentFlag_Trigger;
		    }

		    // Air friction
		    if (body.body_type == BodyType_Dynamic)
			next_vel *= pow(1.f - scene.data.air_friction, dt);
		}
	
		position = next_pos;
		body.vel = next_vel;

		set_entity_position2D(view.entity, position - body.offset);
	    }
	    while (comp_it_next(it, view));

	    // Dispatch events
	    for (BodyCollision& col : current_collisions) {

		if (col.e1 < col.e0) {
		    std::swap(col.e0, col.e1);
		    std::swap(col.b0, col.b1);
		}

		CollisionState state = CollisionState_Enter;
		
		for (BodyCollision& c : last_collisions) {
		    if (col.e0 == c.e0 && col.e1 == c.e1) {
			state = CollisionState_Stay;
			c.leave = false;
			break;
		    }
		}

		if (col.trigger) {

		    TriggerCollisionEvent event;
		    if (col.b0->flags & BodyComponentFlag_Trigger) {
			event.trigger = CompView<BodyComponent>{ col.e0, col.b0 };
			event.body = CompView<BodyComponent>{ col.e1, col.b1 };
		    }
		    else {
			event.body = CompView<BodyComponent>{ col.e0, col.b0 };
			event.trigger = CompView<BodyComponent>{ col.e1, col.b1 };
		    }
		    event.state = state;

		    event_dispatch("on_trigger_collision", &event);
		}
		else {
		    
		    BodyCollisionEvent event;
		    event.body0 = CompView<BodyComponent>{ col.e0, col.b0 };
		    event.body1 = CompView<BodyComponent>{ col.e1, col.b1 };
		    event.state = state;

		    event_dispatch("on_body_collision", &event);
		}

		col.leave = true;
	    }
	    for (BodyCollision& col : last_collisions) {

		if (col.leave) {

		    if (col.trigger) {

			TriggerCollisionEvent event;
			if (col.b0->flags & BodyComponentFlag_Trigger) {
			    event.trigger = CompView<BodyComponent>{ col.e0, col.b0 };
			    event.body = CompView<BodyComponent>{ col.e1, col.b1 };
			}
			else {
			    event.body = CompView<BodyComponent>{ col.e0, col.b0 };
			    event.trigger = CompView<BodyComponent>{ col.e1, col.b1 };
			}
			event.state = CollisionState_Leave;

			event_dispatch("on_trigger_collision", &event);
		    }
		    else {

			BodyCollisionEvent event;
			event.body0 = CompView<BodyComponent>{ col.e0, col.b0 };
			event.body1 = CompView<BodyComponent>{ col.e1, col.b1 };
			event.state = CollisionState_Leave;
		    
			event_dispatch("on_body_collision", &event);
		    }
		}
	    }
	}
    }

    void _update_scene()
    {
	SV_SCENE();
	
	if (!there_is_scene())
	    return;

	if (!entity_exist(scene.data.main_camera)) {
	    scene.data.main_camera = SV_ENTITY_NULL;
	}

	CameraComponent* camera = get_main_camera();

	// Adjust camera
	if (camera) {
	    v2_u32 size = os_window_size();
	    camera->adjust(f32(size.x) / f32(size.y));
	}

	// Update cameras matrices
	{
	    ComponentIterator it;
	    CompView<CameraComponent> view;

	    if (comp_it_begin(it, view)) {
		do {

		    CameraComponent& camera = *view.comp;
		    Entity entity = view.entity;

		    v3_f32 position = get_entity_world_position(entity);
		    v4_f32 rotation = get_entity_world_rotation(entity);

		    update_camera_matrices(camera, position, rotation);
		}
		while (comp_it_next(it, view));
	    }

#if SV_DEV
	    update_camera_matrices(dev.camera, dev.camera.position, dev.camera.rotation);
#endif
	}

#if SV_DEV
	if (!engine.update_scene)
	    return;
#endif
	
	event_dispatch("update_scene", nullptr);
	    
	update_physics();

	event_dispatch("late_update_scene", nullptr);
    }

    CameraComponent* get_main_camera()
    {
	SV_SCENE();
	
	if (entity_exist(scene.data.main_camera))
	    return get_component<CameraComponent>(scene.data.main_camera);
	return nullptr;
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

    SV_INTERNAL void componentAllocatorCreate(CompID compID)
    {
	SV_SCENE();
	componentAllocatorDestroy(compID);
	componentAllocatorCreatePool(scene.components[compID], size_t(get_component_size(compID)));
    }

    SV_INTERNAL void componentAllocatorDestroy(CompID compID)
    {
	SV_SCENE();
	
	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

	    u8* ptr = it->data;
	    u8* endPtr = it->data + it->size;

	    while (ptr != endPtr) {

		BaseComponent* comp = reinterpret_cast<BaseComponent*>(ptr);
		if (comp->_id != 0u) {
		    destroy_component(compID, comp);
		}

		ptr += compSize;
	    }

	    componentPoolFree(*it);

	}
	a.pools.clear();
    }

    SV_INTERNAL BaseComponent* componentAlloc(CompID compID, Entity entity, bool create)
    {
	SV_SCENE();
	
	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
	BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

	if (create) create_component(compID, comp, entity);

	return comp;
    }

    SV_INTERNAL BaseComponent* componentAlloc(CompID compID, BaseComponent* srcComp, Entity entity)
    {
	SV_SCENE();
	
	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	ComponentPool& pool = componentAllocatorPreparePool(a, compSize);
	BaseComponent* comp = reinterpret_cast<BaseComponent*>(componentPoolGetPtr(pool, compSize));

	create_component(compID, comp, entity);
	copy_component(compID, srcComp, comp);

	return comp;
    }

    SV_INTERNAL void componentFree(CompID compID, BaseComponent* comp)
    {
	SV_SCENE();
	
	SV_ASSERT(comp != nullptr);

	ComponentAllocator& a = scene.components[compID];
	size_t compSize = size_t(get_component_size(compID));

	for (auto it = a.pools.begin(); it != a.pools.end(); ++it) {

	    if (componentPoolPtrExist(*it, comp)) {

		destroy_component(compID, comp);
		componentPoolRmvPtr(*it, compSize, comp);

		break;
	    }
	}
    }

    SV_INTERNAL u32 componentAllocatorCount(CompID compID)
    {
	SV_SCENE();
	
	const ComponentAllocator& a = scene.components[compID];
	u32 compSize = get_component_size(compID);

	u32 res = 0u;
	for (const ComponentPool& pool : a.pools) {
	    res += componentPoolCount(pool, compSize);
	}
	return res;
    }

    SV_INTERNAL bool componentAllocatorIsEmpty(CompID compID)
    {
	SV_SCENE();
	
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
	ComponentRegister* reg;
	
	// Check if is available
	{	    
	    if (desc->componentSize < sizeof(u32)) {
		SV_LOG_ERROR("Can't register a component type with size of %u", desc->componentSize);
		return SV_COMPONENT_ID_INVALID;
	    }

	    auto it = scene_state->component_names.find(desc->name);

	    if (it != scene_state->component_names.end()) {
		
		id = it->second;
		reg = &scene_state->registers[id];

		if (reg->size != desc->componentSize) {
		    SV_LOG_ERROR("Can't change the size of a component while the game in playing");
		    return SV_COMPONENT_ID_INVALID;
		}
	    }
	    else {

		if (there_is_scene()) {
		    SV_LOG_ERROR("Can't register component while a scene is running");
		    return SV_COMPONENT_ID_INVALID;
		}
		
		id = CompID(scene_state->registers.size());
		reg = &scene_state->registers.emplace_back();
	    }
	}
	
	reg->name = desc->name;
	reg->size = desc->componentSize;
	reg->version = desc->version;
	reg->createFn = desc->createFn;
	reg->destroyFn = desc->destroyFn;
	reg->moveFn = desc->moveFn;
	reg->copyFn = desc->copyFn;
	reg->serializeFn = desc->serializeFn;
	reg->deserializeFn = desc->deserializeFn;

	scene_state->component_names[desc->name] = id;

	SV_LOG_INFO("Component registred '%s'", desc->name);

	return id;
    }

    void invalidate_component_callbacks(CompID id)
    {
	// NOTE: The game callbacks closes after the unregister of all the components callbacks
	// so the game code will invalidate a component when none exists
	if (scene_state->registers.size() == 0u)
	    return;
	
	if (id == SV_COMPONENT_ID_INVALID) {
	    SV_LOG_ERROR("Can't invalidate a invalid component ID");
	    return;
	}

	if (id >= scene_state->registers.size()) {
	    SV_LOG_ERROR("Can't invalidate the component with the ID: %u", id);
	    return;
	}
	
	ComponentRegister& r = scene_state->registers[id];

	r.createFn = nullptr;
	r.destroyFn = nullptr;
	r.moveFn = nullptr;
	r.copyFn = nullptr;
	r.serializeFn = nullptr;
	r.deserializeFn = nullptr;
    }

    void unregister_components()
    {
	if (there_is_scene()) {
	    SV_LOG_ERROR("Can't unregister the components while there is scene");
	    return;
	}

	scene_state->registers.clear();
	scene_state->component_names.clear();

	SV_LOG_INFO("Components unregistred");
    }

    const char* get_component_name(CompID ID)
    {
	return scene_state->registers[ID].name.c_str();
    }
    u32 get_component_size(CompID ID)
    {
	return scene_state->registers[ID].size;
    }
    u32 get_component_version(CompID ID)
    {
	return scene_state->registers[ID].version;
    }
    CompID get_component_id(const char* name)
    {
	auto it = scene_state->component_names.find(name);
	if (it == scene_state->component_names.end()) return SV_COMPONENT_ID_INVALID;
	return it->second;
    }
    u32 get_component_register_count()
    {
	return u32(scene_state->registers.size());
    }

    void create_component(CompID ID, BaseComponent* ptr, Entity entity)
    {
	scene_state->registers[ID].createFn(ptr);
	ptr->_id = Entity(entity);
    }

    void destroy_component(CompID ID, BaseComponent* ptr)
    {
	scene_state->registers[ID].destroyFn(ptr);
	ptr->_id = 0u;
	ptr->flags = 0u;
    }

    void move_component(CompID ID, BaseComponent* from, BaseComponent* to)
    {
	scene_state->registers[ID].moveFn(from, to);
    }

    void copy_component(CompID ID, const BaseComponent* from, BaseComponent* to)
    {
	scene_state->registers[ID].copyFn(from, to);
    }

    void serialize_component(CompID ID, BaseComponent* comp, Serializer& serializer)
    {
	serialize_u32(serializer, comp->flags);
	SerializeComponentFunction fn = scene_state->registers[ID].serializeFn;
	if (fn) fn(comp, serializer);
    }

    void deserialize_component(CompID ID, BaseComponent* comp, Deserializer& deserializer, u32 version)
    {
	deserialize_u32(deserializer, comp->flags);
	DeserializeComponentFunction fn = scene_state->registers[ID].deserializeFn;
	if (fn) fn(comp, deserializer, version);
    }

    bool component_exist(CompID ID)
    {
	return scene_state->registers.size() > ID && scene_state->registers[ID].createFn != nullptr;
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

    SV_INLINE static void update_childs(Entity entity, i32 count)
    {
	SV_SCENE();
	
	Entity parent = entity;

	while (parent != SV_ENTITY_NULL) {
	    EntityInternal& parentToUpdate = scene.entityData.getInternal(parent);
	    parentToUpdate.childsCount += count;
	    parent = parentToUpdate.parent;
	}
    }

    ///////////////////////////////////// ENTITIES ////////////////////////////////////////

    Entity create_entity(Entity parent, const char* name)
    {
	SV_SCENE();

	Entity entity;

	{
	    auto& a = scene.entityData;
	
	    if (a.freelist.empty()) {

		if (a.size == a.capacity) {
		    EntityInternal* new_internal = SV_ALLOCATE_STRUCT_ARRAY(EntityInternal, a.capacity + ECS_ENTITY_ALLOC_SIZE);
		    EntityTransform* new_transforms = SV_ALLOCATE_STRUCT_ARRAY(EntityTransform, a.capacity + ECS_ENTITY_ALLOC_SIZE);

		    if (a.internal) {

			{
			    EntityInternal* end = a.internal + a.capacity;
			    while (a.internal != end) {

				*new_internal = std::move(*a.internal);

				++new_internal;
				++a.internal;
			    }
			}
			
			memcpy(new_transforms, a.transforms, a.capacity * sizeof(EntityTransform));
			
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
	    update_childs(parent, 1u);

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

	EntityCreateEvent e;
	e.entity = entity;
	event_dispatch("on_entity_create", &e);

	return entity;
    }

    void destroy_entity(Entity entity)
    {
	SV_SCENE();

	EntityInternal& entityData = scene.entityData.getInternal(entity);
	u32 count = entityData.childsCount + 1;

	// data to remove entities
	size_t indexBeginDest = entityData.handleIndex;
	size_t indexBeginSrc = entityData.handleIndex + count;
	size_t cpyCant = scene.entities.size() - indexBeginSrc;

	// Dispatch events
	{
	    EntityDestroyEvent e;

	    for (size_t i = 0; i < count; ++i) {
		
		Entity ent = scene.entities[indexBeginDest + i];
		e.entity = ent;
		event_dispatch("on_entity_destroy", &e);
	    }
	}

	// notify parents
	if (entityData.parent != SV_ENTITY_NULL) {
	    update_childs(entityData.parent, -i32(count));
	}

	// remove components & entityData
	for (size_t i = 0; i < count; i++) {
	    Entity e = scene.entities[indexBeginDest + i];
	    clear_entity(e);

	    { // Free the entity memory
		auto& a = scene.entityData;
		
		SV_ASSERT(a.size >= e);
		a.getInternal(e) = EntityInternal();
		a.getTransform(e) = EntityTransform();

		if (e == a.size) {
		    a.size--;
		}
		else {
		    a.freelist.push_back(e);
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

    void clear_entity(Entity entity)
    {
	SV_SCENE();

	EntityInternal& ed = scene.entityData.getInternal(entity);
	while (!ed.components.empty()) {

	    CompRef ref = ed.components.back();
	    ed.components.pop_back();

	    componentFree(ref.id, ref.ptr);
	}
    }

    SV_INTERNAL Entity entity_duplicate_recursive(Entity duplicated, Entity parent)
    {
	SV_SCENE();
	
	Entity copy;

	copy = create_entity(parent);

	EntityInternal& duplicatedEd = scene.entityData.getInternal(duplicated);
	EntityInternal& copyEd = scene.entityData.getInternal(copy);

	copyEd.name = duplicatedEd.name;

	scene.entityData.getTransform(copy) = scene.entityData.getTransform(duplicated);
	copyEd.flags = duplicatedEd.flags;

	for (u32 i = 0; i < duplicatedEd.components.size(); ++i) {
	    CompID ID = duplicatedEd.components[i].id;

	    BaseComponent* comp = ecs_entitydata_index_get(duplicatedEd, ID);
	    BaseComponent* newComp = componentAlloc(ID, comp, copy);

	    ecs_entitydata_index_add(copyEd, ID, newComp);
	}

	for (u32 i = 0; i < scene.entityData.getInternal(duplicated).childsCount; ++i) {
	    Entity toCopy = scene.entities[scene.entityData.getInternal(duplicated).handleIndex + i + 1];
	    entity_duplicate_recursive(toCopy, copy);
	    i += scene.entityData.getInternal(toCopy).childsCount;
	}

	return copy;
    }

    Entity duplicate_entity(Entity entity)
    {
	SV_SCENE();

	Entity res = entity_duplicate_recursive(entity, scene.entityData.getInternal(entity).parent);
	return res;
    }

    bool entity_is_empty(Entity entity)
    {
	SV_SCENE();
	return scene.entityData.getInternal(entity).components.empty();
    }

    bool entity_exist(Entity entity)
    {
	SV_SCENE();
	if (entity == SV_ENTITY_NULL || entity > scene.entityData.size) return false;

	EntityInternal& ed = scene.entityData.getInternal(entity);
	return ed.handleIndex != u64_max;
    }

    const char* get_entity_name(Entity entity)
    {
	SV_SCENE();
	SV_ASSERT(entity_exist(entity));
	const std::string& name = scene.entityData.getInternal(entity).name;
	return name.empty() ? "Unnamed" : name.c_str();
    }

    void set_entity_name(Entity entity, const char* name)
    {
	SV_SCENE();
	SV_ASSERT(entity_exist(entity));
	scene.entityData.getInternal(entity).name = name;
    }

    u32 get_entity_childs_count(Entity parent)
    {
	SV_SCENE();
	return scene.entityData.getInternal(parent).childsCount;
    }

    void get_entity_childs(Entity parent, Entity const** childsArray)
    {
	SV_SCENE();

	const EntityInternal& ed = scene.entityData.getInternal(parent);
	if (childsArray && ed.childsCount != 0)* childsArray = &scene.entities[ed.handleIndex + 1];
    }

    Entity get_entity_parent(Entity entity)
    {
	SV_SCENE();
	return scene.entityData.getInternal(entity).parent;
    }

    u64& get_entity_flags(Entity entity)
    {
	SV_SCENE();
	return scene.entityData.getInternal(entity).flags;
    }

    u32 get_entity_component_count(Entity entity)
    {
	SV_SCENE();
	return u32(scene.entityData.getInternal(entity).components.size());
    }

    u32 get_entity_count()
    {
	SV_SCENE();
	return u32(scene.entities.size());
    }

    Entity get_entity_by_index(u32 index)
    {
	SV_SCENE();
	return scene.entities[index];
    }

    /////////////////////////////// COMPONENTS /////////////////////////////////////

    BaseComponent* add_component(Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize)
    {
	SV_SCENE();

	comp = componentAlloc(componentID, comp, Entity(entity));
	ecs_entitydata_index_add(scene.entityData.getInternal(entity), componentID, comp);

	return comp;
    }

    BaseComponent* add_component_by_id(Entity entity, CompID componentID)
    {
	SV_SCENE();

	BaseComponent* comp = componentAlloc(componentID, entity);
	ecs_entitydata_index_add(scene.entityData.getInternal(entity), componentID, comp);

	return comp;
    }

    BaseComponent* get_component_by_id(Entity entity, CompID componentID)
    {
	SV_SCENE();

	return ecs_entitydata_index_get(scene.entityData.getInternal(entity), componentID);
    }

    CompRef get_component_by_index(Entity entity, u32 index)
    {
	SV_SCENE();
	return scene.entityData.getInternal(entity).components[index];
    }

    void remove_component_by_id(Entity entity, CompID componentID)
    {
	SV_SCENE();

	EntityInternal& ed = scene.entityData.getInternal(entity);
	BaseComponent* comp = ecs_entitydata_index_get(ed, componentID);

	componentFree(componentID, comp);
	ecs_entitydata_index_remove(ed, componentID);
    }

    u32 get_component_count(CompID ID)
    {
	return componentAllocatorCount(ID);
    }

    // Iterators

    bool comp_it_begin(ComponentIterator& it, Entity& entity, BaseComponent*& comp, CompID comp_id)
    {
	SV_SCENE();

	it._comp_id = comp_id;
	it._it = nullptr;
	it._end = nullptr;
	it._pool = 0u;
	
	if (!componentAllocatorIsEmpty(comp_id)) {

	    const ComponentAllocator& list = scene.components[comp_id];
	    size_t comp_size = size_t(get_component_size(comp_id));

	    u32 pool = 0u;
	    u8* ptr = nullptr;

	    // Finding the first component
	    {
		while (componentPoolCount(list.pools[pool], comp_size) == 0u) pool++;

		ptr = list.pools[pool].data;

		while (true) {
		    BaseComponent* c = reinterpret_cast<BaseComponent*>(ptr);
		    if (c->_id != 0u) {
			break;
		    }
		    ptr += comp_size;
		}
	    }

	    it._pool = pool;
	    it._it = reinterpret_cast<BaseComponent*>(ptr);

	    // Find the end component
	    {
		pool = u32(list.pools.size()) - 1u;
		
		while (componentPoolCount(list.pools[pool], comp_size) == 0u) pool--;
		ptr = list.pools[pool].data + list.pools[pool].size;
	    }

	    it._end = reinterpret_cast<BaseComponent*>(ptr);
	}

	bool res = it._end != it._it;

	if (res) {
	    // TODO: premakes
	    comp = it._it;
	    entity = Entity(it._it->_id);
	}

	return res;
    }

    
    bool comp_it_next(ComponentIterator& it, Entity& entity, BaseComponent*& comp)
    {
	SV_SCENE();

	BaseComponent*& _it = it._it;
	u32& _pool = it._pool;
	CompID comp_id = it._comp_id;

	size_t comp_size = size_t(get_component_size(comp_id));
	auto& list = scene.components[comp_id];
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
	} while (((BaseComponent*)(ptr))->_id == 0u);

	_it = reinterpret_cast<BaseComponent*>(ptr);

	bool res = _it != it._end;

	if (res) {

	    // TODO: premakes
	    comp = _it;
	    entity = Entity(_it->_id);
	}

	return res;
    }

    //////////////////////////////////////////// TRANSFORM ///////////////////////////////////////////////////////////

    SV_AUX void update_world_matrix(EntityTransform& t, Entity entity)
    {
	SV_SCENE();
	
	t._modified = false;

	XMMATRIX m = get_entity_matrix(entity);

	EntityInternal& entityData = scene.entityData.getInternal(entity);
	Entity parent = entityData.parent;

	if (parent != SV_ENTITY_NULL) {
	    
	    XMMATRIX mp = get_entity_world_matrix(parent);
	    m = m * mp;
	}
	
	XMStoreFloat4x4(&t.world_matrix, m);
    }

    SV_AUX void notify_transform(EntityTransform& t, Entity entity)
    {
	SV_SCENE();
	
	if (!t._modified) {

	    t._modified = true;

	    auto& list = scene.entityData;
	    EntityInternal& entityData = list.getInternal(entity);

	    if (entityData.childsCount == 0) return;

	    auto& entities = scene.entities;
	    for (u32 i = 0; i < entityData.childsCount; ++i) {
		EntityTransform& et = list.getTransform(entities[entityData.handleIndex + 1 + i]);
		et._modified = true;
	    }
	}
    }

    void set_entity_transform(Entity entity, const Transform& transform)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	t.position = transform.position;
	t.rotation = transform.rotation;
	t.scale = transform.scale;
	notify_transform(t, entity);
    }
    
    void set_entity_position(Entity entity, const v3_f32& position)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	t.position = position;
	notify_transform(t, entity);
    }
    
    void set_entity_rotation(Entity entity, const v4_f32& rotation)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	t.rotation = rotation;
	notify_transform(t, entity);
    }
    
    void set_entity_scale(Entity entity, const v3_f32& scale)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	t.scale = scale;
	notify_transform(t, entity);
    }
    
    void set_entity_matrix(Entity entity, const XMMATRIX& matrix)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	
	notify_transform(t, entity);

	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, matrix);

	t.position.setDX(position);
	t.scale.setDX(scale);
	t.rotation.set_dx(rotation);
    }

    void set_entity_position2D(Entity entity, const v2_f32& position)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	t.position.x = position.x;
	t.position.y = position.y;
	notify_transform(t, entity);
    }
    
    Transform* get_entity_transform_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return reinterpret_cast<Transform*>(&t.position);
    }
    
    v3_f32* get_entity_position_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return &t.position;
    }
    
    v4_f32* get_entity_rotation_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return &t.rotation;
    }
    
    v3_f32* get_entity_scale_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return &t.scale;
    }
    
    v2_f32* get_entity_position2D_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return reinterpret_cast<v2_f32*>(&t.position);
    }
    
    v2_f32* get_entity_scale2D_ptr(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	notify_transform(t, entity);

	return reinterpret_cast<v2_f32*>(&t.scale);
    }
    
    Transform get_entity_transform(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	Transform trans;
	trans.position = t.position;
	trans.rotation = t.rotation;
	trans.scale = t.scale;
	return trans;
    }
    
    v3_f32 get_entity_position(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	return t.position;
    }
    
    v4_f32 get_entity_rotation(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	return t.rotation;
    }
    
    v3_f32 get_entity_scale(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	return t.scale;
    }
    
    v2_f32 get_entity_position2D(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	return t.position.getVec2();
    }
    
    v2_f32 get_entity_scale2D(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	return t.scale.getVec2();
    }

    XMMATRIX get_entity_matrix(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);

	return XMMatrixScalingFromVector(t.scale.getDX()) * XMMatrixRotationQuaternion(t.rotation.get_dx())
	    * XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
    }

    Transform get_entity_world_transform(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);

	if (t._modified) update_world_matrix(t, entity);
	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t.world_matrix));

	Transform trans;
	trans.position = v3_f32(position);
	trans.rotation = v4_f32(rotation);
	trans.scale = v3_f32(scale);

	return trans;
    }
    
    v3_f32 get_entity_world_position(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(t, entity);
	return *(v3_f32*) &t.world_matrix._41;
    }
    
    v4_f32 get_entity_world_rotation(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(t, entity);
	XMVECTOR scale;
	XMVECTOR rotation;
	XMVECTOR position;

	XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t.world_matrix));

	return v4_f32(rotation);
    }
    
    v3_f32 get_entity_world_scale(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(t, entity);
	return { (*(v3_f32*)& t.world_matrix._11).length(), (*(v3_f32*)& t.world_matrix._21).length(), (*(v3_f32*)& t.world_matrix._31).length() };
    }
    
    XMMATRIX get_entity_world_matrix(Entity entity)
    {
	SV_SCENE();
	EntityTransform& t = scene.entityData.getTransform(entity);
	if (t._modified) update_world_matrix(t, entity);
	return XMLoadFloat4x4(&t.world_matrix);
    }

    //////////////////////////////////////////// COMPONENTS ////////////////////////////////////////////////////////

    void SpriteComponent::serialize(Serializer& s)
    {
	serialize_asset(s, texture);
	serialize_v4_f32(s, texcoord);
	serialize_color(s, color);
	serialize_u32(s, layer);
    }

    void SpriteComponent::deserialize(Deserializer& d, u32 version)
    {
	deserialize_asset(d, texture);
	deserialize_v4_f32(d, texcoord);
	deserialize_color(d, color);
	deserialize_u32(d, layer);
    }

    void CameraComponent::serialize(Serializer& s)
    {
	serialize_u32(s, projection_type);
	serialize_f32(s, near);
	serialize_f32(s, far);
	serialize_f32(s, width);
	serialize_f32(s, height);
    }

    void CameraComponent::deserialize(Deserializer& d, u32 version)
    {
	deserialize_u32(d, (u32&)projection_type);
	deserialize_f32(d, near);
	deserialize_f32(d, far);
	deserialize_f32(d, width);
	deserialize_f32(d, height);
    }

    void MeshComponent::serialize(Serializer& s)
    {
	serialize_asset(s, mesh);
	serialize_asset(s, material);
    }

    void MeshComponent::deserialize(Deserializer& d, u32 version)
    {
	deserialize_asset(d, mesh);
	deserialize_asset(d, material);
    }

    void LightComponent::serialize(Serializer& s)
    {
	serialize_u32(s, light_type);
	serialize_color(s, color);
	serialize_f32(s, intensity);
	serialize_f32(s, range);
	serialize_f32(s, smoothness);
    }
    
    void LightComponent::deserialize(Deserializer& d, u32 version)
    {
	deserialize_u32(d, (u32&)light_type);
	deserialize_color(d, color);
	deserialize_f32(d, intensity);
	deserialize_f32(d, range);
	deserialize_f32(d, smoothness);
    }

    void BodyComponent::serialize(Serializer& s)
    {
	serialize_u32(s, body_type);
	serialize_v2_f32(s, size);
	serialize_v2_f32(s, offset);
	serialize_v2_f32(s, vel);
	serialize_f32(s, mass);
	serialize_f32(s, friction);
	serialize_f32(s, bounciness);
    }

    void BodyComponent::deserialize(Deserializer& d, u32 version)
    {
	switch (version) {
	    
	case 0:
	    deserialize_u32(d, (u32&)body_type);
	    deserialize_v2_f32(d, size);
	    deserialize_v2_f32(d, offset);
	    deserialize_f32(d, mass);
	    deserialize_f32(d, friction);
	    deserialize_f32(d, bounciness);
	    break;

	case 1:
	    deserialize_u32(d, (u32&)body_type);
	    deserialize_v2_f32(d, size);
	    deserialize_v2_f32(d, offset);
	    deserialize_v2_f32(d, vel);
	    deserialize_f32(d, mass);
	    deserialize_f32(d, friction);
	    deserialize_f32(d, bounciness);
	    break;
	}
    }
}
