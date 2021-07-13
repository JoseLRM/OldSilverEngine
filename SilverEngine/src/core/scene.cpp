#include "core/scene.h"

#include "core/renderer.h"
#include "core/physics3D.h"
#include "core/sound_system.h"
#include "debug/console.h"

#define SV_SCENE() sv::Scene& scene = *scene_state->scene
#define SV_ECS() SV_SCENE(); sv::ECS& ecs = scene.ecs

namespace sv {

	struct TagInternal {
		List<Entity> entities;
	};

	struct TagRegister {
		char name[TAG_NAME_SIZE + 1u];
	};

	struct PrefabInternal {
		
		u64 component_mask;
		u32 component_count;
		CompRef components[ENTITY_COMPONENTS_MAX];

		bool valid;

		List<Entity> entities;

		char filepath[FILEPATH_SIZE + 1u];
		char name[ENTITY_NAME_SIZE + 1u];
	};

	struct EntityInternal {

		// TODO: Should move this to other pool??
		u64 component_mask;
		u32 component_count;
		CompRef components[ENTITY_COMPONENTS_MAX];
		u64 tag_mask;
		
		u32 hierarchy_index;
		Entity parent;
		u32 child_count;
		Prefab prefab;
		
	};

	struct EntityMisc {
		
		char name[ENTITY_NAME_SIZE + 1u];
		u64 tag;
		u64 flags;
		
	};

	struct EntityTransform {

		// World space

		XMFLOAT4X4 world_matrix;

		// Local space
		
		v3_f32 position;
		v3_f32 scale;
		v4_f32 rotation;

		bool dirty;

		// Used to know if the physics engine should update the transform
		bool dirty_physics;
		
	};

	struct ComponentPool {
		u8* data;
		u32 count;
		u32 capacity;
		u32 free_count;
	};

	struct ComponentAllocator {

		ComponentPool* pools;
		u32            pool_count;
		
	};

	typedef void(*CreateComponentFn)(Component*, Entity entity);
    typedef void(*DestroyComponentFn)(Component*, Entity entity);
    typedef void(*CopyComponentFn)(Component* dst, const Component* src, Entity entity);
    typedef void(*SerializeComponentFn)(Component* comp, Serializer& serializer);
    typedef void(*DeserializeComponentFn)(Component* comp, Deserializer& deserializer, u32 version);

	struct ComponentRegister {

		char		           name[COMPONENT_NAME_SIZE + 1u];
		u32				       size;
		u32                    version;
		CreateComponentFn	   create_fn;
		DestroyComponentFn	   destroy_fn;
		CopyComponentFn		   copy_fn;
		SerializeComponentFn   serialize_fn;
		DeserializeComponentFn deserialize_fn;
		Library                library;
		char		           struct_name[COMPONENT_NAME_SIZE + 1u];

    };
	
	struct ECS {

		EntityInternal*  entity_internal = NULL;
		EntityMisc*      entity_misc = NULL;
		EntityTransform* entity_transform = NULL;
		u32              entity_size = 0u;
		u32              entity_capacity = 0u;

		List<Entity>     entity_hierarchy;
		List<Entity>     entity_free_list;
		
		List<PrefabInternal> prefabs;
		u32                  prefab_free_count = 0u;

		TagInternal tags[TAG_MAX];

		ComponentAllocator component_allocator[COMPONENT_MAX];
		
	};

    struct Scene {

		SceneData data;
	
		char name[SCENE_NAME_SIZE + 1u] = {};
		
		ECS ecs;
	
    };

    struct SceneState {

		static constexpr u32 VERSION = 4u;

		char next_scene_name[SCENE_NAME_SIZE + 1u] = {};
		Scene* scene = nullptr;

		ComponentRegister component_register[COMPONENT_MAX];
		u32 component_register_count = 0u;

		TagRegister tag_register[TAG_MAX];
		
    };

    static SceneState* scene_state = nullptr;

    SV_INTERNAL bool asset_create_spritesheet(void* ptr, const char* name)
    {
		new(ptr) SpriteSheet();
		return true;
    }

    SV_INTERNAL bool asset_load_spritesheet(void* ptr, const char* name, const char* filepath)
    {
		SpriteSheet& sheet = *new(ptr) SpriteSheet();

		Deserializer d;

		bool res = true;
	
		if (deserialize_begin(d, filepath)) {
	    
			deserialize_sprite_sheet(d, sheet);
			deserialize_end(d);
		}
		else res = false;
	
		return res;
    }

    SV_INTERNAL bool asset_free_spritesheet(void* ptr, const char* name)
    {
		SpriteSheet* sheet = reinterpret_cast<SpriteSheet*>(ptr);
		sheet->~SpriteSheet();
		return true;
    }

	SV_INTERNAL void initialize_ecs();
	SV_INTERNAL void serialize_ecs(Serializer& s);
	SV_INTERNAL bool deserialize_ecs(Deserializer& d);
	SV_INTERNAL void clear_ecs();
	SV_INTERNAL void close_ecs();

#if SV_EDITOR
	void reload_component(ReloadPluginEvent* e);
#endif

    bool _scene_initialize()
    {
		scene_state = SV_ALLOCATE_STRUCT(SceneState, "Scene");

		// Register assets
		{
			const char* extensions[] = {
				"sprites"
			};
	    
			AssetTypeDesc desc;
			desc.name = "SpriteSheet";
			desc.asset_size = sizeof(SpriteSheet);
			desc.extensions = extensions;
			desc.extension_count = 1u;
			desc.create_fn = asset_create_spritesheet;
			desc.load_file_fn = asset_load_spritesheet;
			desc.reload_file_fn = NULL;
			desc.free_fn = asset_free_spritesheet;
			desc.unused_time = 2.f;

			register_asset_type(&desc);
		}

#if SV_EDITOR
		event_register("reload_plugin", reload_component, 0);
#endif

		return true;
    }

    SV_AUX void destroy_current_scene()
    {
		if (there_is_scene()) {

			Scene*& scene = scene_state->scene;

			event_dispatch("close_scene", nullptr);

			free_skybox();

			close_ecs();

			SV_FREE_STRUCT(scene);
			scene = nullptr;
		}
    }
    
    void _scene_close()
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

