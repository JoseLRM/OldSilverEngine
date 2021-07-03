#pragma once

#include "core/scene.h"

namespace sv {

	enum BodyType : u32 {
		BodyType_Static,
		BodyType_Dynamic,
		BodyType_Kinematic,
	};

	struct BodyComponent : public Component {

		static constexpr u32 VERSION = 1u;

		void* _internal;
		BodyType _type;
		
	};

	struct BoxCollider : public Component {

		static constexpr u32 VERSION = 1u;

		v3_f32 size;
		
		void* _internal;
		
	};

	struct SphereCollider : public Component {

		static constexpr u32 VERSION = 1u;

		f32 radius;
		
		void* _internal;
		
	};

	SV_API void body_type_set(BodyComponent& body, BodyType body_type);
	SV_API void body_linear_damping_set(BodyComponent& body, f32 linear_damping);
	SV_API void body_angular_damping_set(BodyComponent& body, f32 angular_damping);
	SV_API void body_velocity_set(BodyComponent& body, v3_f32 velocity);
	SV_API void body_angular_velocity_set(BodyComponent& body, v3_f32 angular_velocity);
	SV_API void body_mass_set(BodyComponent& body, f32 mass);

	SV_API BodyType body_type_get(const BodyComponent& body);
	SV_API f32      body_linear_damping_get(const BodyComponent& body);
	SV_API f32      body_angular_damping_get(const BodyComponent& body);
	SV_API v3_f32   body_velocity_get(const BodyComponent& body);
	SV_API v3_f32   body_angular_velocity_get(const BodyComponent& body);
	SV_API f32      body_mass_get(const BodyComponent& body);

	struct OnBodyCollisionEvent {
		Entity entity0;
		Entity entity1;
		BodyComponent* body0;
		BodyComponent* body1;
	};

	bool _physics3D_initialize();
	void _physics3D_close();
	void _physics3D_update();

	void BodyComponent_create(BodyComponent* comp, Entity entity);
	void BodyComponent_destroy(BodyComponent* comp, Entity entity);
	void BodyComponent_copy(BodyComponent* dst, const BodyComponent* src, Entity entity);
	void BodyComponent_serialize(BodyComponent* comp, Serializer& s);
	void BodyComponent_deserialize(BodyComponent* comp, Deserializer& d, u32 version);

	void BoxCollider_create(BoxCollider* comp, Entity entity);
	void BoxCollider_destroy(BoxCollider* comp, Entity entity);
	void BoxCollider_copy(BoxCollider* dst, const BoxCollider* src, Entity entity);
	void BoxCollider_serialize(BoxCollider* comp, Serializer& s);
	void BoxCollider_deserialize(BoxCollider* comp, Deserializer& d, u32 version);

	void SphereCollider_create(SphereCollider* comp, Entity entity);
	void SphereCollider_destroy(SphereCollider* comp, Entity entity);
	void SphereCollider_copy(SphereCollider* dst, const SphereCollider* src, Entity entity);
	void SphereCollider_serialize(SphereCollider* comp, Serializer& s);
	void SphereCollider_deserialize(SphereCollider* comp, Deserializer& d, u32 version);
	
}
