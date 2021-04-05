#pragma once

#include "graphics.h"
#include "mesh.h"
#include "gui.h"

#define SV_ENTITY_NULL 0u
#define SV_COMPONENT_ID_INVALID std::numeric_limits<sv::CompID>::max()
#define SV_DEFINE_COMPONENT(name, version) struct name : public sv::Component<name, version>

namespace sv {

    typedef u16 CompID;
    typedef u32 Entity;

    struct CameraComponent;	
    struct AudioDevice;	

    struct Scene {

	static constexpr u32 VERSION = 0u;
		
	GUI* gui = nullptr;
	void* user_ptr = nullptr;
	u32 user_id = 0u;
	Entity main_camera = SV_ENTITY_NULL;

	GPUImage* skybox = nullptr;

	AudioDevice* audio_device;

	std::string name;

    };

    SV_API Result set_active_scene(const char* name);
    SV_API Result save_scene(Scene* scene, const char* filepath);
    SV_API Result clear_scene(Scene* scene);

    SV_API Result create_entity_model(Scene* scene, Entity parent, const char* filepath);

    SV_API CameraComponent* get_main_camera(Scene* scene);

    SV_API Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera);
	
    ////////////////////////////////////////// ECS ////////////////////////////////////////////////////////

    enum EntityFlag : u32 {
	EntityFlag_None = 0u,
	EntityFlag_NoSerialize = SV_BIT(0u)
    };

    struct BaseComponent {
	u32 _id = 0u;
    };

    template<typename T, u32 V>
    struct Component : public BaseComponent {
	static CompID SV_API_VAR ID;
	static u32 SV_API_VAR SIZE;
	const static SV_API_VAR u32 VERSION;
    };

#if SV_SILVER_ENGINE
    template<typename T, u32 V>
    CompID Component<T, V>::ID(SV_COMPONENT_ID_INVALID);

    template<typename T, u32 V>
    u32 Component<T, V>::SIZE;

    template<typename T, u32 V>
    const u32 Component<T, V>::VERSION(V);
