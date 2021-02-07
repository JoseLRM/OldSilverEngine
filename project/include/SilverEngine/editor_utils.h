#pragma once

#include "SilverEngine/core.h"

namespace sv {

	struct CameraProjection;

	void key_shortcuts();

	void camera_controller2D(v2_f32& position, CameraProjection& projection, f32 max_projection_length = f32_max);
	void camera_controller3D(v3_f32& position, v2_f32& rotation, CameraProjection& projection);

}