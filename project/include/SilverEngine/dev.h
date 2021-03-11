#pragma once

#ifdef SV_DEV

#include "SilverEngine/scene.h"

namespace sv {

	struct GlobalDevData {
		
		bool console_active = false;

		bool debug_draw = true;
		CameraComponent debug_camera;
		v3_f32 debug_camera_position;
		v4_f32 debug_camera_rotation = { 0.f, 0.f, 0.f, 1.f };
	};

	extern GlobalDevData dev;

}

#endif
