#pragma once

#include "platform/graphics.h"
#include "core/mesh.h"

namespace sv {

	constexpr u32 SCENE_NAME_SIZE = 50u;

	constexpr u32 COMPONENT_MAX = 64u;
	constexpr u32 TAG_MAX = 64u;
	
    constexpr u32 ENTITY_NAME_SIZE = 30u;
	constexpr u32 ENTITY_COMPONENTS_MAX = 10u;
    constexpr u32 COMPONENT_NAME_SIZE = 30u;

	constexpr u32 TAG_NAME_SIZE = 30u;

	constexpr u32 INVALID_COMP_ID = u32_max;
	constexpr u32 TAG_INVALID = u32_max;

    typedef u32 CompID;
    typedef u32 Entity;
	typedef u32 Prefab;
	typedef u32 Tag;

    struct CameraComponent;

    struct SceneData {

		void* user_ptr = nullptr;
		u32 user_id = 0u;
	
		Entity main_camera = 0;

		struct{
			// This asset can't be attached to a image file because it has a different initialization
			TextureAsset image;
			char filepath[FILEPATH_SIZE + 1u] = "";
		} skybox;
		
		Color ambient_light = Color::Gray(20u);

		struct {
			v3_f32 gravity = { 0.f, -36.f, 0.f };
			f32 air_friction = 0.02f;
			bool in_3D = false;
		} physics;
	
    };

    SV_API SceneData* get_scene_data();

    SV_API bool set_scene(const char* name);
    SV_API bool save_scene();
    SV_API bool save_scene(const char* filepath);
    SV_API bool clear_scene();

    SV_API const char* get_scene_name();
    SV_API bool there_is_scene();

    bool _scene_initialize();
    void _manage_scenes();
    void _scene_close();
    void _update_scene();
    void _draw_scene();
    bool _start_scene(const char* name);

    SV_API bool create_entity_model(Entity parent, const char* filepath);

    SV_API CameraComponent* get_main_camera();

