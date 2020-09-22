#include "core_editor.h"

#include "scene_editor.h"
#include "viewports/viewport_scene_editor.h"
#include "input.h"
#include "scene.h"
#include "simulation.h"

namespace sve {

	static SceneEditorMode g_EditorMode = SceneEditorMode(ui32_max);

	static DebugCamera g_Camera;

	static sv::RendererDebugBatch* g_Colliders2DBatch;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(float dt)
	{
		float zoom = sv::renderer_projection_zoom_get(g_Camera.settings.projection);

		// Movement
		if (sv::input_mouse(SV_MOUSE_CENTER)) {
			sv::vec2f direction = sv::input_mouse_dragged_get();
			direction *= sv::vec2f{ g_Camera.settings.projection.width, g_Camera.settings.projection.height } * 2.f;
			g_Camera.position += { -direction.x, direction.y, 0.f };
		}

		// Zoom
		float zoomForce = dt * zoom;

		if (sv::input_key('S')) {
			zoom += zoomForce;
		}
		if (sv::input_key('W')) {
			zoom -= zoomForce;
		}

		sv::renderer_projection_zoom_set(g_Camera.settings.projection, zoom);

	}

	void scene_editor_camera_controller_3D(float dt)
	{
		sv::vec2f dragged = sv::input_mouse_dragged_get();
		
		if (sv::input_mouse(SV_MOUSE_RIGHT)) {
			
			dragged *= 700.f;

			g_Camera.rotation.x += ToRadians(dragged.y);
			g_Camera.rotation.y += ToRadians(dragged.x);

		}
		else if (sv::input_mouse(SV_MOUSE_CENTER)) {

			if (dragged.length() != 0.f) {
				dragged *= { g_Camera.settings.projection.width, g_Camera.settings.projection.height };
				XMVECTOR lookAt = XMVectorSet(dragged.x, -dragged.y, 0.f, 0.f);
				lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
				g_Camera.position -= sv::vec3f(lookAt) * 4000.f;
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
			lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
			lookAt = XMVector3Normalize(lookAt);

			sv::vec3f svlookAt = sv::vec3f(lookAt);
			svlookAt *= scroll * 100.f;
			g_Camera.position += svlookAt;
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
			sv::vec2u res = { mainCamera.offscreen.GetWidth(), mainCamera.offscreen.GetHeight() };
			svCheck(sv::renderer_offscreen_create(res.x, res.y, g_Camera.offscreen));

			g_Camera.position = { 0.f, 0.f, -10.f };

			scene_editor_mode_set(SceneEditorMode_2D);

			svCheck(sv::renderer_debug_batch_create(&g_Colliders2DBatch));
		}
		return sv::Result_Success;
	}

	sv::Result scene_editor_close()
	{
		svCheck(sv::renderer_debug_batch_destroy(g_Colliders2DBatch));
		return sv::Result_Success;
	}

	void scene_editor_update_mode2D(float dt)
	{
		scene_editor_camera_controller_2D(dt);
	}

	void scene_editor_update_mode3D(float dt)
	{
		scene_editor_camera_controller_3D(dt);
	}

	void scene_editor_update(float dt)
	{
		// Adjust camera
		sv::vec2u size = viewport_scene_editor_size();
		if (size.x != 0u && size.y != 0u)
			sv::renderer_projection_aspect_set(g_Camera.settings.projection, float(size.x) / float(size.y));
		
		switch (g_EditorMode)
		{
		case sve::SceneEditorMode_2D:
			scene_editor_update_mode2D(dt);
			break;
		case sve::SceneEditorMode_3D:
			scene_editor_update_mode3D(dt);
			break;
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

		// DEBUG RENDERING

		XMMATRIX viewProjectionMatrix = sv::math_matrix_view(g_Camera.position, g_Camera.rotation) * sv::renderer_projection_matrix(g_Camera.settings.projection);
		sv::CommandList cmd = sv::graphics_commandlist_get();

		// Draw 2D Colliders
		{
			sv::renderer_debug_batch_reset(g_Colliders2DBatch);

			sv::ECS* ecs = sv::scene_ecs_get(simulation_scene_get());

			sv::EntityView<sv::RigidBody2DComponent> bodies(ecs);

			sv::renderer_debug_stroke_set(g_Colliders2DBatch, 0.05f);

			for (sv::RigidBody2DComponent& body : bodies) {

				sv::Transform trans = sv::ecs_entity_transform_get(ecs, body.entity);
				sv::vec3f position = trans.GetWorldPosition();
				sv::vec3f scale = trans.GetWorldScale();
				//TODO: sv::vec3f rotation = trans.GetWorldRotation();
				sv::vec3f rotation = trans.GetLocalRotation();

				for (ui32 i = 0; i < body.collidersCount; ++i) {

					sv::Collider2D& collider = body.colliders[i];

					switch (collider.type)
					{
					case sv::Collider2DType_Box:
					{
						XMVECTOR colliderPosition = XMVectorSet(position.x, position.y, position.z, 0.f);
						XMVECTOR colliderScale = XMVectorSet(scale.x, scale.y, 0.f, 0.f);
						colliderScale = XMVectorMultiply(XMVectorSet(collider.box.size.x, collider.box.size.y, 0.f, 0.f), colliderScale);

						XMVECTOR offset = XMVectorSet(collider.offset.x, collider.offset.y, 0.f, 0.f);
						offset = XMVector3Transform(offset, XMMatrixRotationZ(collider.box.angularOffset + rotation.z));

						colliderPosition += offset;

						sv::renderer_debug_draw_quad(g_Colliders2DBatch, sv::vec3f(colliderPosition), sv::vec2f(colliderScale), rotation, { 0u, 255u, 0u, 255u });
					}
						break;
					}

				}

			}

			sv::renderer_debug_batch_render(g_Colliders2DBatch, g_Camera.offscreen.renderTarget, g_Camera.offscreen.GetViewport(), g_Camera.offscreen.GetScissor(), viewProjectionMatrix, cmd);
		}
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
		case sve::SceneEditorMode_2D:
			g_Camera.settings.projection.cameraType = sv::CameraType_Orthographic;
			g_Camera.settings.projection.near = -100000.f;
			g_Camera.settings.projection.far = 100000.f;
			g_Camera.settings.projection.width = 20.f;
			g_Camera.settings.projection.height = 20.f;
			g_Camera.position.z = 0.f;
			g_Camera.rotation = sv::vec3f();
			break;
		case sve::SceneEditorMode_3D:
			g_Camera.settings.projection.cameraType = sv::CameraType_Perspective;
			g_Camera.settings.projection.near = 0.01f;
			g_Camera.settings.projection.far = 100000.f;
			g_Camera.settings.projection.width = 0.01f;
			g_Camera.settings.projection.height = 0.01f;
			g_Camera.position.z = -10.f;
			break;
		default:
			sv::console_log("Unknown editor mode");
			break;
		}
		
	}

}