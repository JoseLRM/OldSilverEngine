#include "core_editor.h"

#include "SceneEditorViewport.h"
#include "editor.h"
#include "scene_editor.h"
#include "platform/input.h"

namespace sve {
	
	SceneEditorViewport::SceneEditorViewport() : Viewport("Scene Editor")
	{}

	void SceneEditorViewport::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
	}

	bool SceneEditorViewport::onDisplay()
	{
		ImVec2 size = ImGui::GetWindowContentRegionMax();
		size.x -= ImGui::GetWindowContentRegionMin().x;
		size.y -= ImGui::GetWindowContentRegionMin().y;
		m_ScreenSize = { ui32(size.x), ui32(size.y) };

		auto& camera = scene_editor_camera_get();
		sv::GPUImage* offscreen = camera.camera.getOffscreenRT();

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen), { size.x, size.y });

		// POPUP
		if (sv::input_key(SV_KEY_CONTROL)) {
			if (ImGui::BeginPopupContextWindow("ScenePopup", ImGuiMouseButton_Right)) {

				bool mode3D = scene_editor_mode_get() == SceneEditorMode_3D;
				ImGui::Checkbox("3D Mode", &mode3D);
				if ((scene_editor_mode_get() == SceneEditorMode_3D) != mode3D) {
					scene_editor_mode_set(mode3D ? SceneEditorMode_3D : SceneEditorMode_2D);
				}

				ImGui::EndPopup();
			}
		}

		return true;
	}

	void SceneEditorViewport::endDisplay()
	{
		ImGui::PopStyleVar(2u);
	}

	ImGuiWindowFlags SceneEditorViewport::getWindowFlags()
	{
		return ImGuiWindowFlags_NoScrollbar;
	}

}