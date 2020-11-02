#include "core_editor.h"

#include "scene_editor.h"
#include "panel_manager.h"
#include "panels/SceneEditorPanel.h"
#include "platform/input.h"
#include "simulation/scene.h"
#include "simulation.h"
#include "rendering/debug_renderer.h"

namespace sv {

	static SceneEditorMode g_EditorMode = SceneEditorMode(ui32_max);

	static DebugCamera g_Camera;

	static RendererDebugBatch* g_Colliders2DBatch;

	// CAMERA CONTROLLERS

	void scene_editor_camera_controller_2D(float dt)
	{
		auto& cam = g_Camera.camera;
		float length = cam.getProjectionLength();

		// Movement
		if (input_mouse(SV_MOUSE_CENTER)) {
			vec2f direction = input_mouse_dragged_get();
			direction *= vec2f{ cam.getWidth(), cam.getHeight() } * 2.f;
			g_Camera.position += { -direction.x, direction.y, 0.f };
		}

		// Zoom
		float zoomForce = dt * length;

		if (input_key('S')) {
			length += zoomForce;
		}
		if (input_key('W')) {
			length -= zoomForce;
		}

		cam.setProjectionLength(length);

	}

	void scene_editor_camera_controller_3D(float dt)
	{
		vec2f dragged = input_mouse_dragged_get();
		
		if (input_mouse(SV_MOUSE_RIGHT)) {
			
			dragged *= 700.f;

			g_Camera.rotation.x += ToRadians(dragged.y);
			g_Camera.rotation.y += ToRadians(dragged.x);

		}
		else if (input_mouse(SV_MOUSE_CENTER)) {

			if (dragged.length() != 0.f) {
				auto& cam = g_Camera.camera;
				dragged *= { cam.getWidth(), cam.getHeight() };
				XMVECTOR lookAt = XMVectorSet(dragged.x, -dragged.y, 0.f, 0.f);
				lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
				g_Camera.position -= vec3f(lookAt) * 4000.f;
			}
		}

		float scroll = 0.f;

		if (input_key('W')) {
			scroll += dt;
		}
		if (input_key('S')) {
			scroll -= dt;
		}

		if (scroll != 0.f) {
			XMVECTOR lookAt = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			lookAt = XMVector3Transform(lookAt, XMMatrixRotationRollPitchYawFromVector(g_Camera.rotation.get_dx()));
			lookAt = XMVector3Normalize(lookAt);

			vec3f svlookAt = vec3f(lookAt);
			svlookAt *= scroll * 100.f;
			g_Camera.position += svlookAt;
		}

	}

	// MAIN FUNCTIONS
	Result scene_editor_initialize()
	{
		// Create camera
		{
			Scene& scene = simulation_scene_get();
			Entity camera = scene.getMainCamera();

			SV_ASSERT(ecs_entity_exist(scene, camera));
			Camera& mainCamera = ecs_component_get<CameraComponent>(scene, camera)->camera;
			vec2u res = { mainCamera.getResolutionWidth(), mainCamera.getResolutionHeight() };
			g_Camera.camera.setResolution(res.x, res.y);

			g_Camera.position = { 0.f, 0.f, -10.f };

			scene_editor_mode_set(SceneEditorMode_2D);

			svCheck(debug_renderer_batch_create(&g_Colliders2DBatch));
		}
		return Result_Success;
	}

	Result scene_editor_close()
	{
		g_Camera.camera.clear();
		svCheck(debug_renderer_batch_destroy(g_Colliders2DBatch));
		return Result_Success;
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
		SceneEditorPanel* vp = (SceneEditorPanel*) panel_manager_get("Scene Editor");
		if (vp == nullptr) return;

		// Adjust camera
		vec2u size = vp->get_screen_size();
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
		SceneEditorPanel* vp = (SceneEditorPanel*)panel_manager_get("Scene Editor");
		if (vp == nullptr) return;

		if (!vp->isVisible()) {
			return;
		}

		Scene& scene = simulation_scene_get();

		scene.drawCamera(&g_Camera.camera, g_Camera.position, g_Camera.rotation);

		// DEBUG RENDERING

		XMMATRIX viewProjectionMatrix = math_matrix_view(g_Camera.position, g_Camera.rotation) * g_Camera.camera.getProjectionMatrix();
		CommandList cmd = graphics_commandlist_get();

		// Draw 2D Colliders
		{
			debug_renderer_batch_reset(g_Colliders2DBatch);

			Scene& scene = simulation_scene_get();

			EntityView<RigidBody2DComponent> bodies(scene);

			debug_renderer_stroke_set(g_Colliders2DBatch, 0.05f);

			for (RigidBody2DComponent& body : bodies) {

				Transform trans = ecs_entity_transform_get(scene, body.entity);
				vec3f position = trans.getWorldPosition();
				vec3f scale = trans.getWorldScale();
				//TODO: vec3f rotation = trans.GetWorldRotation();
				vec4f rotation = trans.getLocalRotation();

				for (ui32 i = 0; i < body.collidersCount; ++i) {

					Collider2D& collider = body.colliders[i];

					switch (collider.type)
					{
					case Collider2DType_Box:
					{
						XMVECTOR colliderPosition = XMVectorSet(position.x, position.y, position.z, 0.f);
						XMVECTOR colliderScale = XMVectorSet(scale.x, scale.y, 0.f, 0.f);
						colliderScale = XMVectorMultiply(XMVectorSet(collider.box.size.x, collider.box.size.y, 0.f, 0.f), colliderScale);

						XMVECTOR offset = XMVectorSet(collider.offset.x, collider.offset.y, 0.f, 0.f);
						offset = XMVector3Transform(offset, XMMatrixRotationQuaternion(rotation.get_dx()));

						colliderPosition += offset;

						debug_renderer_draw_quad(g_Colliders2DBatch, vec3f(colliderPosition), vec2f(colliderScale), rotation, { 0u, 255u, 0u, 255u });
					}
						break;
					}

				}

			}

			debug_renderer_batch_render(g_Colliders2DBatch, g_Camera.camera.getOffscreenRT(), g_Camera.camera.getViewport(), g_Camera.camera.getScissor(), viewProjectionMatrix, cmd);
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

}