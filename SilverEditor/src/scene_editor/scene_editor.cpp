#include "core_editor.h"

#include "scene_editor.h"
#include "viewports/viewport_scene_editor.h"
#include "input.h"
#include "scene.h"
#include "simulation.h"

namespace sve {

	static DebugCamera g_Camera;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(float dt)
	{
		float zoom = sv::renderer_projection_zoom_get(g_Camera.settings.projection);

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

		sv::renderer_projection_zoom_set(g_Camera.settings.projection, zoom);

	}

	void scene_editor_camera_controller_3D(float dt)
	{
		sv::vec2 dragged = sv::input_mouse_dragged_get();
		
		if (sv::input_mouse(SV_MOUSE_RIGHT)) {
			
			dragged *= 700.f;

			g_Camera.rotation.x += ToRadians(dragged.y);
			g_Camera.rotation.y += ToRadians(dragged.x);

		}
		else if (sv::input_mouse(SV_MOUSE_CENTER)) {

			if (dragged.Mag() != 0.f) {
				XMVECTOR lookAt = XMVectorSet(dragged.x, -dragged.y, 0.f, 0.f);
				lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation));
				g_Camera.position -= lookAt * 100.f;
			}
		}

		float scroll = 0.f;

		if (sv::input_key('W')) {
			scroll += dt;
		}
		if (sv::input_key('S')) {
			scroll -= dt;
		}

		if (scroll != 0.f) {
			XMVECTOR lookAt = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation));
			lookAt = XMVector3Normalize(lookAt);
			g_Camera.position += lookAt * scroll * 100.f;
		}

	}

	// MAIN FUNCTIONS

	sv::Result scene_editor_initialize()
	{
		// Create camera
		{
			sv::Scene* scene = simulation_scene_get();
			sv::ECS* ecs = sv::scene_ecs_get(scene);
			sv::Entity camera = sv::scene_camera_get(scene);

			sv::CameraComponent& mainCamera = *sv::ecs_component_get<sv::CameraComponent>(ecs, camera);
			sv::uvec2 res = { mainCamera.offscreen.GetWidth(), mainCamera.offscreen.GetHeight() };
			svCheck(sv::renderer_offscreen_create(res.x, res.y, g_Camera.offscreen));
			g_Camera.settings.projection = mainCamera.settings.projection;

			// TEMP:
			g_Camera.settings.projection.cameraType = sv::CameraType_Perspective;
			g_Camera.settings.projection.near = 0.01f;
			g_Camera.settings.projection.far = 100000.f;
			g_Camera.settings.projection.width = 0.01f;
			g_Camera.settings.projection.height = 0.01f;
		}
		return sv::Result_Success;
	}

	sv::Result scene_editor_close()
	{
		return sv::Result_Success;
	}

	void scene_editor_update(float dt)
	{
		// Adjust camera
		sv::uvec2 size = viewport_scene_editor_size();
		sv::renderer_projection_aspect_set(g_Camera.settings.projection, float(size.x) / float(size.y));
		
		// Camera Controller
		if (viewport_scene_editor_has_focus()) {
			if (g_Camera.settings.projection.cameraType == sv::CameraType_Orthographic) {
				scene_editor_camera_controller_2D(dt);
			}
			else if (g_Camera.settings.projection.cameraType == sv::CameraType_Perspective) {
				scene_editor_camera_controller_3D(dt);
			}
		}
	}

	void scene_editor_render()
	{
		if (!viewport_scene_editor_visible()) {
			return;
		}

		sv::CameraDrawDesc desc;
		desc.pOffscreen = &g_Camera.offscreen;
		desc.pSettings = &g_Camera.settings;
		desc.position = g_Camera.position;
		desc.rotation = g_Camera.rotation;

		sv::scene_renderer_camera_draw(simulation_scene_get() , &desc);
	}

	// GETTERS

	DebugCamera& scene_editor_camera_get()
	{
		return g_Camera;
	}

}