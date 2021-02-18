#pragma once

#include "core.h"

#define SV_ENTITY_NULL 0u
#define SV_COMPONENT_ID_INVALID std::numeric_limits<sv::CompID>::max()
#define SV_DEFINE_COMPONENT(name) struct name : public sv::Component<name> 

namespace sv {

	typedef u16 CompID;
	typedef u32 Entity;
	SV_DEFINE_HANDLE(ECS);

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
	typedef void(*SerializeComponentFunction)(BaseComponent* comp, ArchiveO&);
	typedef void(*DeserializeComponentFunction)(BaseComponent* comp, ArchiveI&);

	struct ComponentRegisterDesc {

		const char*						name;
		u32								componentSize;
		CreateComponentFunction			createFn;
		DestroyComponentFunction		destroyFn;
		MoveComponentFunction			moveFn;
		CopyComponentFunction			copyFn;
		SerializeComponentFunction		serializeFn;
		DeserializeComponentFunction	deserializeFn;

	};

	class Transform {
		void* trans;
		ECS* pECS;

	public:
		Transform(Entity entity, void* transform, ECS* ecs);
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

	private:
		void updateWorldMatrix();
		void notify();

	};

	void	ecs_create(ECS** ecs);
	void	ecs_destroy(ECS* ecs);
	void	ecs_clear(ECS* ecs);

	Result	ecs_serialize(ECS* ecs, ArchiveO& archive);
	Result	ecs_deserialize(ECS* ecs, ArchiveI& archive); // Must create the ECS before deserialize it

	// Component Register

	CompID ecs_component_register(const ComponentRegisterDesc* desc);

	const char* ecs_component_name(CompID ID);
	u32			ecs_component_size(CompID ID);
	CompID		ecs_component_id(const char* name);
	u32			ecs_component_register_count();

	void		ecs_component_create(CompID ID, BaseComponent* ptr, Entity entity);
	void		ecs_component_destroy(CompID ID, BaseComponent* ptr);
	void		ecs_component_move(CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_component_copy(CompID ID, BaseComponent* from, BaseComponent* to);
	void		ecs_component_serialize(CompID ID, BaseComponent* comp, ArchiveO& archive);
	void		ecs_component_deserialize(CompID ID, BaseComponent* comp, ArchiveI& archive);
	bool		ecs_component_exist(CompID ID);

	// Entity

	Entity			ecs_entity_create(ECS* ecs, Entity parent = SV_ENTITY_NULL, const char* name = nullptr);
	void			ecs_entity_destroy(ECS* ecs, Entity entity);
	void			ecs_entity_clear(ECS* ecs, Entity entity);
	Entity			ecs_entity_duplicate(ECS* ecs, Entity entity);
	bool			ecs_entity_is_empty(ECS* ecs, Entity entity);
	bool			ecs_entity_exist(ECS* ecs, Entity entity);
	std::string&	ecs_entity_name(ECS* ecs, Entity entity);
	u32				ecs_entity_childs_count(ECS* ecs, Entity parent);
	void			ecs_entity_childs_get(ECS* ecs, Entity parent, Entity const** childsArray);
	Entity			ecs_entity_parent_get(ECS* ecs, Entity entity);
	Transform		ecs_entity_transform_get(ECS* ecs, Entity entity);
	u32*                    ecs_entity_flags(ECS* ecs, Entity entity);
	u32				ecs_entity_component_count(ECS* ecs, Entity entity);

	void ecs_entities_create(ECS* ecs, u32 count, Entity parent = SV_ENTITY_NULL, Entity* entities = nullptr);
	void ecs_entities_destroy(ECS* ecs, Entity const* entities, u32 count);

	u32	ecs_entity_count(ECS* ecs);
	Entity	ecs_entity_get(ECS* ecs, u32 index);

	// Components

	BaseComponent* ecs_component_add(ECS* ecs, Entity entity, BaseComponent* comp, CompID componentID, size_t componentSize);
	BaseComponent* ecs_component_add_by_id(ECS* ecs, Entity entity, CompID componentID);

	BaseComponent* ecs_component_get_by_id(ECS* ecs, Entity entity, CompID componentID);
	std::pair<CompID, BaseComponent*>	ecs_component_get_by_index(ECS* ecs, Entity entity, u32 index);

	void ecs_component_remove_by_id(ECS* ecs, Entity entity, CompID componentID);

	u32 ecs_component_count(ECS* ecs, CompID ID);

	// Iterators

	class ComponentIterator {
		ECS* ecs_;
		CompID compID;

		BaseComponent* it;
		u32 pool;

	public:
		ComponentIterator(ECS* ecs, CompID compID, bool end);

		BaseComponent* get_ptr();

		void start_begin();
		void start_end();

		bool equal(const ComponentIterator& other) const noexcept;
		void next();
		void last();

	};

	// TEMPLATES

	template<typename Component>
	void ecs_component_register(const char* name)
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

		desc.serializeFn = [](BaseComponent* comp_, ArchiveO& file)
		{
			Component* comp = reinterpret_cast<Component*>(comp_);
			comp->serialize(file);
		};

		desc.deserializeFn = [](BaseComponent* comp_, ArchiveI& file)
		{
			Component* comp = new(comp_) Component();
			comp->deserialize(file);
		};

		Component::ID = ecs_component_register(&desc);
	}

	template<typename Component, typename... Args>
	Component* ecs_component_add(ECS* ecs, Entity entity, Args&& ... args) {
		Component component(std::forward<Args>(args)...);
		return reinterpret_cast<Component*>(ecs_component_add(ecs, entity, (BaseComponent*)& component, Component::ID, Component::SIZE));
	}

	template<typename Component>
	Component* ecs_component_add(ECS* ecs, Entity entity) {
		return reinterpret_cast<Component*>(ecs_component_add_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	Component* ecs_component_get(ECS* ecs, Entity entity)
	{
		return reinterpret_cast<Component*>(ecs_component_get_by_id(ecs, entity, Component::ID));
	}

	template<typename Component>
	void ecs_component_remove(ECS* ecs, Entity entity) {
		ecs_component_remove_by_id(ecs, entity, Component::ID);
	}

	template<typename Component>
	class EntityView {

		ECS* m_ECS;

	public:

		class TemplatedComponentIterator {
			ComponentIterator it;

		public:
			TemplatedComponentIterator(ECS* ecs, CompID compID, bool end)
				: it(ecs, compID, end) {}
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
		EntityView(ECS* ecs) : m_ECS(ecs) {}

		u32 size()
		{
			return ecs_component_count(m_ECS, Component::ID);
		}

		TemplatedComponentIterator begin()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, false);
			return iterator;
		}

		TemplatedComponentIterator end()
		{
			TemplatedComponentIterator iterator(m_ECS, Component::ID, true);
			return iterator;
		}

	};

}