    bool _start_scene(const char* name_)
    {
		Scene*& scene_ptr = scene_state->scene;

		name_ = string_validate(name_);

		char name[SCENE_NAME_SIZE + 1];
		string_copy(name, name_, SCENE_NAME_SIZE + 1u);
	
		// Close last scene
		destroy_current_scene();

		if (name[0] == '\0')
			return true;
		
		scene_ptr = SV_ALLOCATE_STRUCT(Scene, "Scene");

		Scene& scene = *scene_ptr;
		strcpy(scene.name, name);

		initialize_ecs();

		bool deserialize = false;
		Deserializer d;

		// Deserialize
		{
			// Get filepath
			char filepath[FILEPATH_SIZE] = "assets/scenes/";
			string_append(filepath, name, FILEPATH_SIZE + 1u);
			string_append(filepath, ".scene", FILEPATH_SIZE + 1u);

			bool res = deserialize_begin(d, filepath);

			if (!res) {

				SV_LOG_ERROR("Can't deserialize the scene '%s' at '%s'", name, filepath);
			}
			else {
				u32 scene_version;
				deserialize_u32(d, scene_version);
		    
				deserialize_entity(d, scene.data.main_camera);

				if (scene_version <= 3) {
					Entity e ;
					deserialize_entity(d, e);
				}

				// Skybox
				if (scene_version >= 3u) {

					char filepath[FILEPATH_SIZE + 1u] = "";
					deserialize_string(d, filepath, FILEPATH_SIZE + 1u);

					if (string_size(filepath)) {
							
						set_skybox(filepath);
					}
				}

				if (scene_version == 0u) {
					v2_f32 g;
					deserialize_v2_f32(d, g);
					scene.data.physics.gravity = vec2_to_vec3(g);
					deserialize_f32(d, scene.data.physics.air_friction);
				}
				else {
					deserialize_v3_f32(d, scene.data.physics.gravity);
					deserialize_f32(d, scene.data.physics.air_friction);
					deserialize_bool(d, scene.data.physics.in_3D);
				}

				// ECS
				if (!deserialize_ecs(d)) return false;

				deserialize_end(d);

				deserialize = true;
			}
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
		return scene_state && scene_state->scene != nullptr;
    }

    bool set_scene(const char* name)
    {
		size_t name_size = string_size(name);

		if (name_size > SCENE_NAME_SIZE) {
			SV_LOG_ERROR("The scene name '%s' is to long, max chars = %u", name, SCENE_NAME_SIZE);
			return false;
		}
	
		strcpy(scene_state->next_scene_name, name);
		return true;
    }

    SV_API bool save_scene()
    {
		SV_SCENE();
	
		char filepath[FILEPATH_SIZE + 1u] = "assets/scenes/";
		string_append(filepath, scene.name, FILEPATH_SIZE + 1u);
		string_append(filepath, ".scene", FILEPATH_SIZE + 1u);
		
		return save_scene(filepath);
    }

    bool save_scene(const char* filepath)
    {
		SV_SCENE();
	    
		Serializer s;

		serialize_begin(s);

		serialize_u32(s, SceneState::VERSION);

		serialize_entity(s, scene.data.main_camera);

		// Skybox
		serialize_string(s, scene.data.skybox.image.get() ? scene.data.skybox.filepath : "");

		serialize_v3_f32(s, scene.data.physics.gravity);
		serialize_f32(s, scene.data.physics.air_friction);
		serialize_bool(s, scene.data.physics.in_3D);

		serialize_ecs(s);

		event_dispatch("save_scene", nullptr);
		
		return serialize_end(s, filepath);
    }

    bool clear_scene()
    {
		//SV_SCENE();

		clear_ecs();

		event_dispatch("initialize_scene", nullptr);

		return true;
    }

    bool create_entity_model(Entity parent, const char* folderpath)
    {
		//PARSE_SCENE();
		
		FolderIterator it;
		FolderElement element;

		bool res = folder_iterator_begin(folderpath, &it, &element);

		CompID mesh_id = get_component_id("Mesh");

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
						MeshComponent* comp = (MeshComponent*)add_entity_component(entity, mesh_id);
						SV_ASSERT(comp);
						
						comp->mesh = mesh;

						Mesh* m = mesh.get();

						set_entity_matrix(entity, m->model_transform_matrix);

						if (string_size(m->model_material_filepath)) {
					
							MaterialAsset mat;
							res = load_asset_from_file(mat, m->model_material_filepath);

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
		camera.view_matrix = mat_view_from_quaternion(position, rotation);

		// Compute projection matrix
		{
#if SV_EDITOR
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

    void _update_scene()
    {
		SV_SCENE();
	
		if (!there_is_scene())
			return;

		if (!entity_exists(scene.data.main_camera)) {
			scene.data.main_camera = 0;
		}

#if SV_EDITOR
		if (engine.update_scene) {
#endif
			event_dispatch("update_scene", NULL);

			_physics3D_update();
			event_dispatch("update_physics", NULL);
		
			event_dispatch("late_update_scene", NULL);
#if SV_EDITOR
		}
#endif

#if !(SV_EDITOR)
		
		CameraComponent* camera = get_main_camera();
		
		// Adjust camera
		if (camera) {
			v2_u32 size = os_window_size();
			camera->adjust(f32(size.x) / f32(size.y));
		}
#endif

		// Update cameras matrices
		{
			CompID camera_id = get_component_id("Camera");

			for (CompIt it = comp_it_begin(camera_id);
				 it.has_next;
				 comp_it_next(it))
			{
				CameraComponent& camera = *(CameraComponent*)it.comp;
				Entity entity = it.entity;
				
				v3_f32 position = get_entity_world_position(entity);
				v4_f32 rotation = get_entity_world_rotation(entity);
				
				update_camera_matrices(camera, position, rotation);
			}
			
#if SV_EDITOR
			update_camera_matrices(dev.camera, dev.camera.position, dev.camera.rotation);
#endif
		}
    }

    CameraComponent* get_main_camera()
    {
		SV_SCENE();
	
		if (entity_exists(scene.data.main_camera))
			return (CameraComponent*)get_entity_component(scene.data.main_camera, get_component_id("Camera"));
		return NULL;
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

			position = position * v2_f32(camera->width, camera->height) + vec3_to_vec2(camera_position);
			ray.origin = vec2_to_vec3(position, camera->near);
			ray.direction = { 0.f, 0.f, 1.f };
		}

		return ray;
    }

	void free_skybox()
	{
		SV_SCENE();
		
		if (scene.data.skybox.image.get()) {
			unload_asset(scene.data.skybox.image);
			string_copy(scene.data.skybox.filepath, "", FILEPATH_SIZE + 1u);
		}
	}

	bool set_skybox(const char* filepath)
	{
		SV_SCENE();
		
		if (string_equals(filepath, scene.data.skybox.filepath))
			return true;
			
		free_skybox();

		// TODO: Use asset system features
		GPUImage* image;
		bool res = load_skymap_image(filepath, &image);
		
		if (res) {
			SV_LOG_INFO("Skybox '%s' loaded", filepath);
			create_asset(scene.data.skybox.image, "Texture");
			scene.data.skybox.image.set(image);
			string_copy(scene.data.skybox.filepath, filepath, FILEPATH_SIZE + 1u);
		}
		else SV_LOG_ERROR("Can't load the skybox '%s'", filepath);

		return res;
	}

	/////////////////////////////////////// ECS //////////////////////////////////////////////////////////

	SV_AUX void create_entity_component(CompID comp_id, Component* ptr, Entity entity)
    {
		scene_state->component_register[comp_id].create_fn(ptr, entity);
		ptr->id = entity;
		ptr->flags = 0u;
    }
	SV_AUX void create_prefab_component(CompID comp_id, Component* ptr, Prefab prefab)
    {
		scene_state->component_register[comp_id].create_fn(ptr, 0);
		ptr->id = prefab | SV_BIT(31);
		ptr->flags = 0u;
    }
    SV_AUX void destroy_component(CompID comp_id, Component* ptr)
    {
		Entity entity = (ptr->id & SV_BIT(31)) ? 0 : ptr->id;
		scene_state->component_register[comp_id].destroy_fn(ptr, entity);
		ptr->id = 0u;
		ptr->flags = 0u;
    }
    SV_AUX void copy_component(CompID comp_id, Component* dst, const Component* src, Entity entity)
    {
		scene_state->component_register[comp_id].copy_fn(dst, src, entity);
		dst->id = entity;
    }
    SV_AUX void serialize_component(CompID comp_id, Component* comp, Serializer& serializer)
    {
		serialize_u32(serializer, comp->flags);
		SerializeComponentFn fn = scene_state->component_register[comp_id].serialize_fn;
		if (fn) fn(comp, serializer);
    }
    SV_AUX void deserialize_component(CompID comp_id, Component* comp, Deserializer& deserializer, u32 version)
    {
		deserialize_u32(deserializer, comp->flags);
		DeserializeComponentFn fn = scene_state->component_register[comp_id].deserialize_fn;
		if (fn) fn(comp, deserializer, version);
    }

	SV_AUX bool has_free_components(ComponentPool& pool)
	{
		return pool.count < pool.capacity || pool.free_count;
	}

	SV_AUX Component* allocate_component(CompID comp_id)
	{
		SV_ECS();

		ComponentRegister& reg = scene_state->component_register[comp_id];
		ComponentAllocator& alloc = ecs.component_allocator[comp_id];

		ComponentPool* pool = NULL;

		foreach(i, alloc.pool_count) {

			if (has_free_components(alloc.pools[i])) {

				pool = alloc.pools + i;
				break;
			}
		}

		if (pool == NULL) {

			ComponentPool* new_pools = (ComponentPool*) SV_ALLOCATE_MEMORY(sizeof(ComponentPool) * (alloc.pool_count + 1), "Scene");

			if (alloc.pools) {
				memcpy(new_pools, alloc.pools, sizeof(ComponentPool) * alloc.pool_count);
				SV_FREE_MEMORY(alloc.pools);
			}

			alloc.pools = new_pools;
			pool = alloc.pools + alloc.pool_count++;

			pool->count = 0u;
			pool->capacity = 3u;
			pool->free_count = 0u;
			pool->data = (u8*)SV_ALLOCATE_MEMORY(reg.size * pool->capacity, "Scene");
		}

		Component* component = NULL;

		if (pool->free_count) {

			u8* it = pool->data;
			u8* end = pool->data + pool->count * reg.size;

			while (it < end) {

				Component* comp = reinterpret_cast<Component*>(it);

				if (comp->id == 0u) {

					component = comp;
					break;
				}
				
				it += reg.size;
			}

			SV_ASSERT(component);
			--pool->free_count;
		}
		else {

			SV_ASSERT(pool->count < pool->capacity);

			component = reinterpret_cast<Component*>(pool->data + (pool->count * reg.size));
			++pool->count;
		}

		SV_ASSERT(component);

		return component;
	}

	SV_AUX void free_component(CompRef comp_ref)
	{
		SV_ECS();
		
		CompID comp_id = comp_ref.comp_id;
		Component* comp = comp_ref.comp;
		
		ComponentRegister& reg = scene_state->component_register[comp_id];
		ComponentAllocator& alloc = ecs.component_allocator[comp_id];

		destroy_component(comp_id, comp);

		ComponentPool* pool = NULL;

		foreach(i, alloc.pool_count) {

			ComponentPool* p = alloc.pools + i;

			if (p->data <= (u8*)comp && p->data + p->count * reg.size > (u8*)comp) {
				pool = p;
				break;
			}
		}

		SV_ASSERT(pool);

		if (pool) {

			++pool->free_count;
		}

		// TODO: Destroy pool
	}
	
	constexpr u32 ENTITY_ALLOCATION_POOL = 100u;

	void initialize_ecs()
	{
		SV_ECS();
		
		for (CompID id = 0u; id < COMPONENT_MAX; ++id) {
			
			ComponentAllocator& alloc = ecs.component_allocator[id];
			alloc.pools = NULL;
			alloc.pool_count = 0u;
		}
	}

	void clear_ecs()
	{
		close_ecs();
		initialize_ecs();
	}

	void close_ecs()
	{
		SV_ECS();
		
		// TODO: Dispatch events at once
		for (Entity entity : ecs.entity_hierarchy) {
			
			EntityDestroyEvent e;
			e.entity = entity;
			event_dispatch("on_entity_destroy", &e);
		}
		
		for (CompID id = 0; id < scene_state->component_register_count; ++id) {

			ComponentAllocator& alloc = ecs.component_allocator[id];
			ComponentRegister& reg = scene_state->component_register[id];

			foreach(i, alloc.pool_count) {

				ComponentPool* pool = alloc.pools + i;

				if (pool->data) {
					
					u8* it = pool->data;
					u8* end = it + pool->count * reg.size;

					while (it != end) {

						Component* comp = (Component*)it;

						if (comp->id != 0) {
							destroy_component(id, comp);
						}
					
						it += reg.size;
					}

					SV_FREE_MEMORY(pool->data);
				}
			}

			if (alloc.pools) {
				SV_FREE_MEMORY(alloc.pools);
			}
		}
		
		if (ecs.entity_internal) {

			SV_FREE_MEMORY(ecs.entity_internal);
			SV_FREE_MEMORY(ecs.entity_misc);
			SV_FREE_MEMORY(ecs.entity_transform);
			ecs.entity_internal = NULL;
			ecs.entity_misc = NULL;
			ecs.entity_transform = NULL;

			ecs.entity_size = 0u;
			ecs.entity_capacity = 0u;
			ecs.entity_hierarchy.clear();
			ecs.entity_free_list.clear();
		}

		ecs.prefabs.clear();
		ecs.prefab_free_count = 0u;
	}

	SV_AUX void initialize_entity_internal(EntityInternal& e)
	{
		e.component_mask = 0u;
		e.component_count = 0u;
		e.tag_mask = 0u;
		e.hierarchy_index = u32_max;
		e.parent = 0u;
		e.child_count = 0u;
		e.prefab = 0u;
	}

	SV_AUX void initialize_entity_misc(EntityMisc& e)
	{
		e.name[0] = '\0';
		e.tag = 0u;
		e.flags = 0u;
	}

	SV_AUX void initialize_entity_transform(EntityTransform& e)
	{
		e.position      = { 0.f, 0.f, 0.f };
		e.scale         = { 1.f, 1.f, 1.f };
		e.rotation      = { 0.f, 0.f, 0.f, 1.f };
		e.dirty         = true;
		e.dirty_physics = true;
	}

	void serialize_ecs(Serializer& s)
	{
		SV_ECS();

		constexpr u32 VERSION = 2u;
		serialize_u32(s, VERSION);
		
		// Registers
		{
			serialize_u32(s, scene_state->component_register_count);

			foreach(id, scene_state->component_register_count) {

				serialize_string(s, get_component_name(id));
				serialize_u32(s, get_component_size(id));
				serialize_u32(s, get_component_version(id));
			}

			u32 tag_count = 0u;
			
			foreach(tag, TAG_MAX) {
				if (scene_state->tag_register[tag].name[0])
					++tag_count;
			}

			serialize_u32(s, tag_count);

			foreach(tag, tag_count) {
				if (scene_state->tag_register[tag].name[0]) {
					serialize_u32(s, tag);
					serialize_string(s, scene_state->tag_register[tag].name);
				}
			}
		}

		// Prefabs
		{
			serialize_u32(s, (u32)ecs.prefabs.size() - ecs.prefab_free_count);
				
			foreach(i, ecs.prefabs.size()) {

				if (ecs.prefabs[i].valid) {
				
					serialize_u32(s, i + 1u);
					serialize_string(s, ecs.prefabs[i].filepath);
				}
			}
		}

		// Entity data
		{
			u32 entity_count = u32(ecs.entity_hierarchy.size());
			u32 entity_data_count = ecs.entity_size;

			serialize_u32(s, entity_count);
			serialize_u32(s, entity_data_count);

			for (u32 i = 0; i < entity_data_count; ++i) {

				Entity entity = i + 1u;
				EntityInternal& internal = ecs.entity_internal[entity - 1u];

				if (internal.hierarchy_index != u32_max) {

					EntityMisc& misc = ecs.entity_misc[entity - 1u];
					EntityTransform& transform = ecs.entity_transform[entity - 1u];

					serialize_entity(s, entity);
					serialize_u64(s, internal.tag_mask);
					serialize_u32(s, internal.child_count);
					serialize_u32(s, internal.hierarchy_index);
					serialize_u32(s, internal.prefab);
					serialize_string(s, misc.name);
					serialize_u64(s, misc.flags);
					serialize_v3_f32(s, transform.position);
					serialize_v4_f32(s, transform.rotation);
					serialize_v3_f32(s, transform.scale);
				}
			}
		}

		// Components
		{
			foreach(id, scene_state->component_register_count) {

				serialize_u32(s, get_component_count(id));

				for (CompIt it = comp_it_begin(id, CompItFlag_Once);
					 it.has_next;
					 comp_it_next(it))
				{
					if (it.prefab == 0) {
						serialize_entity(s, it.entity);
						serialize_component(id, it.comp, s);
					}
					else serialize_entity(s, 0);
				}
			}
		}
	}

	bool deserialize_ecs(Deserializer& d)
	{
		SV_ECS();

		u32 version;
		deserialize_u32(d, version);

		if (version <= 0) {

			SV_LOG_ERROR("ECS version %u not supported", version);
			return false;
		}
		
		struct TempComponentRegister {
			char name[COMPONENT_NAME_SIZE + 1u];
			u32 size;
			u32 version;
			CompID id;
		};

		struct TempTagRegister {
			char name[COMPONENT_NAME_SIZE + 1u];
			u32 id;
		};
		
		// Registers
		List<TempComponentRegister> component_registers;
		List<TempTagRegister> tag_registers;
		
		{
			u32 register_count = 0u;
			deserialize_u32(d, register_count);
			
			foreach(i, register_count) {

				TempComponentRegister& reg = component_registers.emplace_back();
				
				deserialize_string(d, reg.name, COMPONENT_NAME_SIZE + 1u);			
				deserialize_u32(d, reg.size);
				deserialize_u32(d, reg.version);
			}

			if (version >= 2u) {

				u32 tag_count = 0u;
				deserialize_u32(d, tag_count);

				foreach(i, tag_count) {

					TempTagRegister& reg = tag_registers.emplace_back();

					deserialize_u32(d, reg.id);
					deserialize_string(d, reg.name, TAG_NAME_SIZE + 1u);
				}
			}
		}

		// Registers ID
		
		foreach(i, component_registers.size()) {

			TempComponentRegister& reg = component_registers[i];
			reg.id = get_component_id(reg.name);

			if (reg.id == INVALID_COMP_ID) {
				SV_LOG_ERROR("Component '%s' doesn't exist", reg.name);
				return false;
			}
		}

		// Prefabs
		struct PrefabRef {
			Prefab old_prefab;
			Prefab current_prefab;
		};
		
		List<PrefabRef> prefabs;
		{
			u32 prefab_count;
			deserialize_u32(d, prefab_count);

			foreach(i, prefab_count) {

				PrefabRef ref;
				
				deserialize_u32(d, ref.old_prefab);

				if (ref.old_prefab) {
					
					char filepath[FILEPATH_SIZE + 1u];
					deserialize_string(d, filepath, FILEPATH_SIZE + 1u);
				
					ref.current_prefab = load_prefab(filepath);

					if (ref.current_prefab) {
						prefabs.push_back(ref);
					}
				}
			}
		}

		// Entity data
		u32 entity_count;
		u32 entity_data_count;

		deserialize_u32(d, entity_count);
		deserialize_u32(d, entity_data_count);

		ecs.entity_hierarchy.resize(entity_count);
		EntityInternal* entity_internal = (EntityInternal*)SV_ALLOCATE_MEMORY(sizeof(EntityInternal) * entity_data_count, "Scene");
		EntityMisc* entity_misc = (EntityMisc*)SV_ALLOCATE_MEMORY(sizeof(EntityMisc) * entity_data_count, "Scene");
		EntityTransform* entity_transform = (EntityTransform*)SV_ALLOCATE_MEMORY(sizeof(EntityTransform) * entity_data_count, "Scene");

		foreach(i, entity_data_count) initialize_entity_internal(entity_internal[i]);
		foreach(i, entity_data_count) initialize_entity_misc(entity_misc[i]);
		foreach(i, entity_data_count) initialize_entity_transform(entity_transform[i]);

		foreach(i, entity_count) {

			Entity entity;
			deserialize_entity(d, entity);

			EntityInternal& internal = entity_internal[entity - 1u];
			EntityMisc& misc = entity_misc[entity - 1u];
			EntityTransform& transform = entity_transform[entity - 1u];

			if (version >= 2)
				deserialize_u64(d, internal.tag_mask);

			deserialize_u32(d, internal.child_count);
			deserialize_u32(d, internal.hierarchy_index);
			deserialize_u32(d, internal.prefab);
			internal.component_mask = 0u;
			internal.component_count = 0u;
			
			deserialize_string(d, misc.name, ENTITY_NAME_SIZE + 1u);
			deserialize_u64(d, misc.flags);
			
			deserialize_v3_f32(d, transform.position);
			deserialize_v4_f32(d, transform.rotation);
			deserialize_v3_f32(d, transform.scale);
			transform.dirty = true;
			transform.dirty_physics = true;

			if (version == 1u) {

				char tag_name[TAG_NAME_SIZE + 1u];
				deserialize_string(d, tag_name, TAG_NAME_SIZE + 1u);
			}

			// Notify tags
			if (internal.tag_mask) {

				u64 mask = 0u;

				foreach(tag, tag_registers.size()) {

					TempTagRegister& reg = tag_registers[tag];
					
					if (internal.tag_mask & SV_BIT(reg.id)) {

						foreach(i, TAG_MAX) {

							if (string_equals(scene_state->tag_register[i].name, reg.name)) {
								mask |= SV_BIT(i);
								ecs.tags[i].entities.push_back(entity);
								break;
							}
						}
					}
				}

				internal.tag_mask = mask;
			}

			// Find prefab
			if (internal.prefab) {

				for (PrefabRef ref : prefabs) {

					if (ref.old_prefab == internal.prefab) {
						internal.prefab = ref.current_prefab;
						ecs.prefabs[internal.prefab - 1u].entities.push_back(entity);
						break;
					}
				}
			}
		}

		// Create entity list and free list
		foreach(i, entity_data_count) {

			EntityInternal& internal = entity_internal[i];

			if (internal.hierarchy_index != u32_max) {
				ecs.entity_hierarchy[internal.hierarchy_index] = i + 1u;
			}
			else ecs.entity_free_list.push_back(i + 1u);
		}

		// Set entity parents
		{
			EntityInternal* it = entity_internal;
			EntityInternal* end = it + entity_data_count;

			while (it != end) {

				if (it->hierarchy_index != u32_max && it->child_count) {

					Entity* eIt = ecs.entity_hierarchy.data() + it->hierarchy_index;
					Entity* eEnd = eIt + it->child_count;
					Entity parent = *eIt++;

					while (eIt <= eEnd) {

						EntityInternal& ed = entity_internal[*eIt - 1u];
						ed.parent = parent;
						if (ed.child_count) {
							eIt += ed.child_count + 1u;
						}
						else ++eIt;
					}
				}

				++it;
			}
		}

		// Set entity data
		SV_ASSERT(ecs.entity_internal == NULL);
		ecs.entity_internal = entity_internal;
		ecs.entity_misc = entity_misc;
		ecs.entity_transform = entity_transform;
		ecs.entity_capacity = entity_data_count;
		ecs.entity_size = entity_data_count;

		// Components
		{
			foreach(reg_index, component_registers.size()) {

				TempComponentRegister& reg = component_registers[reg_index];
				CompID comp_id = reg.id;
				u32 version = reg.version;
				u32 comp_count;
				deserialize_u32(d, comp_count);

				if (comp_count == 0u) continue;

				while (comp_count-- != 0u) {

					Entity entity;
					deserialize_entity(d, entity);

					if (entity == 0) continue;

					Component* comp = allocate_component(comp_id);
					create_entity_component(comp_id, comp, entity);
					deserialize_component(comp_id, comp, d, version);

					EntityInternal& internal = ecs.entity_internal[entity - 1u];

					if (internal.component_count < ENTITY_COMPONENTS_MAX) {
						
						CompRef& ref = internal.components[internal.component_count++];
						ref.comp_id = comp_id;
						ref.comp = comp;
						
						internal.component_mask |= (1ULL << (u64)comp_id);
					}
					else {
						SV_LOG_ERROR("A deserialized entity have more than %u components", ENTITY_COMPONENTS_MAX);
						return false;
					}
				}
			}
		}

		// TODO: dispath all events at once
		for (Entity entity : ecs.entity_hierarchy) {

			EntityCreateEvent e;
			e.entity = entity;
			
			event_dispatch("on_entity_create", &e);
		}

		return true;
	}

	SV_AUX void allocate_entities()
	{
		SV_ECS();

		u32 new_entity_capacity = ecs.entity_capacity + ENTITY_ALLOCATION_POOL;
		
		EntityInternal* new_internal = (EntityInternal*) SV_ALLOCATE_MEMORY(sizeof(EntityInternal) * new_entity_capacity, "Scene");
		EntityMisc* new_misc = (EntityMisc*) SV_ALLOCATE_MEMORY(sizeof(EntityMisc) * new_entity_capacity, "Scene");
		EntityTransform* new_transform = (EntityTransform*) SV_ALLOCATE_MEMORY(sizeof(EntityTransform) * new_entity_capacity, "Scene");

		if (ecs.entity_capacity) {

			memcpy(new_internal, ecs.entity_internal, sizeof(EntityInternal) * ecs.entity_capacity);
			memcpy(new_misc, ecs.entity_misc, sizeof(EntityMisc) * ecs.entity_capacity);
			memcpy(new_transform, ecs.entity_transform, sizeof(EntityTransform) * ecs.entity_capacity);

			SV_FREE_MEMORY(ecs.entity_internal);
			SV_FREE_MEMORY(ecs.entity_misc);
			SV_FREE_MEMORY(ecs.entity_transform);
		}

		foreach(i, ENTITY_ALLOCATION_POOL) {

			EntityInternal& e = new_internal[i + ecs.entity_capacity];
			initialize_entity_internal(e);
		}
		foreach(i, ENTITY_ALLOCATION_POOL) {

			EntityMisc& e = new_misc[i + ecs.entity_capacity];
			initialize_entity_misc(e);
		}
		foreach(i, ENTITY_ALLOCATION_POOL) {

			EntityTransform& e = new_transform[i + ecs.entity_capacity];
			initialize_entity_transform(e);
		}

		ecs.entity_capacity = new_entity_capacity;
		
		ecs.entity_internal = new_internal;
		ecs.entity_misc = new_misc;
		ecs.entity_transform = new_transform;
	}

	Entity create_entity(Entity parent, const char* name, Prefab prefab)
	{
		SV_ECS();

		SV_ASSERT(parent == 0u || entity_exists(parent));

		// Allocate entity
		Entity entity;
		
		if (ecs.entity_free_list.size()) {

			entity = ecs.entity_free_list.back() - 1u;
			ecs.entity_free_list.pop_back();
		}
		else {

			entity = ecs.entity_size;

			if (ecs.entity_size == ecs.entity_capacity) {

				allocate_entities();
			}

			++ecs.entity_size;
		}
		
		EntityInternal& internal = ecs.entity_internal[entity];
		++entity;

		if (parent) {

			// notify parents
			{
				Entity aux = parent;

				while (aux != 0) {
				
					EntityInternal& parent_to_update = ecs.entity_internal[aux - 1u];
					++parent_to_update.child_count;
					aux = parent_to_update.parent;
				}
			}

			EntityInternal& parent_internal = ecs.entity_internal[parent - 1u];

			u32 index = parent_internal.hierarchy_index + parent_internal.child_count;
			ecs.entity_hierarchy.insert(entity, index);

			for (u32 i = index + 1u; i < ecs.entity_hierarchy.size(); ++i) {

				Entity e = ecs.entity_hierarchy[i] - 1u;
				++ecs.entity_internal[e].hierarchy_index;
			}

			internal.hierarchy_index = index;
			internal.parent = parent;
		}
		else {

			internal.hierarchy_index = u32(ecs.entity_hierarchy.size());
			ecs.entity_hierarchy.push_back(entity);
		}

		if (name)
			string_copy(ecs.entity_misc[entity - 1u].name, name, ENTITY_NAME_SIZE + 1u);

		if (prefab) {

			internal.prefab = prefab;

			PrefabInternal& p = ecs.prefabs[prefab - 1u];
			p.entities.push_back(entity);
		}

		EntityCreateEvent e;
		e.entity = entity;
		event_dispatch("on_entity_create", &e);

		return entity;
	}
	
	void destroy_entity(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		u32 count = internal.child_count + 1;

		// data to remove entities
		size_t index_begin_dst = internal.hierarchy_index;
		size_t index_begin_src= internal.hierarchy_index + count;
		size_t cpy_cant = (u32)ecs.entity_hierarchy.size() - index_begin_src;

		// Dispatch events
		{
			EntityDestroyEvent e;

			for (size_t i = 0; i < count; ++i) {
		
				e.entity = ecs.entity_hierarchy[index_begin_dst + i];
				event_dispatch("on_entity_destroy", &e);
			}
		}

		// Remove tag reference
		if (internal.tag_mask) {

			foreach(i, TAG_MAX) {

				if (internal.tag_mask & SV_BIT(i)) {

					TagInternal& tag = ecs.tags[i];

					u32 index = u32_max;
					foreach(j, tag.entities.size())
						if (tag.entities[j] == entity) {
							index = j;
							break;
						}

					if (index != u32_max)
						tag.entities.erase(index);
					else SV_ASSERT(0);
				}
			}
		}

		// remove prefab reference
		if (internal.prefab) {
			PrefabInternal& p = ecs.prefabs[internal.prefab - 1u];
			u32 index = u32_max;

			foreach(i, p.entities.size()) {
				if (p.entities[i] == entity) {
					index = i;
					break;
				}
			}

			SV_ASSERT(index != u32_max);
			if (index != u32_max) {

				p.entities.erase(index);
			}
		}

		// notify parents
		{
			Entity aux = internal.parent;

			while (aux != 0) {
				
				EntityInternal& parent_to_update = ecs.entity_internal[aux - 1u];
				parent_to_update.child_count -= count;
				aux = parent_to_update.parent;
			}
		}

		// remove components & entityData
		for (size_t i = 0; i < count; i++) {
			
			Entity e = ecs.entity_hierarchy[index_begin_dst + i];
			EntityInternal& ed = ecs.entity_internal[e- 1u];
			
			foreach(j, ed.component_count) {
				
				CompRef comp = ed.components[j];					
				free_component(comp);
			}

			initialize_entity_internal(ecs.entity_internal[e - 1u]);
			initialize_entity_misc(ecs.entity_misc[e - 1u]);
			initialize_entity_transform(ecs.entity_transform[e - 1u]);

			if (e == ecs.entity_size) {
				--ecs.entity_size;
			}
			else {
				ecs.entity_free_list.push_back(e);
			}
		}

		// remove from entities & update indices
		if (cpy_cant != 0) memcpy(&ecs.entity_hierarchy[index_begin_dst], &ecs.entity_hierarchy[index_begin_src], cpy_cant * sizeof(Entity));
		ecs.entity_hierarchy.resize((u32)ecs.entity_hierarchy.size() - count);
		for (size_t i = index_begin_dst; i < ecs.entity_hierarchy.size(); ++i) {
			ecs.entity_internal[ecs.entity_hierarchy[i] - 1u].hierarchy_index = (u32)i;
		}
	}

	SV_INTERNAL Entity entity_duplicate_recursive(Entity duplicated, Entity parent)
    {
		SV_ECS();
	
		Entity copy;

		copy = create_entity(parent);

		EntityMisc& duplicated_misc = ecs.entity_misc[duplicated - 1u];
		EntityMisc& copy_misc = ecs.entity_misc[copy - 1u];
		strcpy(copy_misc.name, duplicated_misc.name);
		copy_misc.flags = duplicated_misc.flags;

		ecs.entity_transform[copy - 1u] = ecs.entity_transform[duplicated - 1u];
		
		EntityInternal& duplicated_internal = ecs.entity_internal[duplicated - 1u];
		EntityInternal& copy_internal = ecs.entity_internal[copy - 1u];

		if (duplicated_internal.tag_mask) {

			foreach(i, TAG_MAX) {

				if (duplicated_internal.tag_mask & SV_BIT(i)) {

					TagInternal& tag = ecs.tags[i];
					tag.entities.push_back(copy);
				}
			}

			copy_internal.tag_mask = duplicated_internal.tag_mask;
		}

		if (duplicated_internal.prefab) {

			copy_internal.prefab = duplicated_internal.prefab;

			PrefabInternal& internal = ecs.prefabs[copy_internal.prefab - 1u];
			internal.entities.push_back(copy);
		}

		foreach(i, duplicated_internal.component_count) {

			CompID comp_id = duplicated_internal.components[i].comp_id;
			Component* comp = duplicated_internal.components[i].comp;

			CompRef& ref = copy_internal.components[copy_internal.component_count++];
			ref.comp = allocate_component(comp_id);
			copy_component(comp_id, ref.comp, comp, copy);
			ref.comp_id = comp_id;
		}

		foreach(i, ecs.entity_internal[duplicated - 1u].child_count) {
			Entity to_copy = ecs.entity_hierarchy[ecs.entity_internal[duplicated - 1].hierarchy_index + i + 1];
			entity_duplicate_recursive(to_copy, copy);
			i += ecs.entity_internal[to_copy - 1u].child_count;
		}

		return copy;
    }

	Entity duplicate_entity(Entity entity)
	{
		SV_ECS();
		Entity res = entity_duplicate_recursive(entity, ecs.entity_internal[entity - 1u].parent);
		return res;
	}

	bool entity_exists(Entity entity)
	{
		SV_ECS();
		if (entity) {
			--entity;
			return ecs.entity_size > entity && ecs.entity_internal[entity].hierarchy_index != u32_max;
		}

		return false;
	}
	
	const char* get_entity_name(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		return ecs.entity_misc[entity - 1u].name;
	}
	
	u64 get_entity_flags(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		return ecs.entity_misc[entity - 1u].flags;
	}

	void set_entity_name(Entity entity, const char* name)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		string_copy(ecs.entity_misc[entity - 1u].name, name, ENTITY_NAME_SIZE + 1u);
	}
	
    u32 get_entity_childs_count(Entity parent)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(parent));
		return ecs.entity_internal[parent - 1u].child_count;
	}
	
