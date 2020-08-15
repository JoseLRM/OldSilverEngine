#include "core_editor.h"

#include "viewport_manager.h"
#include "editor.h"
#include "simulation.h"
#include "components.h"

namespace sve {

	bool viewport_game_display()
	{
		ImVec2 v = ImGui::GetWindowSize();
		auto& offscreen = sv::renderer_offscreen_get();

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		return true;
	}

}