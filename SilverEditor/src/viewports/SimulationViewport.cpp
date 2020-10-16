#include "core_editor.h"

#include "SimulationViewport.h"
#include "editor.h"
#include "high_level/scene.h"
#include "simulation.h"

namespace sve {
	SimulationViewport::SimulationViewport() : Viewport("Simulation")
	{
	}

	void SimulationViewport::beginDisplay()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
	}

	bool SimulationViewport::onDisplay()
	{
		ImVec2 size = ImGui::GetWindowContentRegionMax();
		size.x -= ImGui::GetWindowContentRegionMin().x;
		size.y -= ImGui::GetWindowContentRegionMin().y;
		m_ScreenSize = { ui32(size.x), ui32(size.y) };

		sv::Scene& scene = simulation_scene_get();
		sv::Entity cameraEntity = scene.getMainCamera();

		if (sv::ecs_entity_exist(scene, cameraEntity)) {

			sv::CameraComponent* camera = sv::ecs_component_get<sv::CameraComponent>(scene, cameraEntity);
			if (camera) {

				if (camera->camera.isActive()) {
					sv::GPUImage* offscreen = camera->camera.getOffscreenRT();
					ImGuiDevice& device = editor_device_get();

					ImGui::Image(device.ParseImage(offscreen), { size.x, size.y });
				}
			}
		}

		return true;
	}

	void SimulationViewport::endDisplay()
	{
		ImGui::PopStyleVar(2);
	}

	ImGuiWindowFlags SimulationViewport::getWindowFlags()
	{
		return ImGuiWindowFlags_NoScrollbar;
	}
}