    void get_entity_childs(Entity parent, Entity const** childs)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(parent));
		const EntityInternal& ed = ecs.entity_internal[parent - 1u];
		if (childs && ed.child_count != 0)* childs = &ecs.entity_hierarchy[ed.hierarchy_index + 1];
	}
	
    Entity get_entity_parent(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		return ecs.entity_internal[entity - 1u].parent;
	}
	
    u32 get_entity_component_count(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		const EntityInternal& internal = ecs.entity_internal[entity - 1u];

		u32 count = internal.component_count;

		if (internal.prefab) {

			PrefabInternal& p = ecs.prefabs[internal.prefab - 1u];
			count += p.component_count;
		}

		return count;
	}
	
	CompRef get_entity_component_by_index(Entity entity, u32 index)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		const EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (index < internal.component_count) {

			return internal.components[index];
		}
		else if (internal.prefab) {

			PrefabInternal& p = ecs.prefabs[internal.prefab - 1u];
			index -= internal.component_count;

			return p.components[index];
		}

		SV_ASSERT(0);
		return {};
	}
	
    u32 get_entity_count()
	{
		SV_ECS();
		return (u32)ecs.entity_hierarchy.size();
	}
	
    Entity get_entity_by_index(u32 index)
	{
		SV_ECS();
		return ecs.entity_hierarchy[index];
	}

	bool has_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		
		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		if (internal.component_mask & (1ULL << (u64)comp_id)) return true;

		else if (internal.prefab) {

			return has_prefab_component(internal.prefab, comp_id);
		}

		return false;
	}
	
	Component* add_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		if (!component_exists(comp_id))
			return NULL;

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (has_entity_component(entity, comp_id)) {
			SV_LOG_ERROR("The entity component is repeated");
			return NULL;
		}

		if (internal.component_count == ENTITY_COMPONENTS_MAX) {
			SV_LOG_ERROR("A entity can't have more than %u components", ENTITY_COMPONENTS_MAX);
			return NULL;
		}

		Component* component = allocate_component(comp_id);
		
		create_entity_component(comp_id, component, entity);

		internal.components[internal.component_count].comp_id = comp_id;
		internal.components[internal.component_count].comp = component;
		
		++internal.component_count;

		internal.component_mask |= SV_BIT(comp_id);

		return component;
	}
	
	void remove_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (internal.component_mask & SV_BIT(comp_id)) {

			internal.component_mask &= ~SV_BIT(comp_id);

			CompRef comp_ref;
			comp_ref.comp = NULL;
			u32 component_index = 0u;

			foreach(i, internal.component_count) {
				
				const CompRef& c = internal.components[i];

				if (c.comp_id == comp_id) {
					comp_ref = c;
					component_index = i;
					break;
				}
			}

			SV_ASSERT(comp_ref.comp);

			if (comp_ref.comp) {

				for (u32 i = component_index + 1u; i < internal.component_count; ++i) {
					internal.components[i - 1u] = internal.components[i];
				}
				--internal.component_count;

				free_component(comp_ref);
			}			
		}
	}
	
	Component* get_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (internal.component_mask & (1ULL << (u64)comp_id)) {

			foreach(i, internal.component_count) {

				if (internal.components[i].comp_id == comp_id) {
					return internal.components[i].comp;
				}
			}

			SV_ASSERT(0);
			return NULL;
		}
		else if (internal.prefab) {
			return get_prefab_component(internal.prefab, comp_id);
		}

		return NULL;
	}

	bool has_entity_tag(Entity entity, Tag tag)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		return internal.tag_mask & SV_BIT(tag);
	}
	
	void add_entity_tag(Entity entity, Tag tag)
	{
		if (!tag_exists(tag)) return;
		
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (!(internal.tag_mask & SV_BIT(tag))) {

			internal.tag_mask |= SV_BIT(tag);

			TagInternal& tag_internal = ecs.tags[tag];
			tag_internal.entities.push_back(entity);
		}
	}
	
	void remove_entity_tag(Entity entity, Tag tag)
	{
		if (!tag_exists(tag)) return;
		
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (internal.tag_mask & SV_BIT(tag)) {

			internal.tag_mask = internal.tag_mask & ~SV_BIT(tag);

			TagInternal& tag_internal = ecs.tags[tag];

			foreach(i, tag_internal.entities.size()) {
				if (tag_internal.entities[i] == entity) {
					tag_internal.entities.erase(i);
					return;
				}
			}

			SV_ASSERT(0);
		}
	}

	bool create_prefab_file(const char* name, const char* filepath)
	{
		name = string_validate(name);
		size_t size = string_size(name);
		if (size == 0u || size > ENTITY_NAME_SIZE) {
			SV_LOG_ERROR("Invalid prefab name '%s'", name);
			return false;
		}
		
		Serializer s;
		serialize_begin(s);
		
		serialize_u32(s, 0u); // VERSION

		serialize_string(s, name);
		
		serialize_u32(s, 0u); // Component count
		
		return serialize_end(s, filepath);
	}

	SV_AUX bool is_valid_prefab_name(const char* name, Prefab prefab_filter)
	{
		name = string_validate(name);
		size_t size = string_size(name);
		if (size == 0u) {
			SV_LOG_ERROR("The prefab name is empty");
			return false;
		}
		if (size > ENTITY_NAME_SIZE) {
			SV_LOG_ERROR("The prefab name is too long");
			return false;
		}

		for (PrefabIt it = prefab_it_begin();
			 it.has_next;
			 prefab_it_next(it))
		{
			if (it.prefab == prefab_filter)
				continue;
			
			if (string_equals(get_prefab_name(it.prefab), name)) {
				SV_LOG_ERROR("Repeated prefab name '%s'", name);
				return false;
			}
		}

		return true;
	}

	SV_AUX Prefab allocate_prefab()
	{
		SV_ECS();

		Prefab prefab = 0;

		if (ecs.prefab_free_count) {

			foreach(i, ecs.prefabs.size()) {
				if (!ecs.prefabs[i].valid) {
					prefab = i + 1u;
					break;
				}
			}

			--ecs.prefab_free_count;

			if (prefab == 0)
				SV_ASSERT(0);
		}
		else {

			prefab = (u32)ecs.prefabs.size() + 1u;
			ecs.prefabs.emplace_back();
		}

		ecs.prefabs[prefab - 1u].valid = true;
		return prefab;
	}

	SV_AUX void free_prefab(Prefab prefab)
	{
		SV_ECS();

		PrefabInternal& p = ecs.prefabs[prefab - 1u];
		p.valid = false;
		p.entities.clear();
		p.component_count = 0u;
		p.component_mask = 0u;

		++ecs.prefab_free_count;
	}
	
	Prefab load_prefab(const char* filepath)
	{
		SV_ECS();

		Prefab prefab = 0;
		
		Deserializer d;
		if (deserialize_begin(d, filepath)) {

			u32 version;
			deserialize_u32(d, version);

			
			prefab = allocate_prefab();
			
			PrefabInternal& p = ecs.prefabs[prefab - 1u];
			p.component_mask = 0u;
			
			deserialize_string(d, p.name, ENTITY_NAME_SIZE + 1u);

			if (!is_valid_prefab_name(p.name, prefab)) {
				free_prefab(prefab);
				return 0;
			}
			
			deserialize_u32(d, p.component_count);

			if (p.component_count > ENTITY_COMPONENTS_MAX) {
				SV_LOG_ERROR("The prefab '%s' exceeds the component limit of %u", filepath, ENTITY_COMPONENTS_MAX);
				free_prefab(prefab);
				return 0;
			}

			foreach(i, p.component_count) {

				CompRef& ref = p.components[i];

				char comp_name[COMPONENT_NAME_SIZE + 1u];
				u32 comp_version;

				deserialize_string(d, comp_name, COMPONENT_NAME_SIZE + 1u);
				deserialize_u32(d, comp_version);

				ref.comp_id = get_component_id(comp_name);

				if (ref.comp_id == INVALID_COMP_ID) {
					SV_LOG_ERROR("Can't load the prefab '%s', the component '%s' doesn't exists", filepath, comp_name);
					free_prefab(prefab);
					return 0;
				}
				
				ref.comp = allocate_component(ref.comp_id);
				create_prefab_component(ref.comp_id, ref.comp, prefab);
				deserialize_component(ref.comp_id, ref.comp, d, comp_version);

				p.component_mask |= SV_BIT(ref.comp_id);
				p.components[i] = ref;
			}

			string_copy(p.filepath, filepath, FILEPATH_SIZE + 1u);

			deserialize_end(d);
		}
		else {
			SV_LOG_ERROR("Can't load the prefab '%s'", filepath);
			return 0;
		}
		
		return prefab;
	}

	bool save_prefab(Prefab prefab)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& p = ecs.prefabs[prefab - 1u];
		return save_prefab(prefab, p.filepath);
	}
	
	bool save_prefab(Prefab prefab, const char* filepath)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& p = ecs.prefabs[prefab - 1u];
		
		Serializer s;
		serialize_begin(s);

		serialize_u32(s, 0u); // VERSION
		serialize_string(s, p.name);
		serialize_u32(s, p.component_count);

		foreach(i, p.component_count) {

			CompRef ref = p.components[i];
			serialize_string(s, get_component_name(ref.comp_id));
			serialize_u32(s, get_component_version(ref.comp_id));
			serialize_component(ref.comp_id, ref.comp, s);
		}
		
		return serialize_end(s, filepath);
	}

	bool prefab_exists(Prefab prefab)
	{
		SV_ECS();
		
		if (prefab) {

			--prefab;

			if (prefab < ecs.prefabs.size()) {

				return ecs.prefabs[prefab].valid;
			}
			
			return false;
		}
		return false;
	}

	const char* get_prefab_name(Prefab prefab)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		return ecs.prefabs[prefab - 1u].name;
	}

	bool has_prefab_component(Prefab prefab, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));
		
		PrefabInternal& internal = ecs.prefabs[prefab - 1u];
		return internal.component_mask & SV_BIT(comp_id);
	}
	
	Component* add_prefab_component(Prefab prefab, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& internal = ecs.prefabs[prefab - 1u];

		if (has_prefab_component(prefab, comp_id)) {
			SV_LOG_ERROR("The prefab component is repeated");
			return NULL;
		}

		if (internal.component_count == ENTITY_COMPONENTS_MAX) {
			SV_LOG_ERROR("A prefab can't have more than %u components", ENTITY_COMPONENTS_MAX);
			return NULL;
		}

		Component* component = allocate_component(comp_id);
		
		create_prefab_component(comp_id, component, prefab);

		internal.components[internal.component_count].comp_id = comp_id;
		internal.components[internal.component_count].comp = component;
		
		++internal.component_count;

		internal.component_mask |= SV_BIT(comp_id);

		// Remove repeated components
		for (Entity entity : internal.entities) {

			remove_entity_component(entity, comp_id);
		}

		return component;
	}
	
	void remove_prefab_component(Prefab prefab, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& internal = ecs.prefabs[prefab - 1u];

		if (internal.component_mask & SV_BIT(comp_id)) {

			internal.component_mask &= ~SV_BIT(comp_id);

			CompRef comp_ref;
			comp_ref.comp = NULL;
			u32 component_index = 0u;

			foreach(i, internal.component_count) {
				
				const CompRef& c = internal.components[i];

				if (c.comp_id == comp_id) {
					comp_ref = c;
					component_index = i;
					break;
				}
			}

			SV_ASSERT(comp_ref.comp);

			if (comp_ref.comp) {

				for (u32 i = component_index + 1u; i < internal.component_count; ++i) {
					internal.components[i - 1u] = internal.components[i];
				}
				--internal.component_count;

				free_component(comp_ref);
			}			
		}
		else SV_ASSERT(0);
	}
	
	Component* get_prefab_component(Prefab prefab, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& internal = ecs.prefabs[prefab - 1u];

		if (internal.component_mask & SV_BIT(comp_id)) {

			foreach(i, internal.component_count) {

				if (internal.components[i].comp_id == comp_id) {
					return internal.components[i].comp;
				}
			}

			SV_ASSERT(0);
			return NULL;
		}

		return NULL;
	}

	u32 get_prefab_component_count(Prefab prefab)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& internal = ecs.prefabs[prefab - 1u];
		return internal.component_count;
	}
	
	CompRef get_prefab_component_by_index(Prefab prefab, u32 index)
	{
		SV_ECS();
		SV_ASSERT(prefab_exists(prefab));

		PrefabInternal& internal = ecs.prefabs[prefab - 1u];
		if (index < internal.component_count)
			return internal.components[index];

		SV_ASSERT(0);
		return {};
	}

	CompIt comp_it_begin(CompID comp_id, u32 flags)
	{
		SV_ECS();
		
		CompIt it;
		it.flags = flags;
		it.comp_id = comp_id;
		it.pool_index = u32_max;
		it.comp = NULL;
		it.entity = 0u;
		it.prefab = 0;
		it.entity_index = 0;

		ComponentRegister& reg = scene_state->component_register[comp_id];
		ComponentAllocator& alloc = ecs.component_allocator[comp_id];
		
		if (alloc.pool_count) {

			foreach(i, alloc.pool_count) {

				ComponentPool* pool = alloc.pools + i;
				
				if (pool->count && pool->free_count < pool->count) {
					it.pool_index = i;
					break;
				}
			}

			if (it.pool_index != u32_max) {

				ComponentPool* pool = alloc.pools + it.pool_index;

				SV_ASSERT(alloc.pools);
				it.comp = reinterpret_cast<Component*>(pool->data - reg.size);
			}
		}

		it.has_next = it.comp != NULL;

		if (it.has_next)
			comp_it_next(it);

		return it;
	}
	
	void comp_it_next(CompIt& it)
	{
		SV_ECS();

		bool next_component = true;

		if (it.prefab) {

			SV_ASSERT(prefab_exists(it.prefab));
			PrefabInternal& internal = ecs.prefabs[it.prefab - 1u];

			if (++it.entity_index < internal.entities.size() && !(it.flags & CompItFlag_Once)) {

				next_component = false;
				it.entity = internal.entities[it.entity_index];
			}
			else it.prefab = 0;
		}

		if (next_component) {
			
			CompID comp_id = it.comp_id;
		
			ComponentRegister& reg = scene_state->component_register[comp_id];
			ComponentAllocator& alloc = ecs.component_allocator[comp_id];

			u8* comp = (u8*)it.comp;
			ComponentPool* pool = alloc.pools + it.pool_index;

			do {

				comp += reg.size;
			
				if (comp >= pool->data + (pool->count * reg.size)) {

					do {
						++it.pool_index;

						if (it.pool_index >= alloc.pool_count) {

							it.has_next = false;
							return;
						}

						pool = alloc.pools + it.pool_index;
					}
					while (pool->count == 0u);

					comp = pool->data;
				}

				Component* c = reinterpret_cast<Component*>(comp);

				if (c->id == 0) continue;
				else if (c->id & SV_BIT(31)) {
						
					Prefab prefab = c->id & ~SV_BIT(31);
					SV_ASSERT(prefab_exists(prefab));

					List<Entity>& entities = ecs.prefabs[prefab - 1u].entities;

					if (it.flags & CompItFlag_Once) {

						it.comp = c;
						it.entity = 0;
						it.entity_index = 0u;
						it.prefab = prefab;
						break;
					}
					else if (entities.size()) {

						it.comp = c;
						it.entity = entities.front();
						it.entity_index = 0u;
						it.prefab = prefab;
						break;
					}
				}
				else {
					it.comp = c;
					it.entity = c->id;
					break;
				}
			}
			while(1);
		}
	}

	PrefabIt prefab_it_begin()
	{
		SV_ECS();
		
		PrefabIt it;

		if (ecs.prefabs.empty()) it.has_next = false;
		else {

			PrefabInternal* pre = ecs.prefabs.data();
			PrefabInternal* end = pre + ecs.prefabs.size();

			while (pre != end) {

				if (pre->valid) {
					break;
				}
				
				++pre;
			}

			it.has_next = pre != end;

			if (it.has_next) {

				it.ptr = pre;
				it.prefab = Prefab(pre - ecs.prefabs.data()) + 1u;
			}
		}

		return it;
	}
	
	void prefab_it_next(PrefabIt& it)
	{
		SV_ECS();

		PrefabInternal* pre = (PrefabInternal*)it.ptr;
		
		do {
			++pre;

			if (pre >= ecs.prefabs.data() + ecs.prefabs.size()) {
				it.has_next = false;
				return;
			}
		}
		while (!pre->valid);

		it.ptr = pre;
		it.prefab = Prefab(pre - ecs.prefabs.data()) + 1u;
	}

	TagIt tag_it_begin(Tag tag)
	{
		SV_ECS();
		TagIt it;
		
		if (tag_exists(tag)) {

			TagInternal& internal = ecs.tags[tag];

			if (internal.entities.size()) {

				it.tag = tag;
				it._index = 0u;
				it.entity = internal.entities.front();
				it.has_next = true;
			}
			else it.has_next = false;
		}
		else it.has_next = false;
		return it;
	}
	
	void tag_it_next(TagIt& it)
	{
		SV_ECS();
		++it._index;

		TagInternal& internal = ecs.tags[it.tag];
		
		if (it._index >= internal.entities.size())
			it.has_next = false;
		else {
			it.entity = internal.entities[it._index];
		}
	}

	Tag get_tag_id(const char* name)
	{
		foreach(tag, TAG_MAX) {
			if (string_equals(scene_state->tag_register[tag].name, name))
				return tag;
		}

		return TAG_INVALID;
	}

	const char* get_tag_name(Tag tag)
	{
		SV_ASSERT(tag < TAG_MAX);
		return scene_state->tag_register[tag].name;
	}

	bool tag_exists(Tag tag)
	{
		if (tag < TAG_MAX) return scene_state->tag_register[tag].name[0] != '\0';
		return false;
	}

	Entity get_tag_entity(Tag tag)
	{
		SV_ECS();
		Entity entity = ecs.tags[tag].entities.empty() ? 0 : ecs.tags[tag].entities.front();
		return entity;
	}

	bool create_tag(const char* name)
	{
		if (name[0] == '\0') {
			SV_LOG_ERROR("Invalid tag name '%s'", name);
			return false;
		}
		
		Tag tag = get_tag_id(name);

		if (tag_exists(tag)) {
			SV_LOG_ERROR("The tag name '%s' is duplicated", name);
			return false;
		}

		tag = TAG_INVALID;
		
		foreach(i, TAG_MAX) {
			if (scene_state->tag_register[i].name[0] == '\0') {
				tag = Tag(i);
				break;
			}
		}

		if (tag == TAG_INVALID) {
			SV_LOG_ERROR("Can't add more tags, exceeds the limits (%u)", TAG_MAX);
			return false;
		}

		char str[TAG_NAME_SIZE + 2u] = "\n";
		string_append(str, name, TAG_NAME_SIZE + 2u);

		if (file_write_text("tags.txt", str, string_size(str), true)) {
			SV_LOG_INFO("Tag '%s' created", name);
			string_copy(scene_state->tag_register[tag].name, name, TAG_NAME_SIZE + 1u);
			return true;
		}

		SV_LOG_ERROR("Can't create the tag '%s'", name);
		return false;
	}

    struct ComponentRegisterDesc {

		const char*            name;
		u32			           size;
		u32                    version;
		CreateComponentFn	   create_fn;
		DestroyComponentFn     destroy_fn;
		CopyComponentFn	       copy_fn;
		SerializeComponentFn   serialize_fn;
		DeserializeComponentFn deserialize_fn;
		Library                library;
		const char*            struct_name;

    };

	SV_AUX bool register_component(ComponentRegisterDesc desc)
	{
		ComponentRegister& reg = scene_state->component_register[scene_state->component_register_count++];
		// TODO: Assert names, sizes and function pointers

		string_copy(reg.name, desc.name, COMPONENT_NAME_SIZE + 1u);
		reg.size = desc.size;
		reg.version = desc.version;
		reg.create_fn = desc.create_fn;
		reg.destroy_fn = desc.destroy_fn;
		reg.copy_fn = desc.copy_fn;
		reg.serialize_fn = desc.serialize_fn;
		reg.deserialize_fn = desc.deserialize_fn;
		reg.library = desc.library;
		string_copy(reg.struct_name, desc.struct_name, COMPONENT_NAME_SIZE + 1u);
		
		return true;
	}

	typedef void(*RegisterComponentFn)(void**, void**, void**, void**, void**, u32*, u32*, char*);

