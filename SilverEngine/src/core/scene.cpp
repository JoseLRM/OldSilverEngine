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

	struct EntityPrefab {

		// TODO: Optimize
		List<Entity> entities;
		
		char filepath[FILEPATH_SIZE + 1u];
		Entity entity;
		
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
		Entity prefab;

		EntityPrefab* own_prefab;
		
	};

	struct EntityMisc {
		
		char name[ENTITY_NAME_SIZE + 1u];
		u64 tag;
		// TODO: Move to internal (cache friendly iterating components)
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

	// TODO: Use this insted of compute every time the matrices of a mirror
	struct EntityMirrorTransform {

		XMFLOAT4X4 world_matrix;
		
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

		EntityPrefab prefabs[256];
		ThickHashTable<u16, 200> prefab_table;
		u32 prefab_size = 0u;
		u32 prefab_free = 0u;

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

			CloseSceneEvent e;
			string_copy(e.name, scene->name, SCENE_NAME_SIZE + 1);

			event_dispatch("close_scene", &e);

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

		InitializeSceneEvent e;
		string_copy(e.name, scene.name, SCENE_NAME_SIZE + 1);
		
		event_dispatch("pre_initialize_scene", &e);

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
		    
				deserialize_u32(d, scene.data.main_camera);

				if (scene_version <= 3) {
					Entity e ;
					deserialize_u32(d, e);
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
		
		event_dispatch("initialize_scene", &e);
	
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

		serialize_u32(s, scene.data.main_camera);

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

	SV_AUX Entity get_origin_entity(Entity entity)
	{
		return entity & 0x00FFFFFF;
	}
	SV_AUX Entity get_reflected_entity(Entity entity)
	{
		SV_ECS();
		
		u32 index = (entity & 0xFF000000) >> 24;
		if (index == 0) return entity;

		--index;

		Entity origin = get_origin_entity(entity);

		if (entity_exists(origin)) {

			EntityInternal& internal = ecs.entity_internal[origin - 1];

			if (is_prefab(internal.prefab)) {

				EntityInternal& prefab = ecs.entity_internal[internal.prefab - 1];

				if (index < prefab.child_count) {
					return ecs.entity_hierarchy[prefab.hierarchy_index + 1 + index];
				}
				else SV_ASSERT(0);
			}
		}
		else SV_ASSERT(0);

		return 0;
	}

	SV_AUX void create_component(CompID comp_id, Component* ptr, Entity entity)
    {
		scene_state->component_register[comp_id].create_fn(ptr, entity);
		ptr->id = entity;
		if (is_prefab(entity)) ptr->id |= SV_BIT(31);
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

	SV_AUX EntityPrefab* find_prefab(u64 hash)
	{
		SV_ECS();

		u16* index = ecs.prefab_table.find(hash);
		if (index)
			return ecs.prefabs + *index;
		else return NULL;
	}

	SV_AUX EntityPrefab* allocate_prefab(const char* filepath)
	{
		SV_ECS();

		u64 hash = hash_string(filepath);

		EntityPrefab* prefab = find_prefab(hash);

		if (prefab != NULL) {
			SV_LOG_ERROR("Duplicated prefab '%s'", filepath);
			return NULL;
		}
		
		if (ecs.prefab_size == PREFAB_MAX && ecs.prefab_free == 0)
			return NULL;
		
		if (ecs.prefab_free) {

			u32 index = u32_max;

			foreach(i, ecs.prefab_size) {
				if (ecs.prefabs[i].entity == 0) {
					index = i;
					break;
				}
			}

			if (index != u32_max) {
				--ecs.prefab_free;
				prefab = ecs.prefabs + index;
			}
			else SV_ASSERT(0);
		}

		if (prefab == NULL)
			prefab = ecs.prefabs + ecs.prefab_size++;

		ecs.prefab_table[hash] = u16(prefab - ecs.prefabs);
		
		return prefab;
	}

	SV_AUX void free_prefab(EntityPrefab* prefab)
	{
		SV_ECS();

		ecs.prefab_table.erase(prefab->filepath);
		++ecs.prefab_free;
		
		prefab->entities.clear();
		prefab->entity = 0;
		prefab->filepath[0] = '\0';		
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

		foreach(i, ecs.prefab_size) {

			if (ecs.prefabs[i].entity) {
				ecs.prefabs[i].entities.clear();
			}
		}

		ecs.prefab_size = 0u;
		ecs.prefab_free = 0u;
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
		e.own_prefab = NULL;
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

	SV_AUX bool is_serializable(Entity entity)
	{
		SV_ECS();
		
		if (entity_exists(entity)) {

			const EntityMisc& misc = ecs.entity_misc[entity - 1u];
			return !(misc.flags & EntityFlag_NoSerialize) && !(misc.flags & EntityFlag_PrefabChild);
		}
		return false;
	}

	static u32 get_serializable_child_count(Entity entity)
	{
		SV_ECS();
		
		const EntityInternal& internal = ecs.entity_internal[entity - 1u];
		u32 count = 0u;

		foreach(i, internal.child_count) {

			Entity child = ecs.entity_hierarchy[internal.hierarchy_index + i + 1u];
			if (is_serializable(child)) {

				count += 1 + get_serializable_child_count(child);

				const EntityInternal& child_internal = ecs.entity_internal[child - 1u];
				i += child_internal.child_count;
			}
		}

		return count;
	}

	void serialize_ecs(Serializer& s)
	{
		SV_ECS();

		constexpr u32 VERSION = 0u;
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

		// Entity data
		{
			foreach(i, ecs.entity_hierarchy.size()) {

				Entity entity = ecs.entity_hierarchy[i];
				
				EntityInternal& internal = ecs.entity_internal[entity - 1u];
				EntityMisc& misc = ecs.entity_misc[entity - 1u];
				EntityTransform& transform = ecs.entity_transform[entity - 1u];

				if (is_serializable(entity)) {

					u8 type = internal.own_prefab ? 2 : 1;

					if (internal.parent == 0) {

						serialize_u8(s, type);
					}

					if (type == 1) {
						
						serialize_u64(s, internal.tag_mask);
						serialize_u32(s, get_serializable_child_count(entity));

						u64 prefab_hash = 0u;

						if (internal.prefab) {

							EntityPrefab* prefab = ecs.entity_internal[internal.prefab - 1u].own_prefab;
							if (prefab) {
								prefab_hash = hash_string(prefab->filepath);
							}
							else SV_ASSERT(0);
						}
					
						serialize_u64(s, prefab_hash);
					
						serialize_string(s, misc.name);
						serialize_u64(s, misc.flags);
					
						serialize_v3_f32(s, transform.position);
						serialize_v4_f32(s, transform.rotation);
						serialize_v3_f32(s, transform.scale);

						serialize_u32(s, internal.component_count);

						foreach(i, internal.component_count) {

							CompID comp_id = internal.components[i].comp_id;
							serialize_u32(s, comp_id);
							serialize_component(comp_id, internal.components[i].comp, s);
						}
					}
					else {

						save_prefab(entity);
						serialize_string(s, internal.own_prefab->filepath);
					}
				}
				else i += internal.child_count;
			}

			serialize_u8(s, 0);
		}
	}

	struct TempTagRegister {
		char name[COMPONENT_NAME_SIZE + 1u];
		u32 id;
	};

	struct TempComponentRegister {
		char name[COMPONENT_NAME_SIZE + 1u];
		u32 size;
		u32 version;
		CompID id;
	};

	static Entity deserialize_entity(Deserializer& d, Entity parent, const List<TempTagRegister>& tag_registers, const List<TempComponentRegister>& component_registers)
	{
		SV_ECS();
		
		u64 tag_mask;
		u32 child_count;
		u64 prefab_hash;
		char name[ENTITY_NAME_SIZE + 1];
		u64 flags;
		Transform transform;
			
		deserialize_u64(d, tag_mask);
		deserialize_u32(d, child_count);
		deserialize_u64(d, prefab_hash);
			
		deserialize_string(d, name, ENTITY_NAME_SIZE + 1u);
		deserialize_u64(d, flags);
			
		deserialize_v3_f32(d, transform.position);
		deserialize_v4_f32(d, transform.rotation);
		deserialize_v3_f32(d, transform.scale);

		Entity prefab_entity = 0;

		if (prefab_hash != 0) {
			
			EntityPrefab* prefab = find_prefab(prefab_hash);
			if (prefab) {

				prefab_entity = prefab->entity;
			}
		}

		Entity entity = create_entity(parent, name, prefab_entity);

		// Set tags
		if (tag_mask) {

			foreach(tag, tag_registers.size()) {

				const TempTagRegister& reg = tag_registers[tag];
					
				if (tag_mask & SV_BIT(reg.id)) {

					foreach(i, TAG_MAX) {

						if (string_equals(scene_state->tag_register[i].name, reg.name)) {
							add_entity_tag(entity, i);
						}
					}
				}
			}
		}

		// Set flags
		set_entity_flags(entity, flags);

		set_entity_transform(entity, transform);

		// Components
		{
			EntityInternal& internal = ecs.entity_internal[entity - 1];
			deserialize_u32(d, internal.component_count);

			foreach(i, internal.component_count) {

				CompID file_comp_id;
				deserialize_u32(d, file_comp_id);

				const TempComponentRegister& reg = component_registers[file_comp_id];
				CompID comp_id = reg.id;
				u32 version = reg.version;
				
				Component* comp = allocate_component(comp_id);
				create_component(comp_id, comp, entity);
				deserialize_component(comp_id, comp, d, version);

				CompRef& ref = internal.components[i];
				ref.comp_id = comp_id;
				ref.comp = comp;
						
				internal.component_mask |= (1ULL << (u64)comp_id);
			}
		}
		
		foreach(i, child_count) {

			Entity child = deserialize_entity(d, entity, tag_registers, component_registers);

			i += ecs.entity_internal[child - 1].child_count;
		}

		return entity;
	}

	bool deserialize_ecs(Deserializer& d)
	{
		u32 version;
		deserialize_u32(d, version);
		
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

			u32 tag_count = 0u;
			deserialize_u32(d, tag_count);

			foreach(i, tag_count) {

				TempTagRegister& reg = tag_registers.emplace_back();

				deserialize_u32(d, reg.id);
				deserialize_string(d, reg.name, TAG_NAME_SIZE + 1u);
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

		// Entities

		while(1) {

			u8 type;
			deserialize_u8(d, type);

			if (type == 0) break;

			if (type == 1)
				deserialize_entity(d, 0, tag_registers, component_registers);
			else {

				char filepath[FILEPATH_SIZE + 1];
				deserialize_string(d, filepath, FILEPATH_SIZE + 1);
				load_prefab(filepath);
			}
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

	Entity create_entity(Entity parent, const char* name, Entity prefab_entity)
	{
		SV_ECS();

		SV_ASSERT(parent == 0u || entity_exists(parent));

		EntityPrefab* prefab = NULL;
		if (entity_exists(prefab_entity)) {
			prefab = ecs.entity_internal[prefab_entity - 1u].own_prefab;

			if (prefab == NULL) {
				SV_LOG_ERROR("The entity isn't a prefab");
				return 0;
			}
		}

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

			if (is_prefab(parent) || ecs.entity_misc[parent - 1].flags & EntityFlag_PrefabChild) {
				ecs.entity_misc[entity - 1].flags |= EntityFlag_PrefabChild;
			}
		}
		else {

			internal.hierarchy_index = u32(ecs.entity_hierarchy.size());
			ecs.entity_hierarchy.push_back(entity);
		}

		if (name)
			string_copy(ecs.entity_misc[entity - 1u].name, name, ENTITY_NAME_SIZE + 1u);

		if (prefab) {

			internal.prefab = prefab_entity;
			prefab->entities.push_back(entity);

			const EntityTransform& prefab_transform = ecs.entity_transform[prefab_entity - 1];
			EntityTransform& transform = ecs.entity_transform[entity - 1];

			transform.position = prefab_transform.position;
			transform.rotation = prefab_transform.rotation;
			transform.scale = prefab_transform.scale;

			if (name == NULL) {

				const EntityMisc& prefab_misc = ecs.entity_misc[prefab_entity - 1];
				EntityMisc& misc = ecs.entity_misc[entity - 1];
				string_copy(misc.name, prefab_misc.name, ENTITY_NAME_SIZE + 1);
			}
		}

		EntityCreateEvent e;
		e.entity = entity;
		event_dispatch("on_entity_create", &e);

		return entity;
	}
	
	void destroy_entity(Entity entity)
	{
		entity = get_reflected_entity(entity);
		
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
			
			EntityPrefab* p = ecs.entity_internal[internal.prefab - 1u].own_prefab;
			u32 index = u32_max;

			if (p) {
				foreach(i, p->entities.size()) {
					if (p->entities[i] == entity) {
						index = i;
						break;
					}
				}
			}

			SV_ASSERT(index != u32_max);
			if (index != u32_max) {

				p->entities.erase(index);
			}
		}

		EntityPrefab* prefab = internal.own_prefab;
		if (prefab) {

			for (Entity e : prefab->entities) {

				if (entity_exists(e)) {

					EntityInternal& internal = ecs.entity_internal[e - 1];
					internal.prefab = 0;
				}
			}

			ecs.prefab_table.erase(prefab->filepath);
			internal.own_prefab = NULL;
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

	SV_INTERNAL Entity entity_duplicate_recursive(Entity duplicated, Entity parent, bool is_prefab_child)
    {
		SV_ECS();
	
		Entity copy;

		copy = create_entity(parent);

		EntityMisc& duplicated_misc = ecs.entity_misc[duplicated - 1u];
		EntityMisc& copy_misc = ecs.entity_misc[copy - 1u];
		strcpy(copy_misc.name, duplicated_misc.name);

		if (!is_prefab_child)
			copy_misc.flags = duplicated_misc.flags & ~(EntityFlag_PrefabChild);

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

			EntityPrefab* p = ecs.entity_internal[duplicated_internal.prefab - 1u].own_prefab;

			if (p) {
				copy_internal.prefab = duplicated_internal.prefab;
				p->entities.push_back(copy);
			}
		}

		foreach(i, duplicated_internal.component_count) {

			CompID comp_id = duplicated_internal.components[i].comp_id;
			Component* comp = duplicated_internal.components[i].comp;

			CompRef& ref = copy_internal.components[copy_internal.component_count++];
			ref.comp = allocate_component(comp_id);
			copy_component(comp_id, ref.comp, comp, copy);
			ref.comp_id = comp_id;
		}
		copy_internal.component_mask = duplicated_internal.component_mask;

		foreach(i, ecs.entity_internal[duplicated - 1u].child_count) {
			Entity to_copy = ecs.entity_hierarchy[ecs.entity_internal[duplicated - 1].hierarchy_index + i + 1];
			entity_duplicate_recursive(to_copy, copy, is_prefab_child);
			i += ecs.entity_internal[to_copy - 1u].child_count;
		}

		return copy;
    }

	Entity duplicate_entity(Entity entity)
	{
		SV_ECS();

		entity = get_reflected_entity(entity);

		if (entity_exists(entity)) {

			bool is_prefab_child = ecs.entity_misc[entity - 1].flags & EntityFlag_PrefabChild;
			
			return entity_duplicate_recursive(entity, ecs.entity_internal[entity - 1u].parent, is_prefab_child);
		}
		return 0;
	}

	bool entity_exists(Entity entity)
	{
		SV_ECS();
		if (entity) {
			
			entity = get_reflected_entity(entity);
			
			--entity;
			return ecs.entity_size > entity && ecs.entity_internal[entity].hierarchy_index != u32_max;
		}

		return false;
	}
	
	const char* get_entity_name(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);
		
		return ecs.entity_misc[entity - 1u].name;
	}
	
	u64 get_entity_flags(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);
		
		return ecs.entity_misc[entity - 1u].flags;
	}

	void set_entity_name(Entity entity, const char* name)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);
		
		string_copy(ecs.entity_misc[entity - 1u].name, name, ENTITY_NAME_SIZE + 1u);
	}
	
    u32 get_entity_childs_count(Entity parent)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(parent));

		parent = get_reflected_entity(parent);

		EntityInternal& internal = ecs.entity_internal[parent - 1u];
		
		return internal.child_count;
	}
	
    Entity get_entity_child(Entity parent, u32 index)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(parent));

		parent = get_reflected_entity(parent);
		
		const EntityInternal& internal = ecs.entity_internal[parent - 1u];

		if (index < internal.child_count) {
			return ecs.entity_hierarchy[internal.hierarchy_index + 1u + index];
		}	
		
		return 0;
	}
	
    Entity get_entity_parent(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_origin_entity(entity);
		
		return ecs.entity_internal[entity - 1u].parent;
	}
	
    u32 get_entity_component_count(Entity entity)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

		const EntityInternal& internal = ecs.entity_internal[entity - 1u];

		u32 count = internal.component_count;

		if (internal.prefab) {

			EntityInternal& p = ecs.entity_internal[internal.prefab - 1];
			count += p.component_count;
		}

		return count;
	}
	
	CompRef get_entity_component_by_index(Entity entity, u32 index)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

		const EntityInternal& internal = ecs.entity_internal[entity - 1u];

		if (index < internal.component_count) {

			return internal.components[index];
		}
		else if (internal.prefab) {

			EntityInternal& p = ecs.entity_internal[internal.prefab - 1u];
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

	void set_entity_flags(Entity entity, u64 flags)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));
		entity = get_reflected_entity(entity);
		EntityMisc& misc = ecs.entity_misc[entity - 1u];
		misc.flags = flags;
	}

	bool has_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);
		
		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		if (internal.component_mask & (1ULL << (u64)comp_id)) return true;

		else if (internal.prefab) {

			return has_entity_component(internal.prefab, comp_id);
		}

		return false;
	}
	
	Component* add_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

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
		
		create_component(comp_id, component, entity);

		internal.components[internal.component_count].comp_id = comp_id;
		internal.components[internal.component_count].comp = component;

		if (is_prefab(entity))
			component->id |= SV_BIT(31);
		
		++internal.component_count;

		internal.component_mask |= SV_BIT(comp_id);

		return component;
	}
	
	void remove_entity_component(Entity entity, CompID comp_id)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

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

		entity = get_reflected_entity(entity);

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
			return get_entity_component(internal.prefab, comp_id);
		}

		return NULL;
	}

	bool has_entity_tag(Entity entity, Tag tag)
	{
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		if (internal.tag_mask & SV_BIT(tag)) return true;

		if (is_prefab(internal.prefab)) {
			return has_entity_tag(internal.prefab, tag);
		}

		return false;
	}
	
	void add_entity_tag(Entity entity, Tag tag)
	{
		if (!tag_exists(tag)) return;
		
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

		if (entity_exists(entity)) {

			if (!has_entity_tag(entity, tag)) {
				
				EntityInternal& internal = ecs.entity_internal[entity - 1u];

				if (!(internal.tag_mask & SV_BIT(tag))) {

					internal.tag_mask |= SV_BIT(tag);

					TagInternal& tag_internal = ecs.tags[tag];
					tag_internal.entities.push_back(entity);
				}
			}
		}
	}
	
	void remove_entity_tag(Entity entity, Tag tag)
	{
		if (!tag_exists(tag)) return;
		
		SV_ECS();
		SV_ASSERT(entity_exists(entity));

		entity = get_reflected_entity(entity);

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

	SV_AUX bool deserialize_components(Deserializer& d, u32& component_count, u64& component_mask, CompRef* components, Entity entity)
	{
		u32 version;
		deserialize_u32(d, version);
		
		deserialize_u32(d, component_count);

		if (component_count > ENTITY_COMPONENTS_MAX) {
			SV_LOG_ERROR("Exceeds the component limit of %u", ENTITY_COMPONENTS_MAX);
			return false;
		}

		foreach(i, component_count) {

			CompRef& ref = components[i];

			char comp_name[COMPONENT_NAME_SIZE + 1u];
			u32 comp_version;

			deserialize_string(d, comp_name, COMPONENT_NAME_SIZE + 1u);
			deserialize_u32(d, comp_version);

			ref.comp_id = get_component_id(comp_name);

			if (ref.comp_id == INVALID_COMP_ID) {
				SV_LOG_ERROR("The component '%s' doesn't exists", comp_name);
				return false;
			}
				
			ref.comp = allocate_component(ref.comp_id);
			create_component(ref.comp_id, ref.comp, entity);
			
			deserialize_component(ref.comp_id, ref.comp, d, comp_version);

			component_mask |= SV_BIT(ref.comp_id);
			components[i] = ref;
		}

		return true;
	}

	SV_AUX void serialize_components(Serializer& s, u32 component_count, const CompRef* components)
	{
		serialize_u32(s, 0u); // VERSION
		
		serialize_u32(s, component_count);

		foreach(i, component_count) {

			CompRef ref = components[i];
			serialize_string(s, get_component_name(ref.comp_id));
			serialize_u32(s, get_component_version(ref.comp_id));
			serialize_component(ref.comp_id, ref.comp, s);
		}
	}

	static void serialize_entity_internal(Serializer& s, Entity entity)
	{
		SV_ECS();

		const EntityInternal& internal = ecs.entity_internal[entity - 1u];
		const EntityMisc& misc = ecs.entity_misc[entity - 1u];
		const EntityTransform& transform = ecs.entity_transform[entity - 1u];

		// Prefab
		if (internal.prefab) {

			EntityPrefab* prefab = ecs.entity_internal[internal.prefab - 1].own_prefab;
			SV_ASSERT(prefab);
			
			if (prefab)
				serialize_string(s, prefab->filepath);
			else serialize_string(s, "");
		}
		else serialize_string(s, "");
		
		// Tags
		if (internal.tag_mask) {

			u32 tag_count = 0u;

			foreach(i, TAG_MAX) {

				if (internal.tag_mask & SV_BIT(i)) {
					++tag_count;
				}
			}

			serialize_u32(s, tag_count);

			foreach(i, TAG_MAX) {

				if (internal.tag_mask & SV_BIT(i)) {

					const TagRegister& tag = scene_state->tag_register[i];
					serialize_string(s, tag.name);
				}
			}
		}
		else serialize_u32(s, 0u);
		
		serialize_u32(s, internal.child_count);
		serialize_string(s, misc.name);
		serialize_u64(s, misc.flags);
		serialize_v3_f32(s, transform.position);
		serialize_v4_f32(s, transform.rotation);
		serialize_v3_f32(s, transform.scale);

		// Components
		{
			serialize_components(s, internal.component_count, internal.components);
		}

		for (u32 i = 0u; i < internal.child_count; ++i) {

			Entity child_handle = ecs.entity_hierarchy[internal.hierarchy_index + 1 + i];
			serialize_entity_internal(s, child_handle);

			const EntityInternal& child = ecs.entity_internal[child_handle - 1u];
			i += child.child_count;
		}
	}

	static Entity deserialize_entity_prefab(Deserializer& s, Entity parent)
	{
		SV_ECS();

		// Prefab
		Entity prefab = 0;
		{
			char filepath[FILEPATH_SIZE + 1];
			deserialize_string(s, filepath, FILEPATH_SIZE + 1);

			if (filepath[0])
				prefab = load_prefab(filepath);
		}

		Entity entity = create_entity(parent, NULL, prefab);
		
		// Tags
		u32 tag_count;
		deserialize_u32(s, tag_count);

		foreach(i, tag_count) {

			char name[TAG_NAME_SIZE + 1];
			deserialize_string(s, name, TAG_NAME_SIZE + 1);

			Tag tag = get_tag_id(name);

			if (tag_exists(tag))
				add_entity_tag(entity, tag);
			else {
				SV_LOG_ERROR("Unknown tag '%s'", name);
			}
		}

		u32 child_count;
		deserialize_u32(s, child_count);

		char name[ENTITY_NAME_SIZE + 1];
		deserialize_string(s, name, ENTITY_NAME_SIZE + 1);
		set_entity_name(entity, name);

		u64 flags;
		deserialize_u64(s, flags);

		if (parent != 0)
			flags |= EntityFlag_PrefabChild;
		
		set_entity_flags(entity, flags);

		Transform transform;
		deserialize_v3_f32(s, transform.position);
		deserialize_v4_f32(s, transform.rotation);
		deserialize_v3_f32(s, transform.scale);

		set_entity_transform(entity, transform);

		// Components
		{
			EntityInternal& internal = ecs.entity_internal[entity - 1];
			deserialize_components(s, internal.component_count, internal.component_mask, internal.components, entity);

			if (parent == 0)
				foreach(i, internal.component_count)
					internal.components[i].comp->id |= SV_BIT(31);
		}

		for (u32 i = 0u; i < child_count; ++i) {

			Entity child_handle = deserialize_entity_prefab(s, entity);

			if (entity_exists(child_handle)) {

				const EntityInternal& child = ecs.entity_internal[child_handle - 1u];
				i += child.child_count;
			}
		}

		return entity;
	}
	
	Entity load_prefab(const char* filepath)
	{
		SV_ECS();

		u64 hash = hash_string(filepath);
		EntityPrefab* p = find_prefab(hash);

		if (p != NULL) {

			return p->entity;
		}

		Deserializer s;
		Entity prefab = 0;

		if (deserialize_begin(s, filepath)) {

			u32 version;
			deserialize_u32(s, version); // VERSION

			prefab = deserialize_entity_prefab(s, 0);

			deserialize_end(s);
		}
		else {
			SV_LOG_ERROR("Can't load the file '%s'", filepath);
		}

		if (prefab == 0) {
			SV_LOG_ERROR("Can't create the prefab '%s'", filepath);
			return 0;
		}

		EntityPrefab* entity_prefab = allocate_prefab(filepath);
		SV_ASSERT(entity_prefab);
		string_copy(entity_prefab->filepath, filepath, FILEPATH_SIZE + 1u);
		entity_prefab->entity = prefab;

		ecs.entity_internal[prefab - 1].own_prefab = entity_prefab;
		
		return prefab;
	}

	bool save_prefab(Entity prefab)
	{
		SV_ECS();
		if (entity_exists(prefab)) {

			EntityPrefab* p = ecs.entity_internal[prefab - 1].own_prefab;
			if (p)
				return save_prefab(prefab, p->filepath);
			else {
				SV_LOG_ERROR("The entity is not a prefab");
			}
		}
		else SV_LOG_ERROR("The entity doesn't exists");

		return false;
	}
	
	bool save_prefab(Entity prefab, const char* filepath)
	{
		if (!entity_exists(prefab)) {
			SV_LOG_ERROR("Can't save the prefab in file '%s', the entity doesn't exists", filepath);
			return false;
		}

		prefab = get_reflected_entity(prefab);

		Serializer s;

		serialize_begin(s);

		serialize_u32(s, 0); // VERSION

		serialize_entity_internal(s, prefab);

		if (serialize_end(s, filepath)) {

			SV_LOG_INFO("Prefab saved in '%s'", filepath);
		}
		else {

			SV_LOG_ERROR("Can't save the prefab in file '%s'", filepath);
			return false;
		}

		return true;
	}

	bool set_prefab(Entity entity, const char* filepath)
	{
		SV_ECS();
		
		if (entity_exists(entity)) {

			EntityInternal& internal = ecs.entity_internal[entity - 1];
			if (internal.parent) {
				SV_LOG_ERROR("A prefab can't have parents");
				return false;
			}

			EntityPrefab* prefab = allocate_prefab(filepath);
			if (prefab == NULL) return false;
			
			string_copy(prefab->filepath, filepath, FILEPATH_SIZE + 1);
			prefab->entity = entity;
			
			internal.own_prefab = prefab;
						
			foreach(i, internal.component_count) {

				Component* comp = internal.components[i].comp;
				comp->id |= SV_BIT(31);
			}
		}
		else {
			SV_LOG_ERROR("The entity doesn't exists");
			return false;
		}

		return true;
	}

	const char* get_prefab_filepath(Entity prefab)
	{
		SV_ECS();
		
		if (entity_exists(prefab)) {

			EntityPrefab* p = ecs.entity_internal[prefab - 1].own_prefab;
			
			return p ? p->filepath : NULL;
		}
		return NULL;
	}

	bool is_prefab(Entity entity)
	{
		SV_ECS();
		
		if (entity_exists(entity)) {

			return ecs.entity_internal[entity - 1].own_prefab != NULL;
		}
		return false;
	}

	bool is_mirror(Entity entity)
	{
		return entity & 0xFF000000;
	}

	Entity get_entity_mirror(Entity entity, u32 index)
	{
		SV_ECS();
		
		if (entity_exists(entity)) {

			EntityInternal& internal = ecs.entity_internal[entity - 1];

			if (is_prefab(internal.prefab)) {

				EntityInternal& prefab_internal = ecs.entity_internal[internal.prefab - 1];

				if (index < prefab_internal.child_count) {

					return entity | ((index + 1) << 24);
				}
				else SV_ASSERT(0);
			}
			else SV_ASSERT(0);
		}
		else SV_ASSERT(0);

		return 0;
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
		it.prefab_index = 0;

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

	SV_AUX bool comp_it_init_prefab(CompIt& it, Entity prefab, u32 index, Component* c)
	{
		SV_ECS();
		SV_ASSERT(is_prefab(prefab));

		List<Entity>& entities = ecs.entity_internal[prefab - 1u].own_prefab->entities;

		if (it.flags & CompItFlag_Once) {

			it.comp = c;
			it.entity = 0;
			it.entity_index = 0u;
			it.prefab = prefab;
			it.prefab_index = index;
			return true;
		}
		else if (entities.size()) {

			it.comp = c;
			it.entity = entities.front() | (index << 24);
			it.entity_index = 0u;
			it.prefab = prefab;
			it.prefab_index = index;
			return true;
		}

		return false;
	}
	
	void comp_it_next(CompIt& it)
	{
		SV_ECS();

		bool next_component = true;

		if (it.prefab) {

			SV_ASSERT(is_prefab(it.prefab));
			EntityInternal& internal = ecs.entity_internal[it.prefab - 1u];
			EntityPrefab& prefab = *internal.own_prefab;
			
			if (++it.entity_index < prefab.entities.size() && !(it.flags & CompItFlag_Once)) {

				next_component = false;
				it.entity = prefab.entities[it.entity_index] | (it.prefab_index << 24);
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
						
					Entity prefab = c->id & ~SV_BIT(31);
					if (comp_it_init_prefab(it, prefab, 0, c))
						break;
				}
				else {

					if (ecs.entity_misc[c->id - 1].flags & EntityFlag_PrefabChild) {
						
						EntityInternal& entity_internal = ecs.entity_internal[c->id - 1];

						Entity parent = entity_internal.parent;
						Entity prefab = c->id;

						while (parent != 0) {

							EntityInternal& internal = ecs.entity_internal[parent - 1];
							prefab = parent;
							parent = internal.parent;
						}

						EntityInternal& prefab_internal = ecs.entity_internal[prefab - 1];
						SV_ASSERT(prefab_internal.own_prefab);
						
						u32 child_index = entity_internal.hierarchy_index - prefab_internal.hierarchy_index;
						if (comp_it_init_prefab(it, prefab, child_index, c))
							break;
					}
					else {
						it.comp = c;
						it.entity = c->id;
						break;
					}
				}
			}
			while(1);
		}
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
				it.prefab = 0;
			}
			else it.has_next = false;
		}
		else it.has_next = false;
		return it;
	}
	
	void tag_it_next(TagIt& it)
	{
		SV_ECS();

		TagInternal& internal = ecs.tags[it.tag];

		if (it.prefab) {

			EntityPrefab& prefab = *ecs.entity_internal[it.prefab - 1].own_prefab;
			if (it._prefab_index < prefab.entities.size()) {

				it.entity = prefab.entities[it._prefab_index++];
				return;
			}
			else {
				it.prefab = 0;
			}
		}

		++it._index;
		
		if (it._index >= internal.entities.size())
			it.has_next = false;
		else {
			it.entity = internal.entities[it._index];
			if (is_prefab(it.entity)) {

				it.prefab = it.entity;
				it.entity = 0;
				it._prefab_index = 0;
				tag_it_next(it);
			}
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
		register_component<TexturedSpriteComponent>("Textured Sprite");
		register_component<AnimatedSpriteComponent>("Animated Sprite");
		register_component<CameraComponent>("Camera");
		register_component<MeshComponent>("Mesh");
		register_component<TerrainComponent>("Terrain");
		register_component<ParticleSystem>("Particle System");
		register_component<ParticleSystemModel>("Particle System Model");
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

	SV_AUX XMMATRIX compute_world_matrix(Entity entity, Entity parent)
	{
		XMMATRIX m = get_entity_matrix(entity);

		if (parent != 0) {
	    
			XMMATRIX mp = get_entity_world_matrix(parent);
			m = m * mp;
		}

		return m;
	}

	SV_AUX void update_world_matrix(EntityTransform& t, Entity entity)
    {
		SV_ECS();
	
		t.dirty = false;

		EntityInternal& internal = ecs.entity_internal[entity - 1u];
		Entity parent = internal.parent;

		XMMATRIX m = compute_world_matrix(entity, parent);
	
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
		entity = get_reflected_entity(entity);

		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position = transform.position;
		t.rotation = transform.rotation;
		t.scale = transform.scale;
		notify_transform(t, entity);
    }
    
    void set_entity_position(Entity entity, const v3_f32& position)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position = position;
		notify_transform(t, entity);
    }
    
    void set_entity_rotation(Entity entity, const v4_f32& rotation)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.rotation = rotation;
		notify_transform(t, entity);
    }
    
    void set_entity_scale(Entity entity, const v3_f32& scale)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.scale = scale;
		notify_transform(t, entity);
    }
    
    void set_entity_matrix(Entity entity, const XMMATRIX& matrix)
    {
		entity = get_reflected_entity(entity);
		
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
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.position.x = position.x;
		t.position.y = position.y;
		notify_transform(t, entity);
    }

	void set_entity_scale2D(Entity entity, const v2_f32& scale)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		t.scale.x = scale.x;
		t.scale.y = scale.y;
		notify_transform(t, entity);
    }

	void set_entity_euler_rotation(Entity entity, v3_f32 euler_angles)
	{
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];

		t.rotation = XMQuaternionRotationRollPitchYaw(euler_angles.x, euler_angles.y, euler_angles.z);
		notify_transform(t, entity);
	}
	
	void set_entity_euler_rotationX(Entity entity, f32 rotation)
	{
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		//TODO t.rotation = XMQuaternionRotationRollPitchYaw(euler_angles.x, euler_angles.y, euler_angles.z);
		notify_transform(t, entity);
	}
	
	void set_entity_euler_rotationY(Entity entity, f32 rotation)
	{
		entity = get_reflected_entity(entity);
	}
	
	void set_entity_euler_rotationZ(Entity entity, f32 rotation)
	{
		entity = get_reflected_entity(entity);
	}
    
    Transform* get_entity_transform_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<Transform*>(&t.position);
    }
    
    v3_f32* get_entity_position_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.position;
    }
    
    v4_f32* get_entity_rotation_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.rotation;
    }
    
    v3_f32* get_entity_scale_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return &t.scale;
    }
    
    v2_f32* get_entity_position2D_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<v2_f32*>(&t.position);
    }
    
    v2_f32* get_entity_scale2D_ptr(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		notify_transform(t, entity);

		return reinterpret_cast<v2_f32*>(&t.scale);
    }
    
    Transform get_entity_transform(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
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
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.position;
    }
    
    v4_f32 get_entity_rotation(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.rotation;
    }
    
    v3_f32 get_entity_scale(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return t.scale;
    }
    
    v2_f32 get_entity_position2D(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return vec3_to_vec2(t.position);
    }
    
    v2_f32 get_entity_scale2D(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		return vec3_to_vec2(t.scale);
    }

    XMMATRIX get_entity_matrix(Entity entity)
    {
		entity = get_reflected_entity(entity);
		
		SV_ECS();
		EntityTransform& t = ecs.entity_transform[entity - 1u];
		
		return XMMatrixScalingFromVector(vec3_to_dx(t.scale)) * XMMatrixRotationQuaternion(vec4_to_dx(t.rotation))
			* XMMatrixTranslation(t.position.x, t.position.y, t.position.z);
    }

	v3_f32 get_entity_euler_rotation(Entity entity)
	{
		entity = get_reflected_entity(entity);
		
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
		entity = get_reflected_entity(entity);
		
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
		entity = get_reflected_entity(entity);
		
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
		entity = get_reflected_entity(entity);
		
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

	SV_AUX XMFLOAT4X4 get_entity_world_matrix_internal(Entity entity)
	{
		SV_ECS();
		XMFLOAT4X4 world_matrix;
		
		if (is_mirror(entity)) {

			Entity child = get_reflected_entity(entity);
			Entity parent = get_entity_parent(child);

			if (is_prefab(parent)) parent = get_origin_entity(entity);
			else {
			
				const EntityInternal& parent_internal = ecs.entity_internal[parent - 1];

				Entity prefab = 0;

				while (parent != 0) {

					prefab = parent;
					const EntityInternal& internal = ecs.entity_internal[parent - 1];
					parent = internal.parent;
				}

				const EntityInternal& prefab_internal = ecs.entity_internal[prefab - 1];

				u32 index = parent_internal.hierarchy_index - prefab_internal.hierarchy_index;

				parent = get_origin_entity(entity) | (index << 24);
			}
			
			XMMATRIX m = compute_world_matrix(child, parent);
			XMStoreFloat4x4(&world_matrix, m);
		}
		else {
			EntityTransform& t = ecs.entity_transform[entity - 1u];

			if (t.dirty) update_world_matrix(t, entity);
			world_matrix = t.world_matrix;
		}

		return world_matrix;
	}

    Transform get_entity_world_transform(Entity entity)
    {
		XMFLOAT4X4 world_matrix = get_entity_world_matrix_internal(entity);

		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;
		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&world_matrix));

		Transform trans;
		trans.position = v3_f32(position);
		trans.rotation = v4_f32(rotation);
		trans.scale = v3_f32(scale);

		return trans;
    }
    
    v3_f32 get_entity_world_position(Entity entity)
    {
		XMFLOAT4X4 world_matrix = get_entity_world_matrix_internal(entity);
		return *(v3_f32*) &world_matrix._41;
    }
    
    v4_f32 get_entity_world_rotation(Entity entity)
    {
		XMFLOAT4X4 world_matrix = get_entity_world_matrix_internal(entity);
		
		XMVECTOR scale;
		XMVECTOR rotation;
		XMVECTOR position;

		XMMatrixDecompose(&scale, &rotation, &position, XMLoadFloat4x4(&world_matrix));

		return v4_f32(rotation);
    }
    
    v3_f32 get_entity_world_scale(Entity entity)
    {
		XMFLOAT4X4 world_matrix = get_entity_world_matrix_internal(entity);
		
		return { vec3_length(*(v3_f32*)& world_matrix._11), vec3_length(*(v3_f32*)& world_matrix._21), vec3_length(*(v3_f32*)& world_matrix._31) };
    }
    
    XMMATRIX get_entity_world_matrix(Entity entity)
    {
		XMFLOAT4X4 world_matrix = get_entity_world_matrix_internal(entity);
		return XMLoadFloat4x4(&world_matrix);
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
		serialize_color(s, emissive_color);
		serialize_u32(s, layer);
    }

    void SpriteComponent::deserialize(Deserializer& d, u32 version)
    {
		switch (version) {

		case 0:
		{// TODO: Deprecated
			TextureAsset tex;
			v4_f32 texcoord;
			deserialize_asset(d, tex);
			deserialize_v4_f32(d, texcoord);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
		break;

		case 1:
		{
			deserialize_asset(d, sprite_sheet);
			deserialize_u32(d, sprite_id);
			deserialize_color(d, color);
			deserialize_u32(d, layer);
		}
		break;

		case 2:
		{
			deserialize_asset(d, sprite_sheet);
			deserialize_u32(d, sprite_id);
			deserialize_color(d, color);
			deserialize_color(d, emissive_color);
			deserialize_u32(d, layer);
		}
		break;
			
		}
    }

	void TexturedSpriteComponent::serialize(Serializer& s)
    {
		serialize_asset(s, texture);
		serialize_v4_f32(s, texcoord);
		serialize_color(s, color);
		serialize_u32(s, layer);
    }

    void TexturedSpriteComponent::deserialize(Deserializer& d, u32 version)
    {
		deserialize_asset(d, texture);
		deserialize_v4_f32(d, texcoord);
		deserialize_color(d, color);
		deserialize_u32(d, layer);
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
