#include "core_editor.h"

#include "panel_manager.h"
#include "platform/input.h"
#include "simulation/scene.h"
#include "simulation.h"
#include "rendering/debug_renderer.h"

namespace sv {

	

	
	/*

	void scene_editor_update_mode2D(float dt)
	{
		SceneEditorPanel* vp = (SceneEditorPanel*)panel_manager_get("Scene Editor");
		Scene& scene = simulation_scene_get();

		

		// Compute Mouse Pos
		

		

		scene_editor_camera_controller_2D(g_MousePos, dt);

		// Actions

		

	}

	void scene_editor_update_mode3D(float dt)
	{
		scene_editor_camera_controller_3D(dt);
	}

	void scene_editor_update(float dt)
	{
		Scene& scene = simulation_scene_get();

		if (input_key(SV_KEY_CONTROL)) {

			if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed('D')) {
				ecs_entity_duplicate(scene, g_SelectedEntity);
			}

		}

		if (g_SelectedEntity != SV_ENTITY_NULL && input_key_pressed(SV_KEY_DELETE)) {
			ecs_entity_destroy(scene, g_SelectedEntity);
			g_SelectedEntity = SV_ENTITY_NULL;
		}

		SceneEditorPanel* vp = (SceneEditorPanel*) panel_manager_get("Scene Editor");
		if (vp == nullptr) return;

		// Adjust camera
		vec2u size = vp->getScreenSize();
		if (size.x != 0u && size.y != 0u)
			g_Camera.camera.adjust(size.x, size.y);
		
		if (vp->isFocused()) {
			switch (g_EditorMode)
			{
			case sv::SceneEditorMode_2D:
				scene_editor_update_mode2D(dt);
				break;
			case sv::SceneEditorMode_3D:
				scene_editor_update_mode3D(dt);
				break;
			}
		}
	}

	void scene_editor_render()
	{
		
	}

	DebugCamera& scene_editor_camera_get()
	{
		return g_Camera;
	}

	SceneEditorMode scene_editor_mode_get()
	{
		return g_EditorMode;
	}

	void scene_editor_mode_set(SceneEditorMode mode)
	{
		if (g_EditorMode == mode) return;

		g_EditorMode = mode;

		// Change camera

		switch (g_EditorMode)
		{
		case sv::SceneEditorMode_2D:
			g_Camera.camera.setProjectionType(ProjectionType_Orthographic);
			//g_Camera.settings.projection.cameraType = CameraType_Orthographic;
			//g_Camera.settings.projection.near = -100000.f;
			//g_Camera.settings.projection.far = 100000.f;
			//g_Camera.settings.projection.width = 20.f;
			//g_Camera.settings.projection.height = 20.f;
			g_Camera.position.z = 0.f;
			g_Camera.rotation = vec4f();
			break;
		case sv::SceneEditorMode_3D:
			g_Camera.camera.setProjectionType(ProjectionType_Perspective);
			//g_Camera.settings.projection.cameraType = CameraType_Perspective;
			//g_Camera.settings.projection.near = 0.01f;
			//g_Camera.settings.projection.far = 100000.f;
			//g_Camera.settings.projection.width = 0.01f;
			//g_Camera.settings.projection.height = 0.01f;
			g_Camera.position.z = -10.f;
			break;
		default:
			SV_LOG_INFO("Unknown editor mode");
			break;
		}
		
	}

	void scene_editor_selected_entity_set(Entity entity)
	{
		g_SelectedEntity = entity;
	}

	Entity scene_editor_selected_entity_get()
	{
		return g_SelectedEntity;
	}
	*/
}