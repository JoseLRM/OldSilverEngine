#include "core_editor.h"

#include "scene_editor.h"
#include "viewport_manager.h"
#include "viewports/SceneEditorViewport.h"
#include "platform/input.h"
#include "high_level/scene.h"
#include "simulation.h"
#include "rendering/debug_renderer.h"

namespace sve {

	static SceneEditorMode g_EditorMode = SceneEditorMode(ui32_max);

	static DebugCamera g_Camera;

	static sv::RendererDebugBatch* g_Colliders2DBatch;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(float dt)
	{
		auto& cam = g_Camera.camera;
		float length = cam.getProjectionLength();

		// Movement
		if (sv::input_mouse(SV_MOUSE_CENTER)) {
			sv::vec2f direction = sv::input_mouse_dragged_get();
			direction *= sv::vec2f{ cam.getWidth(), cam.getHeight() } * 2.f;
			g_Camera.position += { -direction.x, direction.y, 0.f };
		}

		// Zoom
		float zoomForce = dt * length;

		if (sv::input_key('S')) {
			length += zoomForce;
		}
		if (sv::input_key('W')) {
			length -= zoomForce;
		}

		cam.setProjectionLength(length);

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
				auto& cam = g_Camera.camera;
				dragged *= { cam.getWidth(), cam.getHeight() };
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
			sv::Scene& scene = simulation_scene_get();
			sv::Entity camera = scene.getMainCamera();

			sv::Camera& mainCamera = sv::ecs_component_get<sv::CameraComponent>(scene, camera)->camera;
			sv::vec2u res = { mainCamera.getResolutionWidth(), mainCamera.getResolutionHeight() };
			g_Camera.camera.setResolution(res.x, res.y);

			g_Camera.position = { 0.f, 0.f, -10.f };

			scene_editor_mode_set(SceneEditorMode_2D);

			svCheck(sv::debug_renderer_batch_create(&g_Colliders2DBatch));
		}
		return sv::Result_Success;
	}

	sv::Result scene_editor_close()
	{
		g_Camera.camera.clear();
		svCheck(sv::debug_renderer_batch_destroy(g_Colliders2DBatch));
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
		SceneEditorViewport* vp = (SceneEditorViewport*) viewport_get("SceneEditor");
		if (vp == nullptr) return;

		// Adjust camera
		sv::vec2u size = vp->get_screen_size();
		if (size.x != 0u && size.y != 0u)
			g_Camera.camera.adjust(size.x, size.y);
		
		if (vp->isFocused()) {
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
	}

	void scene_editor_render()
	{
		SceneEditorViewport* vp = (SceneEditorViewport*)viewport_get("SceneEditor");
		if (vp == nullptr) return;

		if (!vp->isVisible()) {
			return;
		}

		sv::Scene& scene = simulation_scene_get();
		scene.drawCamera(&g_Camera.camera, g_Camera.position, g_Camera.rotation);

		// DEBUG RENDERING

		XMMATRIX viewProjectionMatrix = sv::math_matrix_view(g_Camera.position, g_Camera.rotation) * g_Camera.camera.getProjectionMatrix();
		sv::CommandList cmd = sv::graphics_commandlist_get();

		// Draw 2D Colliders
		{
			sv::debug_renderer_batch_reset(g_Colliders2DBatch);

			sv::Scene& scene = simulation_scene_get();

			sv::EntityView<sv::RigidBody2DComponent> bodies(scene);

			sv::debug_renderer_stroke_set(g_Colliders2DBatch, 0.05f);

			for (sv::RigidBody2DComponent& body : bodies) {

				sv::Transform trans = sv::ecs_entity_transform_get(scene, body.entity);
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

						sv::debug_renderer_draw_quad(g_Colliders2DBatch, sv::vec3f(colliderPosition), sv::vec2f(colliderScale), rotation, { 0u, 255u, 0u, 255u });
					}
						break;
					}

				}

			}

			sv::debug_renderer_batch_render(g_Colliders2DBatch, g_Camera.camera.getOffscreenRT(), g_Camera.camera.getViewport(), g_Camera.camera.getScissor(), viewProjectionMatrix, cmd);
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
			g_Camera.camera.setProjectionType(sv::ProjectionType_Orthographic);
			//g_Camera.settings.projection.cameraType = sv::CameraType_Orthographic;
			//g_Camera.settings.projection.near = -100000.f;
			//g_Camera.settings.projection.far = 100000.f;
			//g_Camera.settings.projection.width = 20.f;
			//g_Camera.settings.projection.height = 20.f;
			g_Camera.position.z = 0.f;
			g_Camera.rotation = sv::vec3f();
			break;
		case sve::SceneEditorMode_3D:
			g_Camera.camera.setProjectionType(sv::ProjectionType_Perspective);
			//g_Camera.settings.projection.cameraType = sv::CameraType_Perspective;
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

}