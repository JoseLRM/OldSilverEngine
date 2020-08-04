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
		camComp->projection.Orthographic_SetZoom(5.f);
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
		camComp->Adjust(size.x, size.y);
	}

	void EditorState::FixedUpdate()
	{
	}

	void EditorState::Render()
	{
		sv::CameraComponent* camComp = m_Scene.GetComponent<sv::CameraComponent>(m_MainCamera);
		sv::Transform trans = m_Scene.GetTransform(camComp->entity);

		sv::renderer_scene_begin();
		sv::renderer_scene_draw_scene(m_Scene);
		sv::renderer_scene_set_camera(camComp->projection, trans.GetWorldMatrix());
		sv::renderer_scene_end();
	}

	void EditorState::Unload()
	{
	}

	void EditorState::Close()
	{
	}

}