    SV_API Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera);

	SV_API void free_skybox();
	SV_API bool set_skybox(const char* filepath);
	
    ////////////////////////////////////////// ECS ////////////////////////////////////////////////////////
	
	struct Component {
		u32 id;
		u32 flags;
	};

	struct CompRef {
		CompID comp_id;
		Component* comp;
	};

	// Entity

	SV_API Entity create_entity(Entity parent = 0, const char* name = NULL, Prefab prefab = 0);
	SV_API void   destroy_entity(Entity entity);
	SV_API Entity duplicate_entity(Entity entity);

	SV_API bool        entity_exists(Entity entity);
	SV_API const char* get_entity_name(Entity entity);
	SV_API u64         get_entity_flags(Entity entity);
	SV_API void	       set_entity_name(Entity entity, const char* name);
    SV_API u32	       get_entity_childs_count(Entity parent);
    SV_API void	       get_entity_childs(Entity parent, Entity const** childsArray);
    SV_API Entity      get_entity_parent(Entity entity);
    SV_API u32	       get_entity_component_count(Entity entity);
	SV_API CompRef     get_entity_component_by_index(Entity entity, u32 index);
    SV_API u32	       get_entity_count();
    SV_API Entity      get_entity_by_index(u32 index);

	SV_API bool       has_entity_component(Entity entity, CompID comp_id);
	SV_API Component* add_entity_component(Entity entity, CompID comp_id);
	SV_API void       remove_entity_component(Entity entity, CompID comp_id);
	SV_API Component* get_entity_component(Entity entity, CompID comp_id);

	SV_API bool has_entity_tag(Entity entity, Tag tag);
	SV_API void add_entity_tag(Entity entity, Tag tag);
	SV_API void remove_entity_tag(Entity entity, Tag tag);

	// Prefab

	SV_API bool   create_prefab_file(const char* name, const char* filepath);
	SV_API Prefab load_prefab(const char* filepath);
	SV_API bool   save_prefab(Prefab prefab);
	SV_API bool   save_prefab(Prefab prefab, const char* filepath);

	SV_API bool        prefab_exists(Prefab prefab);
	SV_API const char* get_prefab_name(Prefab prefab);
	
	SV_API bool       has_prefab_component(Prefab prefab, CompID comp_id);
	SV_API Component* add_prefab_component(Prefab prefab, CompID comp_id);
	SV_API void       remove_prefab_component(Prefab prefab, CompID comp_id);
	SV_API Component* get_prefab_component(Prefab prefab, CompID comp_id);
	SV_API u32	      get_prefab_component_count(Prefab prefab);
	SV_API CompRef    get_prefab_component_by_index(Prefab prefab, u32 index);

	// Iterators
	
	struct CompIt {
		u32 flags;
		Prefab prefab;
		u32 entity_index;
		CompID comp_id;
		u32 pool_index;
		Component* comp;
		Entity entity;
		bool has_next;
	};

	enum CompItFlag : u32 {
		CompItFlag_Once = SV_BIT(0),
	};
	
	SV_API CompIt comp_it_begin(CompID comp_id, u32 flags = 0u);
	SV_API void comp_it_next(CompIt& comp_it);

	struct PrefabIt {
		void* ptr;
		bool has_next;
		Prefab prefab;
	};

	SV_API PrefabIt prefab_it_begin();
	SV_API void prefab_it_next(PrefabIt& prefab_it);

	struct TagIt {
		Tag tag;
		u32 _index;
		bool has_next;
		Entity entity;
	};
	
	SV_API TagIt tag_it_begin(Tag tag);
	SV_API void tag_it_next(TagIt& tag_it);

	// Tags

	SV_API Tag         get_tag_id(const char* name);
	SV_API const char* get_tag_name(Tag tag);
	SV_API bool        tag_exists(Tag tag);
	SV_API Entity      get_tag_entity(Tag tag);
	
	// Components

	void register_components();
	void unregister_components();

    SV_API const char* get_component_name(CompID comp_id);
    SV_API u32		   get_component_size(CompID comp_id);
    SV_API u32		   get_component_version(CompID comp_id);
    SV_API CompID	   get_component_id(const char* name);
    SV_API u32		   get_component_register_count();
	SV_API u32         get_component_count(CompID comp_id);

    SV_API bool	component_exists(CompID comp_id);

    struct Transform {

		v3_f32 position = { 0.f, 0.f, 0.f };
		v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
		v3_f32 scale = { 1.f, 1.f, 1.f };

    };

    // Local space setters

    SV_API void set_entity_transform(Entity entity, const Transform& transform);
    SV_API void set_entity_position(Entity entity, const v3_f32& position);
    SV_API void set_entity_rotation(Entity entity, const v4_f32& rotation);
    SV_API void set_entity_scale(Entity entity, const v3_f32& scale);
    SV_API void set_entity_matrix(Entity entity, const XMMATRIX& matrix);

    SV_API void set_entity_position2D(Entity entity, const v2_f32& position);
    SV_API void set_entity_scale2D(Entity entity, const v2_f32& scale);

    // Local space getters
    
    SV_API Transform* get_entity_transform_ptr(Entity entity);
    SV_API v3_f32*    get_entity_position_ptr(Entity entity);
    SV_API v4_f32*    get_entity_rotation_ptr(Entity entity);
    SV_API v3_f32*    get_entity_scale_ptr(Entity entity);
    SV_API v2_f32*    get_entity_position2D_ptr(Entity entity);
    SV_API v2_f32*    get_entity_scale2D_ptr(Entity entity);
    SV_API XMMATRIX   get_entity_matrix(Entity entity);

    // Local space const getters
    
    SV_API Transform get_entity_transform(Entity entity);
    SV_API v3_f32 get_entity_position(Entity entity);
    SV_API v4_f32 get_entity_rotation(Entity entity);
    SV_API v3_f32 get_entity_scale(Entity entity);
    SV_API v2_f32 get_entity_position2D(Entity entity);
    SV_API v2_f32 get_entity_scale2D(Entity entity);

    // World space getters

    SV_API Transform get_entity_world_transform(Entity entity);
    SV_API v3_f32 get_entity_world_position(Entity entity);
    SV_API v4_f32 get_entity_world_rotation(Entity entity);
    SV_API v3_f32 get_entity_world_scale(Entity entity);
    SV_API XMMATRIX get_entity_world_matrix(Entity entity);

    ///////////////////////////////////////////////////////// COMPONENTS /////////////////////////////////////////////////////////
    
    constexpr u32 SPRITE_NAME_SIZE = 15u;
    constexpr u32 SPRITE_ANIMATION_MAX_FRAMES = 32u;
    
    struct Sprite {
		char name[SPRITE_NAME_SIZE + 1u];
		v4_f32 texcoord;
    };

    struct SpriteAnimation {
		char name[SPRITE_NAME_SIZE + 1u];
		u32 sprites[SPRITE_ANIMATION_MAX_FRAMES];
		u32 frames;
		f32 frame_time;
    };

	// TODO: Cache friendly
    struct SpriteSheet {

		static constexpr u32 VERSION = 0u;

		TextureAsset texture;
		IndexedList<Sprite> sprites;
		IndexedList<SpriteAnimation> sprite_animations;

		bool add_sprite(u32* id, const char* name, const v4_f32& texcoord);
		bool modify_sprite(u32 id, const char* name, const v4_f32& texcoord);
		
		bool add_sprite_animation(u32* id, const char* name, u32* sprites, u32 frames, f32 frame_time);
	
		v4_f32 get_sprite_texcoord(u32 id);
	
    };

    SV_API void serialize_sprite_sheet(Serializer& s, const SpriteSheet& sheet);
    SV_API void deserialize_sprite_sheet(Deserializer& d, SpriteSheet& sheet);

    SV_DEFINE_ASSET(SpriteSheetAsset, SpriteSheet);

    enum SpriteComponentFlag : u32 {
		SpriteComponentFlag_XFlip = SV_BIT(0), // Reverse the sprite coordinates in the x axis
		SpriteComponentFlag_YFlip = SV_BIT(1), // Reverse the sprite coordinates in the y axis
    };

    struct SpriteComponent : public Component {

		static constexpr u32 VERSION = 1u;

		SpriteSheetAsset sprite_sheet;
		u32              sprite_id = 0u;
		Color	         color = Color::White();
		u32	             layer = 0u;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

    };

    struct AnimatedSpriteComponent : public Component {

		static constexpr u32 VERSION = 1u;
	
		SpriteSheetAsset sprite_sheet;
		u32              animation_id = 0u;
		u32              index = 0u;
		f32              time_mult = 1.f;
		f32              simulation_time = 0.f;
		
	
		Color	     color = Color::White();
		u32	         layer = 0u;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

    };

    enum ProjectionType : u32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective,
    };

    struct CameraComponent : public Component {

		static constexpr u32 VERSION = 1u;

		ProjectionType projection_type = ProjectionType_Orthographic;
		f32 near = -1000.f;
		f32 far = 1000.f;
		f32 width = 10.f;
		f32 height = 10.f;

		XMMATRIX view_matrix;
		XMMATRIX projection_matrix;
		XMMATRIX view_projection_matrix;
		XMMATRIX inverse_view_matrix;
		XMMATRIX inverse_projection_matrix;
		XMMATRIX inverse_view_projection_matrix;

		struct {
			bool active = false;
			f32 threshold = 0.6f;
			f32 intensity = 0.03f;
		} bloom;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

		SV_INLINE void adjust(f32 aspect)
			{
				if (width / height == aspect) return;
				v2_f32 res = { width, height };
				f32 mag = vec2_length(res);
				res.x = aspect;
				res.y = 1.f;
				res = vec2_normalize(res);
				res *= mag;
				width = res.x;
				height = res.y;
			}

		SV_INLINE f32 getProjectionLength()
			{
				return math_sqrt(width * width + height * height);
			}

		SV_INLINE void setProjectionLength(f32 length)
			{
				f32 currentLength = getProjectionLength();
				if (currentLength == length) return;
				width = width / currentLength * length;
				height = height / currentLength * length;
			}

    };

    struct MeshComponent : public Component {

		static constexpr u32 VERSION = 0u;
	
		MeshAsset		mesh;
		MaterialAsset	material;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

    };

    enum LightType : u32 {
		LightType_Point,
		LightType_Direction,
		LightType_Spot,
    };

    struct LightComponent : public Component {

		static constexpr u32 VERSION = 3u;
	
		LightType light_type = LightType_Point;
		Color color = Color::White();
		f32 intensity = 1.f;
		f32 range = 5.f;
		f32 smoothness = 0.5f;

		bool shadow_mapping_enabled = false;
		f32 cascade_distance[3u] = { 20.f, 100.f, 500.f };
		f32 shadow_bias = 2.f;

		void serialize(Serializer& s);
		void deserialize(Deserializer& s, u32 version);

    };

    // EVENTS

    struct EntityCreateEvent {
		Entity entity;
    };

    struct EntityDestroyEvent {
		Entity entity;
    };
    
}

#if SV_EDITOR
#define __TAG(name) sv::get_tag_id(#name)
#endif
