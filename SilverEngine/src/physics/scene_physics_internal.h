#pragma once

#include "physics/scene_physics.h"

// BOX 2D

#include "external/box2d/b2_world.h"
#include "external/box2d/b2_body.h"
#include "external/box2d/b2_polygon_shape.h"
#include "external/box2d/b2_circle_shape.h"
#include "external/box2d/b2_fixture.h"

namespace sv {

	struct ScenePhysics_internal {

		b2World world2D = b2World({ 0.f, 5.f });

	};

	b2BodyDef scene_physics2D_body_def(b2Body* body);

}