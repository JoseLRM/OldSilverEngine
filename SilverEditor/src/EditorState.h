#pragma once

#include "core_editor.h"
#include "components.h"
#include "engine.h"

namespace sve {

	struct DebugCamera {
		sv::CameraComponent camera;
		XMMATRIX viewMatrix;
		sv::vec3 position;
		sv::vec2 rotation;
	};

	class EditorState : public sv::State {
		sv::Scene m_Scene;
		DebugCamera m_DebugCamera;

	public:
		EditorState();

		void Load() override;
		void Initialize() override;
		void Update(float dt) override;
		void FixedUpdate() override;
		void Render() override;
		void Unload() override;
		void Close() override;

		inline sv::Scene& GetScene() noexcept { return m_Scene; }
		inline sv::Entity GetMainCamera() const noexcept { return sv::scene_camera_get(); }
		inline DebugCamera& GetDebugCamera() noexcept { return m_DebugCamera; }

	};

}