#pragma once

#ifdef SV_DEV

#include "SilverEngine/scene.h"

namespace sv {

	struct DebugCamera : public CameraComponent {
		v3_f32 position;
		v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
		f32 velocity = 0.45f;
	};

	struct GlobalDevData {
		
		bool console_active = false;

		bool debug_draw = true;
		DebugCamera camera;
	};

	extern GlobalDevData dev;

}

#endif
