#include "core_editor.h"

#include "RendererPanel.h"
#include "rendering/scene_renderer.h"
#include "gui.h"

namespace sv {

	RendererPanel::RendererPanel()
	{}

	bool RendererPanel::onDisplay()
	{
		u32 showLayer = m_RenderLayerSelected;

		if (m_RenderLayerSelected == u32_max) ImGui::Columns(1);
		else ImGui::Columns(2);

		if (ImGui::TreeNodeEx("RenderLayer 2D", ImGuiTreeNodeFlags_Framed)) {

			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer2D& rl = SceneRenderer::renderLayers2D[i];

				ImGui::MenuItem(rl.name.c_str());

				if (ImGui::IsItemClicked()) {
					m_RenderLayerSelected = i;
				}

			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNodeEx("RenderLayer 3D", ImGuiTreeNodeFlags_Framed)) {

			for (u32 i = 0u; i < RENDERLAYER_COUNT; ++i) {

				RenderLayer3D& rl = SceneRenderer::renderLayers3D[i];

				ImGui::MenuItem(rl.name.c_str());

				if (ImGui::IsItemClicked()) {
					m_RenderLayerSelected = i + RENDERLAYER_COUNT;
				}
			}

			ImGui::TreePop();
		}

		if (showLayer != u32_max) {

			ImGui::NextColumn();

			if (ImGui::BeginChild("RenderLayerEditor")) {

				if (m_RenderLayerSelected < RENDERLAYER_COUNT) {

					RenderLayer2D& rl = SceneRenderer::renderLayers2D[m_RenderLayerSelected];

					gui_component_item_begin();

					gui_component_item_next("Name");
					gui_component_item_string(rl.name);

					gui_component_item_next("Sort Value");
					ImGui::DragInt("##SortValue", &rl.sortValue);

					gui_component_item_next("Frustum Test");
					ImGui::Checkbox("##FrustumTest", &rl.frustumTest);

					gui_component_item_next("Light Mult");
					ImGui::DragFloat("##LightMult", &rl.lightMult, 0.05f, 0.f, float_max);

					gui_component_item_next("Ambient Mult");
					ImGui::DragFloat("##AmbientMult", &rl.ambientMult, 0.05f, 0.f, float_max);

					gui_component_item_end();

				}
				else {

					RenderLayer3D& rl = SceneRenderer::renderLayers3D[m_RenderLayerSelected - RENDERLAYER_COUNT];

					gui_component_item_begin();

					gui_component_item_next("Name");
					gui_component_item_string(rl.name);
					
					gui_component_item_end();

				}
				ImGui::EndChild();
			}
		}

		ImGui::Columns(1);

		// Debug info
		ImGui::Separator();

		u32 drawCalls = (u32) SV_PROFILER_SCALAR_GET("Draw Calls");
		
		ImGui::Text("Draw Calls: %u", drawCalls);

		return true;
	}

	void RendererPanel::onClose()
	{
	}
}