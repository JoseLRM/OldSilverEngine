#include "core_editor.h"

#include "engine.h"
#include "editor.h"
#include "EditorState.h"

namespace sve {

	EditorState::EditorState() : State()
	{
	}

	void EditorState::Load()
	{
	}

	void EditorState::Initialize()
	{
		m_Scene = sv::scene_create();
		sv::scene_bind(m_Scene);

		sv::Entity mainCamera = sv::scene_ecs_entity_create();
		sv::scene_ecs_component_add<sv::CameraComponent>(mainCamera, sv::CameraType_Orthographic);
		sv::scene_ecs_component_add<sv::NameComponent>(mainCamera, "Main Camera");
		sv::CameraComponent* camComp = sv::scene_ecs_component_get<sv::CameraComponent>(mainCamera);
		camComp->projection.orthographic.width = 10.f;
		camComp->projection.orthographic.height = 10.f;
		sv::renderer_compute_orthographic_zoom_set(camComp->projection, 5.f);

		sv::scene_camera_set(mainCamera);

		// Create debug camera
		SV_ASSERT(m_DebugCamera.camera.CreateOffscreen(sv::renderer_resolution_get().x, sv::renderer_resolution_get().y));
		m_DebugCamera.camera.projection = camComp->projection;
	}

	void EditorState::Update(float dt)
	{
		sv::Entity mainCamera = sv::scene_camera_get();
		sv::CameraComponent* camComp = sv::scene_ecs_component_get<sv::CameraComponent>(mainCamera);
		
		// Adjust cameras
		{
			auto props = viewport_manager_properties_get("Game");
			sv::uvec2 size = { props.width, props.height };
			camComp->Adjust(size.x, size.y);
		}

		auto props = viewport_manager_properties_get("Scene Editor");
		{
			sv::uvec2 size = { props.width, props.height };
			m_DebugCamera.camera.Adjust(size.x, size.y);
		}

		// Debug camera controller
		float zoom = sv::renderer_compute_orthographic_zoom_get(m_DebugCamera.camera.projection);
		float force = 15.f * (zoom * 0.05f) * dt;
		float zoomForce = 20.f * dt * (zoom * 0.1f);

		if (props.focus) {
			if (sv::input_key('W')) {
				m_DebugCamera.position.y += force;
			}
			if (sv::input_key('S')) {
				m_DebugCamera.position.y -= force;
			}
			if (sv::input_key('A')) {
				m_DebugCamera.position.x -= force;
			}
			if (sv::input_key('D')) {
				m_DebugCamera.position.x += force;
			}

			if (sv::input_key(SV_KEY_SPACE)) {
				zoom += zoomForce;
			}
			if (sv::input_key(SV_KEY_SHIFT)) {
				zoom -= zoomForce;
			}

			sv::renderer_compute_orthographic_zoom_set(m_DebugCamera.camera.projection, zoom);
		}
	}

	void EditorState::FixedUpdate()
	{
	}

	void EditorState::Render()
	{
		sv::renderer_scene_begin();
		sv::renderer_scene_draw_scene();
		sv::renderer_scene_end();

		m_DebugCamera.viewMatrix = XMMatrixTranslation(-m_DebugCamera.position.x, -m_DebugCamera.position.y, -m_DebugCamera.position.z);
		sv::renderer_present(m_DebugCamera.camera.projection, m_DebugCamera.viewMatrix, m_DebugCamera.camera.GetOffscreen());
	}

	void EditorState::Unload()
	{
	}

	void EditorState::Close()
	{
		sv::scene_destroy(m_Scene);
	}

}