#if SV_EDITOR

	void reload_component(ReloadPluginEvent* e) {

		List<Var> vars;
		
		if (read_var_file("components.txt", vars)) {

			foreach(i, SV_MIN(vars.size(), COMPONENT_MAX)) {

				Var& var = vars[i];

				if (var.type != VarType_String) {
					SV_LOG_ERROR("Invalid var type '%s'", var.name);
					continue;
				}

				if (string_equals(e->name, var.value.string)) {

					Library lib = e->library;

					constexpr size_t s = COMPONENT_NAME_SIZE + 9;
					char name[s] = "_define_";
					string_append(name, var.name, s);
					
					RegisterComponentFn reg_fn = (RegisterComponentFn)os_library_proc_address(lib, name);
					if (reg_fn) {

						CompID id = INVALID_COMP_ID;

						foreach(i, scene_state->component_register_count) {

							ComponentRegister& reg = scene_state->component_register[i];
							if (string_equals(reg.struct_name, var.name)) {
								id = (CompID)i;
								break;
							}
						}

						if (component_exists(id)) {
							
							ComponentRegister& reg = scene_state->component_register[id];
							reg.name[0] = '\0';
							
							reg_fn((void**)&reg.create_fn, (void**)&reg.destroy_fn, (void**)&reg.copy_fn, (void**)&reg.serialize_fn, (void**)&reg.deserialize_fn, &reg.version, &reg.size, reg.name);
						}
					}
					else {
						SV_LOG_ERROR("Component '%s' not defined in '%s'", var.name, var.value.string);
					}
				}
			}
		}
	}
	
