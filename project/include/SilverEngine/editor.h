#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	void editor_camera_controller2D(v2_f32& position, CameraProjection& projection);
	void editor_camera_controller3D(v3_f32& position, v2_f32& rotation, CameraProjection& projection);

}