#pragma once

#include "core.h"

namespace sv {

	struct InitializationPhysicsDesc {
	};

	typedef void* Body2D;
	typedef void* Collider2D;

	enum ForceType {
		ForceType_Force,
		ForceType_Impulse,
	};
	
	enum Collider2DType {
		Collider2DType_Box,
		Collider2DType_Circle,
	};
	
	struct Body2DDesc {
		sv::vec2	position;
		sv::vec2	velocity;
		float		rotation;
		float		angularVelocity;
		bool		dynamic;
		bool		fixedRotation;
	};
	
	struct Collider2DDesc {
	
		sv::Body2D				body;
		Collider2DType			colliderType;
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

	// Body 2D

	Body2D	body2D_create(const Body2DDesc* desc);
	void	body2D_destroy(Body2D body);
	void	body2D_dynamic_set(bool dynamic, Body2D body);
	bool	body2D_dynamic_get(Body2D body);
	void	body2D_fixedRotation_set(bool fixedRotation, Body2D body);
	bool	body2D_fixedRotation_get(Body2D body);
	void	body2D_apply_force(vec2 force, ForceType forceType, Body2D body);

	// Colliders 2D

	Collider2D		collider2D_create(const Collider2DDesc* desc);
	void			collider2D_destroy(Collider2D collider);
	void			collider2D_density_set(float density, Collider2D collider);
	float			collider2D_density_get(Collider2D collider);
	Collider2DType	collider2D_type_get(Collider2D collider);

}