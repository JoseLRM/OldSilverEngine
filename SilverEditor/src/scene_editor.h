#pragma once

#include "core_editor.h"
#include "renderer.h"

namespace sve {

	struct DebugCamera {
		sv::CameraProjection	projection;
		sv::CameraSettings		settings;
		sv::Offscreen			offscreen;
		sv::vec3				position;
		sv::vec2				rotation;
	};

	bool scene_editor_initialize();
	bool scene_editor_close();

	void scene_editor_update(float dt);
	void scene_editor_render();

	DebugCamera& scene_editor_camera_get();

}