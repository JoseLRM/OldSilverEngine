#pragma once

#include "core.h"

struct SV_PHYSICS_INITIALIZATION_DESC {
};

namespace _sv {

	bool physics_initialize(const SV_PHYSICS_INITIALIZATION_DESC& desc);	
	bool physics_close();

}

namespace sv {

	struct PhysicsWorld {
		void* pInternal2D;
	};

	typedef void* Body2D;
	typedef void* Collider2D;

}

enum SV_PHY_FORCE_TYPE {
	SV_PHY_FORCE_TYPE_FORCE,
	SV_PHY_FORCE_TYPE_IMPULSE,
};

enum SV_PHY2D_COLLIDER_TYPE {
	SV_PHY2D_COLLIDER_TYPE_BOX,
	SV_PHY2D_COLLIDER_TYPE_CIRCLE,
};

struct SV_PHY_WORLD_DESC {
	sv::vec3 gravity;
};

struct SV_PHY2D_BODY_DESC {
	sv::vec2	position;
	sv::vec2	velocity;
	float		rotation;
	float		angularVelocity;
	bool		dynamic;
	bool		fixedRotation;
};

struct SV_PHY2D_COLLIDER_DESC {

	sv::Body2D				body;
	SV_PHY2D_COLLIDER_TYPE	colliderType;
	float					density;

	union {

		struct {
			float width;
			float height;
		} box;

		struct {
			float radius;
		} circle;

	};

};

namespace sv {

	void physics_update(float dt);

	// World

	PhysicsWorld	physics_world_create(const SV_PHY_WORLD_DESC* desc);
	void			physics_world_destroy(PhysicsWorld& world);
	void			physics_world_clear(PhysicsWorld& world);

	// Body 2D

	Body2D	physics_2d_body_create(const SV_PHY2D_BODY_DESC* desc, PhysicsWorld& world);
	void	physics_2d_body_destroy(Body2D body);
	void	physics_2d_body_set_dynamic(bool dynamic, Body2D body);
	bool	physics_2d_body_get_dynamic(Body2D body);
	void	physics_2d_body_set_fixed_rotation(bool fixedRotation, Body2D body);
	bool	physics_2d_body_get_fixed_rotation(Body2D body);
	void	physics_2d_body_apply_force(vec2 force, SV_PHY_FORCE_TYPE forceType, Body2D body);

	// Colliders 2D

	Collider2D				physics_2d_collider_create(const SV_PHY2D_COLLIDER_DESC* desc);
	void					physics_2d_collider_destroy(Collider2D collider);
	void					physics_2d_collider_set_density(float density, Collider2D collider);
	float					physics_2d_collider_get_density(Collider2D collider);
	SV_PHY2D_COLLIDER_TYPE	physics_2d_collider_get_type(Collider2D collider);

}