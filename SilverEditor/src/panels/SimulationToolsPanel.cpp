#include "core_editor.h"

#include "SimulationToolsPanel.h"
#include "simulation.h"

namespace sv {

	SimulationToolsPanel::SimulationToolsPanel()
	{
	}

	void SimulationToolsPanel::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.f);
	}

	bool SimulationToolsPanel::onDisplay()
	{
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
		
		return true;
	}

	void SimulationToolsPanel::endDisplay()
	{
		ImGui::PopStyleVar();
	}
	ImGuiWindowFlags SimulationToolsPanel::getWindowFlags()
	{
		return ImGuiWindowFlags_NoDecoration;
	}
}