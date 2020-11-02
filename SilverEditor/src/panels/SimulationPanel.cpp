#include "core_editor.h"

#include "SimulationPanel.h"
#include "editor.h"
#include "simulation/scene.h"
#include "simulation.h"

namespace sv {
	SimulationPanel::SimulationPanel()
	{
	}

	void SimulationPanel::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
	}

	bool SimulationPanel::onDisplay()
	{
		ImVec2 size = ImGui::GetWindowContentRegionMax();
		size.x -= ImGui::GetWindowContentRegionMin().x;
		size.y -= ImGui::GetWindowContentRegionMin().y;
		m_ScreenSize = { ui32(size.x), ui32(size.y) };

		Scene& scene = simulation_scene_get();
		Entity cameraEntity = scene.getMainCamera();

		if (ecs_entity_exist(scene, cameraEntity)) {

			CameraComponent* camera = ecs_component_get<CameraComponent>(scene, cameraEntity);
			if (camera) {

				if (camera->camera.isActive()) {
					GPUImage* offscreen = camera->camera.getOffscreenRT();
					ImGuiDevice& device = editor_device_get();

					ImGui::Image(device.ParseImage(offscreen), { size.x, size.y });
				}
			}
		}

		return true;
	}

	void SimulationPanel::endDisplay()
	{
		ImGui::PopStyleVar(2);
	}

	ImGuiWindowFlags SimulationPanel::getWindowFlags()
	{
		return ImGuiWindowFlags_NoScrollbar;
	}
}