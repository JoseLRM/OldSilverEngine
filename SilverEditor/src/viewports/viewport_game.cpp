#include "core.h"

#include "viewport_manager.h"
#include "EditorState.h"
#include "editor.h"

namespace sve {

	bool viewport_game_display()
	{
		EditorState& state = editor_state_get();

		sv::Scene& scene = state.GetScene();
		ImVec2 v = ImGui::GetWindowSize();
		auto& offscreen = _sv::renderer_offscreen_get();

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		return true;
	}

}