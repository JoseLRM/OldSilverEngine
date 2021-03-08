#pragma once

#include "SilverEngine/entity_system.h"
#include "SilverEngine/graphics.h"
#include "SilverEngine/mesh.h"
#include "SilverEngine/gui.h"

#define SV_ENTITY_NULL 0u
#define SV_COMPONENT_ID_INVALID std::numeric_limits<sv::CompID>::max()
#define SV_DEFINE_COMPONENT(name) struct name : public sv::Component<name>

namespace sv {

	typedef u16 CompID;
	typedef u32 Entity;

	struct CameraComponent;	

	struct Scene {

		Entity main_camera = SV_ENTITY_NULL;

		std::string name;

	};

	Result set_active_scene(const char* name);
	Result save_scene(Scene* scene, const char* filepath);
	Result clear_scene(Scene* scene);

	CameraComponent* get_main_camera(Scene* scene);

	Ray screen_to_world_ray(v2_f32 position, const v3_f32& camera_position, const v4_f32& camera_rotation, CameraComponent* camera);
	
	////////////////////////////////////////// ECS ////////////////////////////////////////////////////////

	enum EntityFlag : u32 {
		EntityFlag_None = 0u,
		EntityFlag_NoSerialize = SV_BIT(0u)
	};

	struct BaseComponent {
		Entity entity = SV_ENTITY_NULL;
	};

	template<typename T>
	struct Component : public BaseComponent {
		static CompID ID;
		static u32 SIZE;
	};

	template<typename T>
	CompID Component<T>::ID(SV_COMPONENT_ID_INVALID);

	template<typename T>
	u32 Component<T>::SIZE;

	typedef void(*CreateComponentFunction)(BaseComponent*);
	typedef void(*DestroyComponentFunction)(BaseComponent*);
	typedef void(*MoveComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*CopyComponentFunction)(BaseComponent* from, BaseComponent* to);
	typedef void(*SerializeComponentFunction)(BaseComponent* comp, Archive&);
	typedef void(*DeserializeComponentFunction)(BaseComponent* comp, Archive&);

	struct ComponentRegisterDesc {

		const char* name;
		u32								componentSize;
		CreateComponentFunction			createFn;
		DestroyComponentFunction		destroyFn;
		MoveComponentFunction			moveFn;
		CopyComponentFunction			copyFn;
		SerializeComponentFunction		serializeFn;
		DeserializeComponentFunction	deserializeFn;

	};

	// Component Register

	CompID register_component(const ComponentRegisterDesc* desc);

	const char* get_component_name(CompID ID);
	u32			get_component_size(CompID ID);
	CompID		get_component_id(const char* name);
	u32			get_component_register_count();

	void		create_component(CompID ID, BaseComponent* ptr, Entity entity);
	void		destroy_component(CompID ID, BaseComponent* ptr);
	void		move_component(CompID ID, BaseComponent* from, BaseComponent* to);
	void		copy_component(CompID ID, BaseComponent* from, BaseComponent* to);
	void		serialize_component(CompID ID, BaseComponent* comp, Archive& archive);
	void		deserialize_component(CompID ID, BaseComponent* comp, Archive& archive);
	bool		component_exist(CompID ID);

	// Transform

	class Transform {
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

	Entity			create_entity(Scene* scene, Entity parent = SV_ENTITY_NULL, const char* name = nullptr);
	void			destroy_entity(Scene* scene, Entity entity);
	void			clear_entity(Scene* scene, Entity entity);
	Entity			duplicate_entity(Scene* scene, Entity entity);
	bool			entity_is_empty(Scene* scene, Entity entity);
	bool			entity_exist(Scene* scene, Entity entity);
	const char*		get_entity_name(Scene* scene, Entity entity);
	void			set_entity_name(Scene* scene, Entity entity, const char* name);
	u32				get_entity_childs_count(Scene* scene, Entity parent);
	void			get_entity_childs(Scene* scene, Entity parent, Entity const** childsArray);
	Entity			get_entity_parent(Scene* scene, Entity entity);
	Transform		get_entity_transform(Scene* scene, Entity entity);
	u32*			get_entity_flags(Scene* scene, Entity entity);
	u32				get_entity_component_count(Scene* scene, Entity entity);
	u32				get_entity_count(Scene* scene);
	Entity			get_entity_by_index(Scene* scene, u32 index);

	// Components

