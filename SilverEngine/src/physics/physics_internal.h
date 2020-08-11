#pragma once

#include "physics.h"

namespace sv {

	bool physics_initialize(const sv::InitializationPhysicsDesc& desc);
	bool physics_close();
	void physics_update(float dt);

	struct PhysicsWorld {
		void* pInternal2D;
	};

	PhysicsWorld	physics_world_create();
	void			physics_world_destroy(PhysicsWorld& world);
	void			physics_world_clear(PhysicsWorld& world);

}