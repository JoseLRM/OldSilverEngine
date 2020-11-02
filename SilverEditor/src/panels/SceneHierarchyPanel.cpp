#include "core_editor.h"

#include "SceneHierarchyPanel.h"
#include "EntityInspectorPanel.h"
#include "panel_manager.h"
#include "simulation/scene.h"
#include "simulation.h"

namespace sv {
	
	SceneHierarchyViewport::SceneHierarchyViewport()
	{
	}

	bool SceneHierarchyViewport::onDisplay()
	{
		m_Popup = false;
		Scene& scene = simulation_scene_get();

		for (ui32 i = 0; i < ecs_entity_count(scene); ++i) {

			Entity entity = ecs_entity_get(scene, i);

			if (ecs_entity_parent_get(scene, entity) == SV_ENTITY_NULL) {
				showEntity(scene, entity);
				i += ecs_entity_childs_count(scene, entity);
			}

		}

		if (!ecs_entity_exist(scene, m_SelectedEntity)) m_SelectedEntity = SV_ENTITY_NULL;

		if (!m_Popup) {

			if (ImGui::BeginPopupContextWindow("Hierarchy Popup", ImGuiMouseButton_Right)) {

				m_Popup = true;

				if (ImGui::Button("Create empty entity")) {
					ecs_entity_create(scene);
				}

				if (ImGui::Button("Create sprite")) {
					Entity entity = ecs_entity_create(scene);
					ecs_component_add<SpriteComponent>(scene, entity);
				}

				ImGui::EndPopup();
			}
		}

		EntityInspectorViewport* vp = (EntityInspectorViewport*)panel_manager_get("EntityInspector");
		if (vp)
		{
			vp->setEntity(m_SelectedEntity);
		}

		return true;
	}

	void SceneHierarchyViewport::showEntity(ECS* ecs, Entity entity)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (ecs_entity_childs_count(ecs, entity) == 0 ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);

		if (m_SelectedEntity == entity) {
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
					m_SelectedEntity = 0;
				}

				if (ImGui::Button("Destroy")) {
					ecs_entity_destroy(ecs, entity);
					m_SelectedEntity = 0;
				}

				ImGui::EndPopup();
			}
		}

		if (ImGui::IsItemClicked()) {
			if (m_SelectedEntity != entity) {
				m_SelectedEntity = entity;
			}
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