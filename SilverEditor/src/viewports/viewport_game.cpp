#include "core.h"

#include "viewport_manager.h"
#include "EditorState.h"
#include "editor.h"

namespace sve {

	static sv::uvec2 g_ViewportSize = { 0u, 0u };

	bool viewport_game_display()
	{
		EditorState& state = editor_state_get();

		sv::Scene& scene = state.GetScene();
		ImVec2 v = ImGui::GetWindowSize();
		auto& offscreen = _sv::renderer_offscreen_get();

		g_ViewportSize.x = ui32(v.x);
		g_ViewportSize.y = ui32(v.y);

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		return true;
	}

	sv::uvec2 viewport_game_get_size()
	{
		return g_ViewportSize;
	}
}