#pragma once

#include "core_editor.h"
#include "scene.h"

namespace sve {

	struct DebugCamera {
		sv::CameraSettings		settings;
		sv::Offscreen			offscreen;
		sv::vec3				position;
		sv::vec3				rotation;
	};

	sv::Result scene_editor_initialize();
	sv::Result scene_editor_close();

	void scene_editor_update(float dt);
	void scene_editor_render();

	DebugCamera& scene_editor_camera_get();

}