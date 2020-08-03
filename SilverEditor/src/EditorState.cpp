#include "core.h"

#include "EditorState.h"
#include "editor.h"

namespace sve {

	EditorState::EditorState() : State()
	{
		m_Scene.Initialize();

		m_MainCamera = m_Scene.CreateEntity();
		m_Scene.AddComponent<sv::CameraComponent>(m_MainCamera, SV_REND_CAMERA_TYPE_ORTHOGRAPHIC);
		m_Scene.AddComponent<sv::NameComponent>(m_MainCamera, "Main Camera");
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		camComp->CreateOffscreen(1080, 720);
	}

	void EditorState::Load()
	{
	}

	void EditorState::Initialize()
	{

	}

	void EditorState::Update(float dt)
	{
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		
		// Resize offscreen if resized
		sv::uvec2 size = viewport_game_get_size();
		if (size.Mag() != 0u) {
			auto& offscreen = *camComp->GetOffscreen();
			if (offscreen.GetWidth() != size.x || offscreen.GetHeight() != size.y) {
				// TODO: use sv::graphics_image_resize()
				SV_ASSERT(_sv::renderer_offscreen_destroy(offscreen));
				SV_ASSERT(_sv::renderer_offscreen_create(size.x, size.y, offscreen));
			}

			// Adjust camera to viewport
			camComp->Adjust(size.x, size.y);
		}

	}

	void EditorState::FixedUpdate()
	{
	}

	void EditorState::Render()
	{
		sv::renderer_scene_begin();
		sv::renderer_scene_draw_scene(m_Scene);
		sv::renderer_scene_end();
	}

	void EditorState::Unload()
	{
	}

	void EditorState::Close()
	{
	}

}