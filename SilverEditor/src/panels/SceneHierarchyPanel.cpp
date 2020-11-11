#include "core_editor.h"

#include "SceneHierarchyPanel.h"
#include "panel_manager.h"
#include "simulation/scene.h"
#include "simulation.h"

namespace sv {
	
	SceneHierarchyPanel::SceneHierarchyPanel()
	{
	}

	bool SceneHierarchyPanel::onDisplay()
	{
		m_Popup = false;

		for (ui32 i = 0; i < ecs_entity_count(g_Scene); ++i) {

			Entity entity = ecs_entity_get(g_Scene, i);

			if (ecs_entity_parent_get(g_Scene, entity) == SV_ENTITY_NULL) {
				showEntity(g_Scene, entity);
				i += ecs_entity_childs_count(g_Scene, entity);
			}

		}


		if (!m_Popup) {

			if (ImGui::BeginPopupContextWindow("Hierarchy Popup", ImGuiMouseButton_Right)) {

				m_Popup = true;

				if (ImGui::MenuItem("Create Empty Entity")) {
					ecs_entity_create(g_Scene);
				}

				if (ImGui::MenuItem("Create Camera")) {
					Entity entity = ecs_entity_create(g_Scene);
					ecs_component_add<CameraComponent>(g_Scene, entity)->camera.setResolution(1080u, 720u);
					ecs_component_add<NameComponent>(g_Scene, entity, "Camera");
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("2D")) {

					if (ImGui::MenuItem("Create Sprite")) {
						Entity entity = ecs_entity_create(g_Scene);
						ecs_component_add<SpriteComponent>(g_Scene, entity);
						ecs_component_add<NameComponent>(g_Scene, entity, "Sprite");
					}

					if (ImGui::MenuItem("Create Animated Sprite")) {
						Entity entity = ecs_entity_create(g_Scene);
						ecs_component_add<AnimatedSpriteComponent>(g_Scene, entity);
						ecs_component_add<NameComponent>(g_Scene, entity, "Animated Sprite");
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("3D")) {

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		return true;
	}

	void SceneHierarchyPanel::showEntity(ECS* ecs, Entity entity)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | (ecs_entity_childs_count(ecs, entity) == 0 ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);

		if (g_SelectedEntity == entity) {
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool active;
		std::string label;
		NameComponent* nameComponent = (NameComponent*) ecs_component_get_by_id(ecs, entity, NameComponent::ID);
		if (nameComponent) {
			label = nameComponent->name + "[" + std::to_string(entity) + "]";
		}
		else {
			label = "Entity[" + std::to_string(entity) + "]";
		}

		active = ImGui::TreeNodeEx(label.c_str(), treeFlags);

		if (!m_Popup) {
			if (ImGui::BeginPopupContextItem(label.c_str(), ImGuiMouseButton_Right)) {

				m_Popup = true;

				if (ImGui::Button("Create child")) {
					ecs_entity_create(ecs, entity);
				}

				if (ImGui::Button("Duplicate")) {
					ecs_entity_duplicate(ecs, entity);
					g_SelectedEntity = SV_ENTITY_NULL;
				}

				if (ImGui::Button("Destroy")) {
					ecs_entity_destroy(ecs, entity);
					g_SelectedEntity = SV_ENTITY_NULL;
				}

				ImGui::EndPopup();
			}
		}

		if (ImGui::IsItemClicked()) {
			g_SelectedEntity = entity;
		}
		if (active) {
			if (ecs_entity_childs_count(ecs, entity) != 0) {

				Entity const* childs;

				for (ui32 i = 0; i < ecs_entity_childs_count(ecs, entity); ++i) {

					ecs_entity_childs_get(ecs, entity, &childs);

					Entity e = childs[i];
					i += ecs_entity_childs_count(ecs, e);
					showEntity(ecs, e);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}
	}

}