#endif

	template<typename T>
	SV_AUX bool register_component(const char* name)
	{
		ComponentRegisterDesc desc;
		desc.name = name;
		desc.size = sizeof(T);
		desc.version = T::VERSION;
		desc.library = 0;
		desc.struct_name = "";

		desc.create_fn = [](Component* comp, Entity entity)
			{
				new(comp) T();
			};

		desc.destroy_fn = [](Component* comp, Entity entity)
			{
				T* c = reinterpret_cast<T*>(comp);
				c->~T();
			};

		desc.copy_fn = [](Component* dst, const Component* src, Entity entity)
			{
				const T* from = reinterpret_cast<const T*>(src);
				T* to = reinterpret_cast<T*>(dst);
				u32 id = to->id;
				*to = *from;
				to->id = id;
			};

		desc.serialize_fn = [](Component* comp_, Serializer& s)
			{
				T* comp = reinterpret_cast<T*>(comp_);
				comp->serialize(s);
			};

		desc.deserialize_fn = [](Component* comp_, Deserializer& d, u32 version)
			{
				T* comp = reinterpret_cast<T*>(comp_);
				comp->deserialize(d, version);
			};

		return register_component(desc);
	}

	void register_components()
	{
		register_component<SpriteComponent>("Sprite");
		register_component<AnimatedSpriteComponent>("Animated Sprite");
		register_component<CameraComponent>("Camera");
		register_component<MeshComponent>("Mesh");
		register_component<TerrainComponent>("Terrain");
		register_component<ParticleSystem>("Particle System");
		register_component<LightComponent>("Light");

		ComponentRegisterDesc desc;
		desc.library = 0;
		desc.struct_name = "";
		
		desc.name = "Body";
		desc.size = sizeof(BodyComponent);
		desc.version = BodyComponent::VERSION;
		desc.create_fn = (CreateComponentFn)BodyComponent_create;
		desc.destroy_fn = (DestroyComponentFn)BodyComponent_destroy;
		desc.copy_fn = (CopyComponentFn)BodyComponent_copy;
		desc.serialize_fn = (SerializeComponentFn)BodyComponent_serialize;
		desc.deserialize_fn = (DeserializeComponentFn)BodyComponent_deserialize;

		register_component(desc);
		
		desc.name = "Box Collider";
		desc.size = sizeof(BoxCollider);
		desc.version = BoxCollider::VERSION;
		desc.create_fn = (CreateComponentFn)BoxCollider_create;
		desc.destroy_fn = (DestroyComponentFn)BoxCollider_destroy;
		desc.copy_fn = (CopyComponentFn)BoxCollider_copy;
		desc.serialize_fn = (SerializeComponentFn)BoxCollider_serialize;
		desc.deserialize_fn = (DeserializeComponentFn)BoxCollider_deserialize;

		register_component(desc);

		desc.name = "Sphere Collider";
		desc.size = sizeof(SphereCollider);
		desc.version = SphereCollider::VERSION;
		desc.create_fn = (CreateComponentFn)SphereCollider_create;
		desc.destroy_fn = (DestroyComponentFn)SphereCollider_destroy;
		desc.copy_fn = (CopyComponentFn)SphereCollider_copy;
		desc.serialize_fn = (SerializeComponentFn)SphereCollider_serialize;
		desc.deserialize_fn = (DeserializeComponentFn)SphereCollider_deserialize;

		register_component(desc);

		desc.name = "Audio Source";
		desc.size = sizeof(AudioSourceComponent);
		desc.version = AudioSourceComponent::VERSION;
		desc.create_fn = (CreateComponentFn)AudioSourceComponent_create;
		desc.destroy_fn = (DestroyComponentFn)AudioSourceComponent_destroy;
		desc.copy_fn = (CopyComponentFn)AudioSourceComponent_copy;
		desc.serialize_fn = (SerializeComponentFn)AudioSourceComponent_serialize;
		desc.deserialize_fn = (DeserializeComponentFn)AudioSourceComponent_deserialize;

		register_component(desc);

		// Register user components

		List<Var> vars;
		
		if (read_var_file("components.txt", vars)) {

			foreach(i, SV_MIN(vars.size(), COMPONENT_MAX)) {

				Var& var = vars[i];

				if (var.type != VarType_String) {
					SV_LOG_ERROR("Invalid var type '%s'", var.name);
					continue;
				}

				Library lib;
				
				if (get_plugin_by_name(var.value.string, &lib)) {

					constexpr size_t s = COMPONENT_NAME_SIZE + 9;
					char name[s] = "_define_";
					string_append(name, var.name, s);
					
					RegisterComponentFn reg_fn = (RegisterComponentFn)os_library_proc_address(lib, name);
					if (reg_fn) {

						char struct_name[COMPONENT_NAME_SIZE + 1u];
						desc.struct_name = struct_name;

						string_copy(struct_name, var.name, COMPONENT_NAME_SIZE + 1u);

						name[0] = '\0';
						desc.size = 0;
						desc.version = 0;
						desc.create_fn = NULL;
						desc.destroy_fn = NULL;
						desc.copy_fn = NULL;
						desc.serialize_fn = NULL;
						desc.deserialize_fn = NULL;
						desc.library = lib;

						reg_fn((void**)&desc.create_fn, (void**)&desc.destroy_fn, (void**)&desc.copy_fn, (void**)&desc.serialize_fn, (void**)&desc.deserialize_fn, &desc.version, &desc.size, name);

						desc.name = name;

						register_component(desc);
					}
					else {
						SV_LOG_ERROR("Component '%s' not defined in '%s'", var.name, var.value.string);
					}
				}
				else SV_LOG_ERROR("Plugin '%s' not found", var.value.string);
			}
		}

		foreach(i, TAG_MAX)
			scene_state->tag_register[i].name[0] = '\0';

		// TEMP

		vars.reset();
		
		if (read_var_file("tags.txt", vars)) {

			foreach(i, SV_MIN(vars.size(), TAG_MAX)) {

				string_copy(scene_state->tag_register[i].name, vars[i].name, TAG_NAME_SIZE + 1u);
			}
		}
	}
	
	void unregister_components()
	{
		scene_state->component_register_count = 0u;
	}

	const char* get_component_name(CompID ID)
	{
		ComponentRegister& reg = scene_state->component_register[ID];
		return reg.name;
	}
	
    u32 get_component_size(CompID ID)
	{
		ComponentRegister& reg = scene_state->component_register[ID];
		return reg.size;
	}
	
    u32 get_component_version(CompID ID)
	{
		ComponentRegister& reg = scene_state->component_register[ID];
		return reg.version;
	}
	
    CompID get_component_id(const char* name)
	{
		// TODO: Optimize

		foreach(i, scene_state->component_register_count) {
			
			ComponentRegister& reg = scene_state->component_register[i];
			if (string_equals(name, reg.name))
				return i;
		}
		return INVALID_COMP_ID;
	}
	
    u32 get_component_register_count()
	{
		return scene_state->component_register_count;
	}

	u32 get_component_count(CompID comp_id)
	{
		SV_ECS();

		ComponentAllocator& alloc = ecs.component_allocator[comp_id];

		u32 count = 0u;

		foreach(i, alloc.pool_count) {

			ComponentPool* pool = alloc.pools + i;
			count += pool->count - pool->free_count;
		}

		return count;
	}
	
    bool component_exists(CompID ID)
	{
		return ID < scene_state->component_register_count;
	}

	SV_AUX void update_world_matrix(EntityTransform& t, Entity entity)
    {
		SV_ECS();
	
		t.dirty = false;

		XMMATRIX m = get_entity_matrix(entity);

		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		Entity parent = internal.parent;

		if (parent != 0) {
	    
			XMMATRIX mp = get_entity_world_matrix(parent);
			m = m * mp;
		}
	
		XMStoreFloat4x4(&t.world_matrix, m);
    }

    SV_AUX void notify_transform(EntityTransform& t, Entity entity)
    {
		SV_ECS();
	
		if (!t.dirty) {

			t.dirty = true;
			t.dirty_physics = true;

			EntityInternal& internal = ecs.entity_internal[entity - 1];

			if (internal.child_count == 0) return;

			for (u32 i = 0; i < internal.child_count; ++i) {
				Entity e = ecs.entity_hierarchy[internal.hierarchy_index + 1 + i];
				EntityTransform& et = ecs.entity_transform[e - 1u];
				et.dirty = true;
				et.dirty_physics = true;
			}
		}
    }

    void set_entity_transform(Entity entity, const Transform& transform)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position = transform.position;
		t.rotation = transform.rotation;
		t.scale = transform.scale;
		notify_transform(t, entity);
    }
    
    void set_entity_position(Entity entity, const v3_f32& position)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position = position;
		notify_transform(t, entity);
    }
    
    void set_entity_rotation(Entity entity, const v4_f32& rotation)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.rotation = rotation;
		notify_transform(t, entity);
    }
    
    void set_entity_scale(Entity entity, const v3_f32& scale)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.scale = scale;
		notify_transform(t, entity);
    }
    
    void set_entity_matrix(Entity entity, const XMMATRIX& matrix)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
	
		notify_transform(t, entity);

		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, matrix);

		t.position = position;
		t.scale = scale;
		t.rotation = rotation;
    }

    void set_entity_position2D(Entity entity, const v2_f32& position)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position.x = position.x;
		t.position.y = position.y;
		notify_transform(t, entity);
    }

	void set_entity_scale2D(Entity entity, const v2_f32& scale)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.scale.x = scale.x;
		t.scale.y = scale.y;
		notify_transform(t, entity);
    }

	void set_entity_euler_rotation(Entity entity, v3_f32 euler_angles)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];

		t.rotation = XMQuaternionRotationRollPitchYaw(euler_angles.x, euler_angles.y, euler_angles.z);
		notify_transform(t, entity);
	}
	
	void set_entity_euler_rotationX(Entity entity, f32 rotation)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		//TODO t.rotation = XMQuaternionRotationRollPitchYaw(euler_angles.x, euler_angles.y, euler_angles.z);
		notify_transform(t, entity);
	}
	
	void set_entity_euler_rotationY(Entity entity, f32 rotation)
	{
	}
	
	void set_entity_euler_rotationZ(Entity entity, f32 rotation)
	{

	}
    
    Transform* get_entity_transform_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<Transform*>(&t.position);
    }
    
    v3_f32* get_entity_position_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.position;
    }
    
    v4_f32* get_entity_rotation_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.rotation;
    }
    
    v3_f32* get_entity_scale_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.scale;
    }
    
    v2_f32* get_entity_position2D_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<v2_f32*>(&t.position);
    }
    
    v2_f32* get_entity_scale2D_ptr(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<v2_f32*>(&t.scale);
    }
    
    Transform get_entity_transform(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		Transform trans;
		trans.position = t.position;
		trans.rotation = t.rotation;
		trans.scale = t.scale;
		return trans;
    }
    
    v3_f32 get_entity_position(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.position;
    }
    
    v4_f32 get_entity_rotation(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.rotation;
    }
    
    v3_f32 get_entity_scale(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.scale;
    }
    
    v2_f32 get_entity_position2D(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return vec3_to_vec2(t.position);
    }
    
    v2_f32 get_entity_scale2D(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return vec3_to_vec2(t.scale);
    }

    XMMATRIX get_entity_matrix(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		return XMMatrixScalingFromVector(vec3_to_dx(t.scale)) * XMMatrixRotationQuaternion(vec4_to_dx(t.rotation))
			* XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
    }

	v3_f32 get_entity_euler_rotation(Entity entity)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		v3_f32 euler;

		// roll (x-axis rotation)
		
		v4_f32 q = t.rotation;
		
		f32 sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
		f32 cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		euler.x = atan2f(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		float sinp = 2.f * (q.w * q.y - q.z * q.x);
		if (abs(sinp) >= 1.f)
			euler.y = copysignf(PI / 2.f, sinp); // use 90 degrees if out of range
		else
			euler.y = asinf(sinp);

		// yaw (z-axis rotation)
		f32 siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		f32 cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		euler.z = atan2f(siny_cosp, cosy_cosp);

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
	
	f32 get_entity_euler_rotationX(Entity entity)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		f32 euler;
		v4_f32 q = t.rotation;
		
		f32 sinr_cosp = 2.f * (q.w * q.x + q.y * q.z);
		f32 cosr_cosp = 1.f - 2.f * (q.x * q.x + q.y * q.y);
		euler = atan2f(sinr_cosp, cosr_cosp);

		if (euler < 0.f) {
			euler = 2.f * PI + euler;
		}

		return euler;
	}
	
	f32 get_entity_euler_rotationY(Entity entity)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		f32 euler;
		v4_f32 q = t.rotation;

		float sinp = 2.f * (q.w * q.y - q.z * q.x);
		if (abs(sinp) >= 1.f)
			euler = copysignf(PI / 2.f, sinp); // use 90 degrees if out of range
		else
			euler = asinf(sinp);
		
		if (euler < 0.f) {
			euler = 2.f * PI + euler;
		}

		return euler;
	}
	
	f32 get_entity_euler_rotationZ(Entity entity)
	{
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		f32 euler;
		v4_f32 q = t.rotation;
		
		f32 siny_cosp = 2 * (q.w * q.z + q.x * q.y);
		f32 cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
		euler = atan2f(siny_cosp, cosy_cosp);
		
		if (euler < 0.f) {
			euler = 2.f * PI + euler;
		}

		return euler;
	}

    Transform get_entity_world_transform(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];

		if (t.dirty) update_world_matrix(t, entity);
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
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		if (t.dirty) update_world_matrix(t, entity);
		return *(v3_f32*) &t.world_matrix._41;
    }
    
    v4_f32 get_entity_world_rotation(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		if (t.dirty) update_world_matrix(t, entity);
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&t.world_matrix));

		return v4_f32(rotation);
    }
    
    v3_f32 get_entity_world_scale(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		if (t.dirty) update_world_matrix(t, entity);
		return { vec3_length(*(v3_f32*)& t.world_matrix._11), vec3_length(*(v3_f32*)& t.world_matrix._21), vec3_length(*(v3_f32*)& t.world_matrix._31) };
    }
    
    XMMATRIX get_entity_world_matrix(Entity entity)
    {
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		if (t.dirty) update_world_matrix(t, entity);
		return XMLoadFloat4x4(&t.world_matrix);
    }

	//////////////////////////////////////////// COMPONENTS ////////////////////////////////////////////////////////

    bool SpriteSheet::add_sprite(u32* _id, const char* name, const v4_f32& texcoord)
    {
		if (name == nullptr) name = "Unnamed";
	
		size_t name_size = string_size(name);
		if (name_size > SPRITE_NAME_SIZE) {
			SV_LOG_ERROR("The sprite name '%s' is too large", name);
			return false;
		}

		// Check if have repeated sprite names
		for (auto it = sprites.begin();
			 it.has_next();
			 ++it)
		{
			Sprite& s = *it;

			if (string_equals(s.name, name)) {
				SV_LOG_ERROR("Can''t add the sprite '%s', the name is used", name);
				return false;
			}
		}

		u32 id = sprites.emplace();

		Sprite& s = sprites[id];
		s.texcoord = texcoord;
		string_copy(s.name, name, SPRITE_NAME_SIZE + 1u);

		if (_id) *_id = id;

		return true;
    }
	
	bool SpriteSheet::modify_sprite(u32 id, const char* name, const v4_f32& texcoord)
	{
		if (name == nullptr) name = "Unnamed";

		if (!sprites.exists(id)) {
			SV_LOG_ERROR("The sprite '%s' with id %u doesn`t exists", name, id);
			return false;
		}
	
		size_t name_size = string_size(name);
		if (name_size > SPRITE_NAME_SIZE) {
			SV_LOG_ERROR("The sprite name '%s' is too large", name);
			return false;
		}

		// Check if have repeated sprite names
		for (auto it = sprites.begin();
			 it.has_next();
			 ++it)
		{
			if (it.get_index() == id) continue;

			Sprite& s = *it;

			if (string_equals(s.name, name)) {
				SV_LOG_ERROR("Can''t modify the sprite '%s' to '%s', the name is used", sprites[id].name, name);
				return false;
			}
		}

		Sprite& s = sprites[id];
		s.texcoord = texcoord;
		string_copy(s.name, name, SPRITE_NAME_SIZE + 1u);

		return true;
    }

	u32 get_sprite_id(SpriteSheet* sheet, const char* name)
	{
		for (auto it = sheet->sprites.begin();
			 it.has_next();
			 ++it)
		{
			if (string_equals(it->name, name)) {
				return it.get_index();
			}
		}

		return u32_max;
	}
    
    bool SpriteSheet::add_sprite_animation(u32* _id, const char* name, u32* sprites_ptr, u32 frames, f32 frame_time)
    {
		if (name == nullptr) name = "Unnamed";
	
		size_t name_size = string_size(name);
		if (name_size > SPRITE_NAME_SIZE) {
			SV_LOG_ERROR("The sprite animation name '%s' is too large", name);
			return false;
		}

		// Check if have repeated sprite names
		for (auto it = sprite_animations.begin();
			 it.has_next();
			 ++it)
		{
			SpriteAnimation& s = *it;

			if (string_equals(s.name, name)) {
				SV_LOG_ERROR("Can''t add the sprite animation '%s', the name is used", name);
				return false;
			}
		}

		u32 id = sprite_animations.emplace();

		SpriteAnimation& s = sprite_animations[id];
		string_copy(s.name, name, SPRITE_NAME_SIZE + 1u);
		memcpy(s.sprites, sprites_ptr, frames * sizeof(u32));
		s.frames = frames;
		s.frame_time = frame_time;

		if (_id) *_id = id;

		return true;
    }

    v4_f32 SpriteSheet::get_sprite_texcoord(u32 id)
    {
		if (sprites.exists(id))
			return sprites[id].texcoord;
		else return { 0.f, 0.f, 1.f, 1.f };
    }

    void serialize_sprite_sheet(Serializer& s, const SpriteSheet& sheet)
    {
		serialize_u32(s, SpriteSheet::VERSION);
		serialize_asset(s, sheet.texture);
	
		serialize_u32(s, (u32)sheet.sprites.size());
		for (auto it = sheet.sprites.begin();
			 it.has_next();
			 ++it)
		{
			const Sprite& spr = *it;

			serialize_string(s, spr.name);
			serialize_v4_f32(s, spr.texcoord);
		}
	
		serialize_u32(s, (u32)sheet.sprite_animations.size());
		for (auto it = sheet.sprite_animations.begin();
			 it.has_next();
			 ++it)
		{
			const SpriteAnimation& spr = *it;

			serialize_string(s, spr.name);
			serialize_u32(s, spr.frames);
			serialize_f32(s, spr.frame_time);

			foreach(i, spr.frames) {

				serialize_u32(s, spr.sprites[i]);
			}
		}
    }
    
    void deserialize_sprite_sheet(Deserializer& d, SpriteSheet& sheet)
    {
		u32 version;
		deserialize_u32(d, version);
		deserialize_asset(d, sheet.texture);
		
		u32 sprite_count, sprite_animation_count;
		
		deserialize_u32(d, sprite_count);
		
		foreach (i, sprite_count) {
			
			Sprite& spr = sheet.sprites[sheet.sprites.emplace()];

			deserialize_string(d, spr.name, SPRITE_NAME_SIZE + 1u);
			deserialize_v4_f32(d, spr.texcoord);
		}
	
		deserialize_u32(d, sprite_animation_count);
		
		foreach (i, sprite_animation_count) {
			
			SpriteAnimation& spr = sheet.sprite_animations[sheet.sprite_animations.emplace()];

			deserialize_string(d, spr.name, SPRITE_NAME_SIZE + 1u);
			deserialize_u32(d, spr.frames);
			deserialize_f32(d, spr.frame_time);

			foreach(i, spr.frames) {

				deserialize_u32(d, spr.sprites[i]);
			}
		}
    }

    void SpriteComponent::serialize(Serializer& s)
    {
		serialize_asset(s, sprite_sheet);
		serialize_u32(s, sprite_id);
		serialize_color(s, color);
		serialize_u32(s, layer);
    }

    void SpriteComponent::deserialize(Deserializer& d, u32 version)
    {
		// TODO: Deprecated
		if (version == 0u) {
			TextureAsset tex;
			v4_f32 texcoord;
			deserialize_asset(d, tex);
			deserialize_v4_f32(d, texcoord);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
		else {
			deserialize_asset(d, sprite_sheet);
			deserialize_u32(d, sprite_id);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
    }

    void AnimatedSpriteComponent::serialize(Serializer& s)
    {
		serialize_asset(s, sprite_sheet);
		serialize_u32(s, animation_id);
		serialize_u32(s, index);
		serialize_f32(s, time_mult);
		serialize_f32(s, simulation_time);
		serialize_color(s, color);
		serialize_u32(s, layer);
    }

    void AnimatedSpriteComponent::deserialize(Deserializer& d, u32 version)
    {
		// TODO: Deprecated
		if (version == 0u) {
			TextureAsset texture;
			u32 t;
			f32 f;
			deserialize_asset(d, texture);
			deserialize_u32(d, t);
			deserialize_u32(d, t);
			deserialize_u32(d, t);
			deserialize_u32(d, t);
			deserialize_f32(d, f);
			deserialize_u32(d, index);
			deserialize_f32(d, f);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
		else {
			deserialize_asset(d, sprite_sheet);
			deserialize_u32(d, animation_id);
			deserialize_u32(d, index);
			deserialize_f32(d, time_mult);
			deserialize_f32(d, simulation_time);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
    }

    void CameraComponent::serialize(Serializer& s)
    {
		serialize_bool(s, adjust_width);
		
		serialize_u32(s, projection_type);
		serialize_f32(s, near);
		serialize_f32(s, far);
		serialize_f32(s, width);
		serialize_f32(s, height);

		serialize_bool(s, bloom.active);
		serialize_f32(s, bloom.threshold);
		serialize_f32(s, bloom.intensity);

		serialize_bool(s, ssao.active);
		serialize_u32(s, ssao.samples);
		serialize_f32(s, ssao.radius);
		serialize_f32(s, ssao.bias);
    }

    void CameraComponent::deserialize(Deserializer& d, u32 version)
    {
		switch (version) {

		case 0:
			deserialize_u32(d, (u32&)projection_type);
			deserialize_f32(d, near);
			deserialize_f32(d, far);
			deserialize_f32(d, width);
			deserialize_f32(d, height);
			break;

		case 1:
			deserialize_u32(d, (u32&)projection_type);
			deserialize_f32(d, near);
			deserialize_f32(d, far);
			deserialize_f32(d, width);
			deserialize_f32(d, height);
			deserialize_bool(d, bloom.active);
			deserialize_f32(d, bloom.threshold);
			deserialize_f32(d, bloom.intensity);
			break;

		case 2:
			deserialize_u32(d, (u32&)projection_type);
			deserialize_f32(d, near);
			deserialize_f32(d, far);
			deserialize_f32(d, width);
			deserialize_f32(d, height);
			deserialize_bool(d, bloom.active);
			deserialize_f32(d, bloom.threshold);
			deserialize_f32(d, bloom.intensity);
			deserialize_bool(d, ssao.active);
			break;

		case 3:
			deserialize_u32(d, (u32&)projection_type);
			deserialize_f32(d, near);
			deserialize_f32(d, far);
			deserialize_f32(d, width);
			deserialize_f32(d, height);
			deserialize_bool(d, bloom.active);
			deserialize_f32(d, bloom.threshold);
			deserialize_f32(d, bloom.intensity);
			deserialize_bool(d, ssao.active);
			deserialize_u32(d, ssao.samples);
			deserialize_f32(d, ssao.radius);
			deserialize_f32(d, ssao.bias);
			break;

		case 4:
			deserialize_bool(d, adjust_width);
			deserialize_u32(d, (u32&)projection_type);
			deserialize_f32(d, near);
			deserialize_f32(d, far);
			deserialize_f32(d, width);
			deserialize_f32(d, height);
			deserialize_bool(d, bloom.active);
			deserialize_f32(d, bloom.threshold);
			deserialize_f32(d, bloom.intensity);
			deserialize_bool(d, ssao.active);
			deserialize_u32(d, ssao.samples);
			deserialize_f32(d, ssao.radius);
			deserialize_f32(d, ssao.bias);
			break;

		}
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
		serialize_bool(s, shadow_mapping_enabled);
		serialize_f32(s, cascade_distance[0]);
		serialize_f32(s, cascade_distance[1]);
		serialize_f32(s, cascade_distance[2]);
		serialize_f32(s, shadow_bias);
    }
    
    void LightComponent::deserialize(Deserializer& d, u32 version)
    {
		switch(version) {

		case 0:
			deserialize_u32(d, (u32&)light_type);
			deserialize_color(d, color);
			deserialize_f32(d, intensity);
			deserialize_f32(d, range);
			deserialize_f32(d, smoothness);
			break;

		case 1:
			deserialize_u32(d, (u32&)light_type);
			deserialize_color(d, color);
			deserialize_f32(d, intensity);
			deserialize_f32(d, range);
			deserialize_f32(d, smoothness);
			deserialize_bool(d, shadow_mapping_enabled);
			break;

		case 2:
			deserialize_u32(d, (u32&)light_type);
			deserialize_color(d, color);
			deserialize_f32(d, intensity);
			deserialize_f32(d, range);
			deserialize_f32(d, smoothness);
			deserialize_bool(d, shadow_mapping_enabled);
			deserialize_f32(d, cascade_distance[0]);
			deserialize_f32(d, cascade_distance[1]);
			deserialize_f32(d, cascade_distance[2]);
			break;

		case 3:
			deserialize_u32(d, (u32&)light_type);
			deserialize_color(d, color);
			deserialize_f32(d, intensity);
			deserialize_f32(d, range);
			deserialize_f32(d, smoothness);
			deserialize_bool(d, shadow_mapping_enabled);
			deserialize_f32(d, cascade_distance[0]);
			deserialize_f32(d, cascade_distance[1]);
			deserialize_f32(d, cascade_distance[2]);
			deserialize_f32(d, shadow_bias);
			break;
			
		}
    }
	
}
