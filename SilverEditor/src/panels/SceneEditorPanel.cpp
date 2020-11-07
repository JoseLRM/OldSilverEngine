#include "core_editor.h"

#include "SceneEditorPanel.h"
#include "editor.h"
#include "scene_editor.h"
#include "platform/input.h"

namespace sv {
	
	SceneEditorPanel::SceneEditorPanel()
	{}

	void SceneEditorPanel::adjustCoord(vec2f& coord) const noexcept
	{
		vec2f winSize = vec2f(float(window_size_get().x), float(window_size_get().y));
		vec2f screenPos = vec2f{ float(m_ScreenPos.x), float(m_ScreenPos.y) };

		coord += { 0.5f, 0.5f };
		coord *= winSize;

		coord -= screenPos;
		coord /= vec2f{ float(m_ScreenSize.x), float(m_ScreenSize.y) };;
		coord -= { 0.5f, 0.5f };
	}

	void SceneEditorPanel::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
	}

	bool SceneEditorPanel::onDisplay()
	{
		ImVec2 size = ImGui::GetWindowContentRegionMax();
		size.x -= ImGui::GetWindowContentRegionMin().x;
		size.y -= ImGui::GetWindowContentRegionMin().y;
		m_ScreenSize = { ui32(size.x), ui32(size.y) };

		auto& camera = scene_editor_camera_get();
		GPUImage* offscreen = camera.camera.getOffscreenRT();

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen), { size.x, size.y });

		// POPUP
		if (input_key(SV_KEY_CONTROL)) {
			if (ImGui::BeginPopupContextWindow("ScenePopup", ImGuiMouseButton_Right)) {

				bool mode3D = scene_editor_mode_get() == SceneEditorMode_3D;
				ImGui::Checkbox("3D Mode", &mode3D);
				if ((scene_editor_mode_get() == SceneEditorMode_3D) != mode3D) {
					scene_editor_mode_set(mode3D ? SceneEditorMode_3D : SceneEditorMode_2D);
				}

				ImGui::EndPopup();
			}
		}

		ImVec2 pos = ImGui::GetItemRectMin();
		m_ScreenPos.x = pos.x;
		m_ScreenPos.y = pos.y;

		return true;
	}

	void SceneEditorPanel::endDisplay()
	{
		ImGui::PopStyleVar(2u);
	}

	ImGuiWindowFlags SceneEditorPanel::getWindowFlags()
	{
		return ImGuiWindowFlags_NoScrollbar;
	}

}