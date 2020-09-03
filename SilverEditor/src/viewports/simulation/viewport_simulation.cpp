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
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar;
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SIMULATION), 0, windowFlags)) {
			
			g_Visible = true;
			ImVec2 size = ImGui::GetWindowContentRegionMax();
			size.x -= ImGui::GetWindowContentRegionMin().x;
			size.y -= ImGui::GetWindowContentRegionMin().y;
			g_Size = { ui32(size.x), ui32(size.y) };

			auto& offscreen = sv::renderer_offscreen_get();

			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(offscreen.renderTarget), { size.x, size.y });

		}
		else g_Visible = false;
		ImGui::End();

		ImGui::PopStyleVar(2);

		return true;
	}

	void viewport_simulationtools_display()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.f);

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration;
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SIMULATION_TOOLS), 0, windowFlags)) {

			if (ImGui::Button(simulation_running() ? "Stop" : "Start")) {
				if (simulation_running()) {
					simulation_stop();
				}
				else simulation_run();
			}


			if (simulation_running()) {

				ImGui::SameLine();

				if (ImGui::Button(simulation_paused() ? "Continue" : "Pause")) {
					if (simulation_paused()) {
						simulation_continue();
					}
					else {
						simulation_pause();
					}
				}

			}

		}
		ImGui::End();

		ImGui::PopStyleVar();
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