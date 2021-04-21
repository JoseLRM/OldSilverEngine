#pragma once

#include "platform/graphics.h"
#include "core/mesh.h"
#include "core/gui.h"

#define SV_ENTITY_NULL 0u
#define SV_COMPONENT_ID_INVALID std::numeric_limits<sv::CompID>::max()

namespace sv {

    typedef u16 CompID;
    typedef u32 Entity;

    struct CameraComponent;

    struct SceneData {

	GUI* gui = nullptr;
	void* user_ptr = nullptr;
	u32 user_id = 0u;
	Entity main_camera = SV_ENTITY_NULL;

	GPUImage* skybox = nullptr;
	Color ambient_light = Color::Gray(20u);

	v2_f32 gravity = { 0.f, -36.f };
	f32 air_friction = 0.02f;
	
    };

    SV_API SceneData* get_scene_data();

    SV_API bool set_scene(const char* name);
    SV_API bool save_scene();
    SV_API bool save_scene(const char* filepath);
    SV_API bool clear_scene();

    SV_API const char* get_scene_name();
    SV_API bool there_is_scene();

    void _initialize_scene();
    void _manage_scenes();
    void _close_scene();
    void _update_scene();
    void _draw_scene();
    bool _start_scene(const char* name);

    SV_API bool create_entity_model(Entity parent, const char* filepath);

    SV_API CameraComponent* get_main_camera();

