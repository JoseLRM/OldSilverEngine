#pragma once

#include "SilverEngine/scene.h"

namespace sv {

	void key_shortcuts();

	void camera_controller2D(ECS* ecs, CameraComponent& camera, f32 max_projection_length = f32_max);
	void camera_controller3D(ECS* ecs, CameraComponent& camera);

}