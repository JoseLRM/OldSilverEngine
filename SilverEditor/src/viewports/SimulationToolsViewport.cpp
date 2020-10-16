#include "core_editor.h"

#include "SimulationToolsViewport.h"
#include "simulation.h"

namespace sve {

	SimulationToolsViewport::SimulationToolsViewport() : Viewport("SimulationTools")
	{
	}

	void SimulationToolsViewport::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.f);
	}

	bool SimulationToolsViewport::onDisplay()
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

	void SimulationToolsViewport::endDisplay()
	{
		ImGui::PopStyleVar();
	}
	ImGuiWindowFlags SimulationToolsViewport::getWindowFlags()
	{
		return ImGuiWindowFlags_NoDecoration;
	}
}