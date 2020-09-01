#include "core_editor.h"

#include "editor.h"
#include "simulation.h"
#include "viewports/viewport_simulation.h"
#include "viewports.h"

namespace sve {

	static bool g_Visible = false;
	static sv::uvec2 g_Size;

	bool viewport_simulation_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SIMULATION))) {
			
			g_Visible = true;
			ImVec2 size = ImGui::GetWindowSize();
			g_Size = { ui32(size.x), ui32(size.y) };

			ImVec2 v = ImGui::GetWindowSize();
			auto& offscreen = sv::renderer_offscreen_get();

			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		}
		else g_Visible = false;
		ImGui::End();

		return true;
	}

	bool viewport_simulation_visible()
	{
		return g_Visible;
	}

	sv::uvec2 viewport_simulation_size()
	{
		return g_Size;
	}

}