    SV_API Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera);
	
    ////////////////////////////////////////// ECS ////////////////////////////////////////////////////////

    enum EntitySystemFlag : u64 {
	EntitySystemFlag_None = 0u,
	EntitySystemFlag_NoSerialize = SV_BIT(32)
    };

    struct BaseComponent {
	u32 _id = 0u;
    };

    struct CompRef {
	CompID id;
	BaseComponent* ptr;
    };

    typedef void(*CreateComponentFunction)(BaseComponent*);
    typedef void(*DestroyComponentFunction)(BaseComponent*);
    typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
    typedef void(*CopyComponentFunction)(const BaseComponent* from, BaseComponent* to);
    typedef void(*SerializeComponentFunction)(BaseComponent* comp, Archive&);
    typedef void(*DeserializeComponentFunction)(BaseComponent* comp, u32 version, Archive&);

    struct ComponentRegisterDesc {

	const char*                   name;
	u32			      componentSize;
	u32                           version;
	CreateComponentFunction	      createFn;
	DestroyComponentFunction      destroyFn;
	MoveComponentFunction	      moveFn;
	CopyComponentFunction	      copyFn;
	SerializeComponentFunction    serializeFn;
	DeserializeComponentFunction  deserializeFn;

    };

    // Component Register

    SV_API CompID register_component(const ComponentRegisterDesc* desc);
    SV_API void   invalidate_component_callbacks(CompID id);

    SV_API const char*  get_component_name(CompID ID);
    SV_API u32		get_component_size(CompID ID);
    SV_API u32		get_component_version(CompID ID);
    SV_API CompID	get_component_id(const char* name);
    SV_API u32		get_component_register_count();

    SV_API void		create_component(CompID ID, BaseComponent* ptr, Entity entity);
    SV_API void		destroy_component(CompID ID, BaseComponent* ptr);
    SV_API void		move_component(CompID ID, BaseComponent* from, BaseComponent* to);
    SV_API void		copy_component(CompID ID, const BaseComponent* from, BaseComponent* to);
    SV_API void		serialize_component(CompID ID, BaseComponent* comp, Archive& archive);
    SV_API void		deserialize_component(CompID ID, BaseComponent* comp, u32 version, Archive& archive);
    SV_API bool		component_exist(CompID ID);

    struct Transform {

	// Local space
	
	v3_f32 position = { 0.f, 0.f, 0.f };
	v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
	v3_f32 scale = { 1.f, 1.f, 1.f };

	// World space
	
	XMFLOAT4X4 world_matrix;

	bool _modified = true;

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
    
    SV_API const Transform& get_entity_transform(Entity entity);
    SV_API v3_f32 get_entity_position(Entity entity);
    SV_API v4_f32 get_entity_rotation(Entity entity);
    SV_API v3_f32 get_entity_scale(Entity entity);
    SV_API v2_f32 get_entity_position2D(Entity entity);
    SV_API v2_f32 get_entity_scale2D(Entity entity);

    // World space getters
    
    SV_API v3_f32 get_entity_world_position(Entity entity);
    SV_API v4_f32 get_entity_world_rotation(Entity entity);
    SV_API v3_f32 get_entity_world_scale(Entity entity);
    SV_API XMMATRIX get_entity_world_matrix(Entity entity);

    // Entity

    SV_API Entity       create_entity(Entity parent = SV_ENTITY_NULL, const char* name = nullptr);
    SV_API void	        destroy_entity(Entity entity);
    SV_API void	        clear_entity(Entity entity);
    SV_API Entity       duplicate_entity(Entity entity);
    SV_API bool	        entity_is_empty(Entity entity);
    SV_API bool	        entity_exist(Entity entity);
    SV_API const char*  get_entity_name(Entity entity);
    SV_API void	        set_entity_name(Entity entity, const char* name);
    SV_API u32	        get_entity_childs_count(Entity parent);
    SV_API void		get_entity_childs(Entity parent, Entity const** childsArray);
    SV_API Entity	get_entity_parent(Entity entity);
    SV_API u64*		get_entity_flags(Entity entity);
    SV_API u32		get_entity_component_count(Entity entity);
    SV_API u32		get_entity_count();
    SV_API Entity	get_entity_by_index(u32 index);

    // Components

    SV_API BaseComponent* add_component(Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
    SV_API BaseComponent* add_component_by_id(Entity entity, CompID componentID);
    SV_API BaseComponent* get_component_by_id(Entity entity, CompID componentID);
    SV_API CompRef        get_component_by_index(Entity entity, u32 index);
    SV_API void		  remove_component_by_id(Entity entity, CompID componentID);
    SV_API u32		  get_component_count(CompID ID);

    // Iterators

    struct SV_API ComponentIterator {
		
	BaseComponent* _it;
	u32 _pool;

	CompID comp_id;

	void get(Entity* entity, BaseComponent** comp);

	bool equal(const ComponentIterator& other) const noexcept;
	void next();
	void last();

    };

    SV_API ComponentIterator begin_component_iterator(CompID comp_id);
    SV_API ComponentIterator end_component_iterator(CompID comp_id);

    // TEMPLATES

    template<typename Component>
    void register_component_ex(const char* name)
    {
	ComponentRegisterDesc desc;
	desc.componentSize = sizeof(Component);
	desc.name = name;
	desc.version = Component::VERSION;

	desc.createFn = [](BaseComponent* comp_ptr)
	    {
		Component* comp = new(comp_ptr) Component();
		comp->create();
	    };

	desc.destroyFn = [](BaseComponent* comp_ptr)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_ptr);
		comp->destroy();
		comp->~Component();
	    };

	desc.moveFn = [](BaseComponent* from_ptr, BaseComponent* to_ptr)
	    {
		Component* from = reinterpret_cast<Component*>(from_ptr);
		Component* to = reinterpret_cast<Component*>(to_ptr);
		to->move(from);
	    };

	desc.copyFn = [](const BaseComponent* from_ptr, BaseComponent* to_ptr)
	    {
		const Component* from = reinterpret_cast<const Component*>(from_ptr);
		Component* to = reinterpret_cast<Component*>(to_ptr);
		to->copy(from);
	    };

	desc.serializeFn = [](BaseComponent* comp_, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->serialize(file);
	    };

	desc.deserializeFn = [](BaseComponent* comp_, u32 version, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->deserialize(version, file);
	    };

	Component::ID = register_component(&desc);
    }

    template<typename Component>
    void register_component(const char* name)
    {
	ComponentRegisterDesc desc;
	desc.componentSize = sizeof(Component);
	desc.name = name;
	desc.version = Component::VERSION;

	desc.createFn = [](BaseComponent* compPtr)
	    {
		new(compPtr) Component();
	    };

	desc.destroyFn = [](BaseComponent* compPtr)
	    {
		Component* comp = reinterpret_cast<Component*>(compPtr);
		comp->~Component();
	    };

	desc.moveFn = [](BaseComponent* fromB, BaseComponent* toB)
	    {
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		u32 id = to->_id;
		*to = std::move(*from);
		to->_id = id;
	    };

	desc.copyFn = [](const BaseComponent* fromB, BaseComponent* toB)
	    {
		const Component* from = reinterpret_cast<const Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		u32 id = to->_id;
		*to = *from;
		to->_id = id;
	    };

	desc.serializeFn = [](BaseComponent* comp_, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->serialize(file);
	    };

	desc.deserializeFn = [](BaseComponent* comp_, u32 version, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->deserialize(version, file);
	    };

	Component::ID = register_component(&desc);
    }

    template<typename Component, typename... Args>
    Component* add_component(Entity entity, Args&& ... args) {
	Component component(std::forward<Args>(args)...);
	return reinterpret_cast<Component*>(add_component(entity, (BaseComponent*)& component, Component::ID, Component::SIZE));
    }

    template<typename Component>
    Component* add_component(Entity entity) {
	return reinterpret_cast<Component*>(add_component_by_id(entity, Component::ID));
    }

    template<typename Component>
    Component* get_component(Entity entity)
    {
	return reinterpret_cast<Component*>(get_component_by_id(entity, Component::ID));
    }

    template<typename Component>
    void remove_component(Entity entity) {
	remove_component_by_id(entity, Component::ID);
    }

    template<typename Component>
    struct ComponentView {
	Entity entity;
	Component* comp;
    };

    template<typename Component>
    struct EntityView {

	struct TemplatedComponentIterator {
	    ComponentIterator it;

	    TemplatedComponentIterator(CompID compID, bool end)
		{
		    if (end)
			it = end_component_iterator(compID);
		    else 
			it = begin_component_iterator(compID);
		}

	    SV_INLINE ComponentView<Component> operator*() 
		{ 
		    ComponentView<Component> view;
		    it.get(&view.entity, reinterpret_cast<BaseComponent**>(&view.comp));
		    return view;
		}

	    SV_INLINE bool operator==(const TemplatedComponentIterator& other) const noexcept { return it.equal(other.it); }
	    SV_INLINE bool operator!=(const TemplatedComponentIterator& other) const noexcept { return !it.equal(other.it); }
	    SV_INLINE void operator+=(u32 count) { while (count-- > 0) it.next(); }
	    SV_INLINE void operator-=(u32 count) { while (count-- > 0) it.last(); }
	    SV_INLINE void operator++() { it.next(); }
	    SV_INLINE void operator--() { it.last(); }
	};

	EntityView() {}

	SV_INLINE u32 size()
	    {
		return get_component_count(Component::ID);
	    }

	SV_INLINE TemplatedComponentIterator begin()
	    {
		TemplatedComponentIterator iterator(Component::ID, false);
		return iterator;
	    }

	SV_INLINE TemplatedComponentIterator end()
	    {
		TemplatedComponentIterator iterator(Component::ID, true);
		return iterator;
	    }

    };

    ///////////////////////////////////////////////////////// COMPONENTS /////////////////////////////////////////////////////////

    struct SpriteComponent : public BaseComponent {

	static CompID SV_API ID;
	static constexpr u32 VERSION = 0u;
	
	TextureAsset texture;
	v4_f32	     texcoord = { 0.f, 0.f, 1.f, 1.f };
	Color	     color = Color::White();
	u32	     layer = 0u;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

    };

    enum ProjectionType : u32 {
	ProjectionType_Clip,
	ProjectionType_Orthographic,
	ProjectionType_Perspective,
    };

    struct CameraComponent : public BaseComponent {

	static CompID SV_API ID;
	static constexpr u32 VERSION = 0u;

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

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

	SV_INLINE void adjust(f32 aspect)
	{
	    if (width / height == aspect) return;
	    v2_f32 res = { width, height };
	    f32 mag = res.length();
	    res.x = aspect;
	    res.y = 1.f;
	    res.normalize();
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

    struct MeshComponent : public BaseComponent {

	static CompID SV_API ID;
	static constexpr u32 VERSION = 0u;
	
	MeshAsset		mesh;
	MaterialAsset	material;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

    };

    enum LightType : u32 {
	LightType_Point,
	LightType_Direction,
	LightType_Spot,
    };

    struct LightComponent : public BaseComponent {

	static CompID SV_API ID;
	static constexpr u32 VERSION = 0u;
	
	LightType light_type = LightType_Point;
	Color color = Color::White();
	f32 intensity = 1.f;
	f32 range = 5.f;
	f32 smoothness = 0.5f;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

    };

    enum BodyType : u32 {
	BodyType_Static,
	BodyType_Dynamic,
    };

    struct BodyComponent : public BaseComponent {

	static CompID SV_API ID;
	static constexpr u32 VERSION = 0u;

	BodyType body_type = BodyType_Static;
	v2_f32 vel;
	v2_f32 size = { 1.f, 1.f };
	v2_f32 offset = { 0.f, 0.f };
	bool in_ground = false;
	f32 mass = 1.f;
	f32 friction = 0.99f;
	f32 bounciness = 0.f;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);
	
    };

    // EVENTS
    
    // on_body_collision
    struct BodyCollisionEvent {
	ComponentView<BodyComponent> body0;
	ComponentView<BodyComponent> body1;
    };

}
