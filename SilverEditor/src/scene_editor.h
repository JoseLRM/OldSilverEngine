#pragma once

#include "core_editor.h"
#include "simulation/scene.h"

namespace sv {

	struct DebugCamera {
		Camera				camera;
		vec3f				position;
		vec4f				rotation;
	};

	enum SceneEditorMode : ui32 {
		SceneEditorMode_2D,
		SceneEditorMode_3D,
	};

	Result scene_editor_initialize();
	Result scene_editor_close();

	void scene_editor_update(float dt);
	void scene_editor_render();

	DebugCamera& scene_editor_camera_get();

	SceneEditorMode scene_editor_mode_get();
	void			scene_editor_mode_set(SceneEditorMode mode);

	void	scene_editor_selected_entity_set(Entity entity);
	Entity	scene_editor_selected_entity_get();

}