#endif
    
    typedef void(*CreateComponentFunction)(Scene* scene, BaseComponent*);
    typedef void(*DestroyComponentFunction)(Scene* scene, BaseComponent*);
    typedef void(*MoveComponentFunction)(Scene* scene, BaseComponent* from, BaseComponent* to);
    typedef void(*CopyComponentFunction)(Scene* scene, const BaseComponent* from, BaseComponent* to);
    typedef void(*SerializeComponentFunction)(Scene* scene, BaseComponent* comp, Archive&);
    typedef void(*DeserializeComponentFunction)(Scene* scene, BaseComponent* comp, u32 version, Archive&);

    struct ComponentRegisterDesc {

	const char* name;
	u32								componentSize;
	u32 version;
	CreateComponentFunction			createFn;
	DestroyComponentFunction		destroyFn;
	MoveComponentFunction			moveFn;
	CopyComponentFunction			copyFn;
	SerializeComponentFunction		serializeFn;
	DeserializeComponentFunction	deserializeFn;

    };

    // Component Register

    SV_API CompID register_component(const ComponentRegisterDesc* desc);

    SV_API const char* get_component_name(CompID ID);
    SV_API u32			get_component_size(CompID ID);
    SV_API u32			get_component_version(CompID ID);
    SV_API CompID		get_component_id(const char* name);
    SV_API u32			get_component_register_count();

    SV_API void		create_component(Scene* scene, CompID ID, BaseComponent* ptr, Entity entity);
    SV_API void		destroy_component(Scene* scene, CompID ID, BaseComponent* ptr);
    SV_API void		move_component(Scene* scene, CompID ID, BaseComponent* from, BaseComponent* to);
    SV_API void		copy_component(Scene* scene, CompID ID, const BaseComponent* from, BaseComponent* to);
    SV_API void		serialize_component(Scene* scene, CompID ID, BaseComponent* comp, Archive& archive);
    SV_API void		deserialize_component(Scene* scene, CompID ID, BaseComponent* comp, u32 version, Archive& archive);
    SV_API bool		component_exist(CompID ID);

    // Transform

    class SV_API Transform {
	void* trans;
	Scene* scene;

    public:
	Transform(Entity entity, void* transform, Scene* ecs);
	~Transform() = default;

	Transform(const Transform& other) = default;
	Transform(Transform&& other) = default;

	const Entity entity = 0;

	// getters
	const v3_f32& getLocalPosition() const noexcept;
	const v4_f32& getLocalRotation() const noexcept;
	v3_f32			getLocalEulerRotation() const noexcept;
	const v3_f32& getLocalScale() const noexcept;
	XMVECTOR getLocalPositionDXV() const noexcept;
	XMVECTOR getLocalRotationDXV() const noexcept;
	XMVECTOR getLocalScaleDXV() const noexcept;
	XMMATRIX getLocalMatrix() const noexcept;

	v3_f32 getWorldPosition() noexcept;
	v4_f32 getWorldRotation() noexcept;
	v3_f32 getWorldEulerRotation() noexcept;
	v3_f32 getWorldScale() noexcept;
	XMVECTOR getWorldPositionDXV() noexcept;
	XMVECTOR getWorldRotationDXV() noexcept;
	XMVECTOR getWorldScaleDXV() noexcept;
	XMMATRIX getWorldMatrix() noexcept;

	XMMATRIX getParentMatrix() const noexcept;

	// setters
	void setPosition(const v3_f32& position) noexcept;
	void setPositionX(float x) noexcept;
	void setPositionY(float y) noexcept;
	void setPositionZ(float z) noexcept;

	void setRotation(const v4_f32& rotation) noexcept;
	void setEulerRotation(const v3_f32& rotation) noexcept;
	void setRotationX(float x) noexcept;
	void setRotationY(float y) noexcept;
	void setRotationZ(float z) noexcept;
	void setRotationW(float w) noexcept;

	void rotateRollPitchYaw(f32 roll, f32 pitch, f32 yaw);

	void setScale(const v3_f32& scale) noexcept;
	void setScaleX(float x) noexcept;
	void setScaleY(float y) noexcept;
	void setScaleZ(float z) noexcept;

	void setMatrix(const XMMATRIX& matrix) noexcept;

    private:
	void updateWorldMatrix();
	void notify();

    };

    // Entity

    SV_API Entity       create_entity(Scene* scene, Entity parent = SV_ENTITY_NULL, const char* name = nullptr);
    SV_API void	        destroy_entity(Scene* scene, Entity entity);
    SV_API void	        clear_entity(Scene* scene, Entity entity);
    SV_API Entity       duplicate_entity(Scene* scene, Entity entity);
    SV_API bool	        entity_is_empty(Scene* scene, Entity entity);
    SV_API bool	        entity_exist(Scene* scene, Entity entity);
    SV_API const char*  get_entity_name(Scene* scene, Entity entity);
    SV_API void	        set_entity_name(Scene* scene, Entity entity, const char* name);
    SV_API u32	        get_entity_childs_count(Scene* scene, Entity parent);
    SV_API void		get_entity_childs(Scene* scene, Entity parent, Entity const** childsArray);
    SV_API Entity	get_entity_parent(Scene* scene, Entity entity);
    SV_API Transform	get_entity_transform(Scene* scene, Entity entity);
    SV_API u32*		get_entity_flags(Scene* scene, Entity entity);
    SV_API u32		get_entity_component_count(Scene* scene, Entity entity);
    SV_API u32		get_entity_count(Scene* scene);
    SV_API Entity	get_entity_by_index(Scene* scene, u32 index);

    // Components

    SV_API BaseComponent*						add_component(Scene* scene, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
    SV_API BaseComponent*						add_component_by_id(Scene* scene, Entity entity, CompID componentID);
    SV_API BaseComponent*						get_component_by_id(Scene* scene, Entity entity, CompID componentID);
    SV_API std::pair<CompID, BaseComponent*>	get_component_by_index(Scene* scene, Entity entity, u32 index);
    SV_API void								remove_component_by_id(Scene* scene, Entity entity, CompID componentID);
    SV_API u32									get_component_count(Scene* scene, CompID ID);

    // Iterators

    struct ComponentIterator {
		
	Scene* _scene;
	BaseComponent* _it;
	u32 _pool;

	CompID comp_id;

	void get(Entity* entity, BaseComponent** comp);

	bool equal(const ComponentIterator& other) const noexcept;
	void next();
	void last();

    };

    SV_API ComponentIterator begin_component_iterator(Scene* scene, CompID comp_id);
    SV_API ComponentIterator end_component_iterator(Scene* scene, CompID comp_id);

    // TEMPLATES

    template<typename Component>
    void register_component_ex(const char* name)
    {
	ComponentRegisterDesc desc;
	desc.componentSize = sizeof(Component);
	desc.name = name;
	desc.version = Component::VERSION;

	desc.createFn = [](Scene* scene, BaseComponent* comp_ptr)
	    {
		Component* comp = new(comp_ptr) Component();
		comp->create(scene);
	    };

	desc.destroyFn = [](Scene* scene, BaseComponent* comp_ptr)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_ptr);
		comp->destroy(scene);
		comp->~Component();
	    };

	desc.moveFn = [](Scene* scene, BaseComponent* from_ptr, BaseComponent* to_ptr)
	    {
		Component* from = reinterpret_cast<Component*>(from_ptr);
		Component* to = reinterpret_cast<Component*>(to_ptr);
		to->move(scene, from);
	    };

	desc.copyFn = [](Scene* scene, const BaseComponent* from_ptr, BaseComponent* to_ptr)
	    {
		const Component* from = reinterpret_cast<const Component*>(from_ptr);
		Component* to = reinterpret_cast<Component*>(to_ptr);
		to->copy(scene, from);
	    };

	desc.serializeFn = [](Scene* scene, BaseComponent* comp_, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->serialize(scene, file);
	    };

	desc.deserializeFn = [](Scene* scene, BaseComponent* comp_, u32 version, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->deserialize(scene, version, file);
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

	desc.createFn = [](Scene* scene, BaseComponent* compPtr)
	    {
		new(compPtr) Component();
	    };

	desc.destroyFn = [](Scene* scene, BaseComponent* compPtr)
	    {
		Component* comp = reinterpret_cast<Component*>(compPtr);
		comp->~Component();
	    };

	desc.moveFn = [](Scene* scene, BaseComponent* fromB, BaseComponent* toB)
	    {
		Component* from = reinterpret_cast<Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		u32 id = to->_id;
		*to = std::move(*from);
		to->_id = id;
	    };

	desc.copyFn = [](Scene* scene, const BaseComponent* fromB, BaseComponent* toB)
	    {
		const Component* from = reinterpret_cast<const Component*>(fromB);
		Component* to = reinterpret_cast<Component*>(toB);
		u32 id = to->_id;
		*to = *from;
		to->_id = id;
	    };

	desc.serializeFn = [](Scene* scene, BaseComponent* comp_, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->serialize(file);
	    };

	desc.deserializeFn = [](Scene* scene, BaseComponent* comp_, u32 version, Archive& file)
	    {
		Component* comp = reinterpret_cast<Component*>(comp_);
		comp->deserialize(version, file);
	    };

	Component::ID = register_component(&desc);
    }

    template<typename Component, typename... Args>
    Component* add_component(Scene* scene, Entity entity, Args&& ... args) {
	Component component(std::forward<Args>(args)...);
	return reinterpret_cast<Component*>(add_component(scene, entity, (BaseComponent*)& component, Component::ID, Component::SIZE));
    }

    template<typename Component>
    Component* add_component(Scene* scene, Entity entity) {
	return reinterpret_cast<Component*>(add_component_by_id(scene, entity, Component::ID));
    }

    template<typename Component>
    Component* get_component(Scene* scene, Entity entity)
    {
	return reinterpret_cast<Component*>(get_component_by_id(scene, entity, Component::ID));
    }

    template<typename Component>
    void remove_component(Scene* scene, Entity entity) {
	remove_component_by_id(scene, entity, Component::ID);
    }

    template<typename Component>
    struct ComponentView {
	Entity entity;
	Component* comp;
    };

    template<typename Component>
    struct EntityView {

	Scene* _scene;

	struct TemplatedComponentIterator {
	    ComponentIterator it;

	    TemplatedComponentIterator(Scene* scene, CompID compID, bool end)
		{
		    if (end)
			it = end_component_iterator(scene, compID);
		    else 
			it = begin_component_iterator(scene, compID);
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

	EntityView(Scene* scene) : _scene(scene) {}

	SV_INLINE u32 size()
	    {
		return get_component_count(_scene, Component::ID);
	    }

	SV_INLINE TemplatedComponentIterator begin()
	    {
		TemplatedComponentIterator iterator(_scene, Component::ID, false);
		return iterator;
	    }

	SV_INLINE TemplatedComponentIterator end()
	    {
		TemplatedComponentIterator iterator(_scene, Component::ID, true);
		return iterator;
	    }

    };

    ///////////////////////////////////////////////////////// COMPONENTS /////////////////////////////////////////////////////////

    SV_DEFINE_COMPONENT(SpriteComponent, 0u) {

	TextureAsset	texture;
	v4_f32			texcoord = { 0.f, 0.f, 1.f, 1.f };
	Color			color = Color::White();
	u32				layer = 0u;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

    };

    enum ProjectionType : u32 {
	ProjectionType_Clip,
	ProjectionType_Orthographic,
	ProjectionType_Perspective,
    };

    SV_DEFINE_COMPONENT(CameraComponent, 0u) {

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

    SV_DEFINE_COMPONENT(MeshComponent, 0u) {

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

    SV_DEFINE_COMPONENT(LightComponent, 0u) {

	LightType light_type = LightType_Point;
	Color color = Color::White();
	f32 intensity = 1.f;
	f32 range = 5.f;
	f32 smoothness = 0.5f;

	void serialize(Archive & archive);
	void deserialize(u32 version, Archive & archive);

    };

}