#include "core_editor.h"

#include "scene_editor.h"
#include "viewport_manager.h"
#include "input.h"
#include "scene.h"
#include "components.h"

namespace sve {

	static DebugCamera g_Camera;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(float dt)
	{
		float zoom = sv::renderer_projection_zoom_get(g_Camera.projection);

		// Movement
		sv::vec2 direction;

		if (sv::input_key('W')) {
			direction.y--;
		}
		if (sv::input_key('S')) {
			direction.y++;
		}
		if (sv::input_key('D')) {
			direction.x++;
		}
		if (sv::input_key('A')) {
			direction.x--;
		}

		float force = 8.f * dt * zoom * 0.15f;
		
		if (direction.Mag() != 0.f) {
			direction.Normalize();
			direction *= force;
			g_Camera.position += sv::vec3(direction.x, direction.y, 0.f);
		}

		// Zoom
		float zoomForce = dt * zoom;

		if (sv::input_key(SV_KEY_SPACE)) {
			zoom += zoomForce;
		}
		if (sv::input_key(SV_KEY_SHIFT)) {
			zoom -= zoomForce;
		}

		sv::renderer_projection_zoom_set(g_Camera.projection, zoom);

	}

	void scene_editor_camera_controller_3D(float dt)
	{

	}

	// MAIN FUNCTIONS

	bool scene_editor_initialize()
	{
		// Create camera
		{
			sv::uvec2 res = sv::renderer_resolution_get();
			svCheck(sv::renderer_offscreen_create(res.x, res.y, g_Camera.offscreen));
			sv::CameraComponent& mainCamera = *sv::scene_ecs_component_get<sv::CameraComponent>(sv::scene_camera_get());
			g_Camera.projection = mainCamera.projection;
		}
		return true;
	}

	bool scene_editor_close()
	{
		return true;
	}

	void scene_editor_update(float dt)
	{
		auto props = viewport_manager_properties_get("Scene Editor");

		// Adjust camera
		sv::renderer_projection_aspect_set(g_Camera.projection, float(props.width) / float(props.height));
		
		// Camera Controller
		if (g_Camera.projection.cameraType == sv::CameraType_Orthographic) {
			scene_editor_camera_controller_2D(dt);
		}
		else if (g_Camera.projection.cameraType == sv::CameraType_Perspective) {
			scene_editor_camera_controller_3D(dt);
		}
		
	}

	void scene_editor_render()
	{
		sv::RendererDesc desc;
		desc.rendererTarget = sv::RendererTarget_CameraOffscreen;
		desc.camera.pOffscreen = &g_Camera.offscreen;
		desc.camera.projection = g_Camera.projection;
		desc.camera.settings = g_Camera.settings;
		desc.camera.viewMatrix = XMMatrixTranslation(-g_Camera.position.x, -g_Camera.position.y, -g_Camera.position.z);

		sv::renderer_scene_begin(&desc);
		sv::renderer_scene_draw_scene();
		sv::renderer_scene_end();
	}

	// GETTERS

	DebugCamera& scene_editor_camera_get()
	{
		return g_Camera;
	}

}