	BaseComponent*						add_component(Scene* scene, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
	BaseComponent*						add_component_by_id(Scene* scene, Entity entity, CompID componentID);
	BaseComponent*						get_component_by_id(Scene* scene, Entity entity, CompID componentID);
	std::pair<CompID, BaseComponent*>	get_component_by_index(Scene* scene, Entity entity, u32 index);
	void								remove_component_by_id(Scene* scene, Entity entity, CompID componentID);
	u32									get_component_count(Scene* scene, CompID ID);

	// Iterators

	class ComponentIterator {
		Scene* scene_;
		CompID compID;

		BaseComponent* it;
		u32 pool;

	public:
		ComponentIterator(Scene* scene, CompID compID, bool end);

		BaseComponent* get_ptr();

		void start_begin();
		void start_end();

		bool equal(const ComponentIterator& other) const noexcept;
		void next();
		void last();

	};

	// TEMPLATES

	template<typename Component>
	void register_component(const char* name)
	{
		ComponentRegisterDesc desc;
		desc.componentSize = sizeof(Component);
		desc.name = name;

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
			new(to) Component(std::move(*from));
		};

		desc.copyFn = [](BaseComponent* fromB, BaseComponent* toB)
		{
			Component* from = reinterpret_cast<Component*>(fromB);
			Component* to = reinterpret_cast<Component*>(toB);
			new(to) Component(*from);
		};

		desc.serializeFn = [](BaseComponent* comp_, Archive& file)
		{
			Component* comp = reinterpret_cast<Component*>(comp_);
			comp->serialize(file);
		};

		desc.deserializeFn = [](BaseComponent* comp_, Archive& file)
		{
			Component* comp = new(comp_) Component();
			comp->deserialize(file);
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
	class EntityView {

		Scene* m_Scene;

	public:

		class TemplatedComponentIterator {
			ComponentIterator it;

		public:
			TemplatedComponentIterator(Scene* scene, CompID compID, bool end)
				: it(scene, compID, end) {}
			inline Component* operator->() { return reinterpret_cast<Component*>(it.get_ptr()); }
			inline Component& operator*() { return *reinterpret_cast<Component*>(it.get_ptr()); }

			inline bool operator==(const TemplatedComponentIterator& other) const noexcept { return it.equal(other.it); }
			inline bool operator!=(const TemplatedComponentIterator& other) const noexcept { return !it.equal(other.it); }
			inline void operator+=(u32 count) { while (count-- > 0) it.next(); }
			inline void operator-=(u32 count) { while (count-- > 0) it.last(); }
			inline void operator++() { it.next(); }
			inline void operator--() { it.last(); }
		};

	public:
		EntityView(Scene* scene) : m_Scene(scene) {}

		u32 size()
		{
			return get_component_count(m_Scene, Component::ID);
		}

		TemplatedComponentIterator begin()
		{
			TemplatedComponentIterator iterator(m_Scene, Component::ID, false);
			return iterator;
		}

		TemplatedComponentIterator end()
		{
			TemplatedComponentIterator iterator(m_Scene, Component::ID, true);
			return iterator;
		}

	};

	///////////////////////////////////////////////////////// COMPONENTS /////////////////////////////////////////////////////////

	SV_DEFINE_COMPONENT(SpriteComponent) {

		TextureAsset	texture;
		v4_f32			texcoord = { 0.f, 0.f, 1.f, 1.f };
		Color			color = Color::White();
		u32				layer = 0u;

		void serialize(Archive& archive);
		void deserialize(Archive& archive);

	};

	enum ProjectionType : u32 {
		ProjectionType_Clip,
		ProjectionType_Orthographic,
		ProjectionType_Perspective,
	};

	SV_DEFINE_COMPONENT(CameraComponent) {

		ProjectionType projection_type = ProjectionType_Orthographic;
		f32 near = -1000.f;
		f32 far = 1000.f;
		f32 width = 10.f;
		f32 height = 10.f;

		void serialize(Archive& archive);
		void deserialize(Archive& archive);

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

	SV_DEFINE_COMPONENT(MeshComponent) {

		MeshAsset	mesh;
		Material	material;

		void serialize(Archive& archive);
		void deserialize(Archive& archive);

	};

	enum LightType : u32 {
		LightType_Point,
		LightType_Direction,
		LightType_Spot,
	};

	SV_DEFINE_COMPONENT(LightComponent) {

		LightType light_type = LightType_Point;
		Color3f color = Color3f::White();
		f32 intensity = 1.f;
		f32 range = 5.f;
		f32 smoothness = 0.5f;

		void serialize(Archive& archive);
		void deserialize(Archive& archive);

	};

}
