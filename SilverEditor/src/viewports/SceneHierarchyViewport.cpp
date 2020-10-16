#include "core_editor.h"

#include "SceneHierarchyViewport.h"
#include "EntityInspectorViewport.h"
#include "viewport_manager.h"
#include "high_level/scene.h"
#include "simulation.h"

namespace sve {
	
	SceneHierarchyViewport::SceneHierarchyViewport() : Viewport("Scene Hierarchy")
	{
	}

	bool SceneHierarchyViewport::onDisplay()
	{
		m_Popup = false;
		sv::Scene& scene = simulation_scene_get();

		for (size_t i = 0; i < sv::ecs_entity_count(scene); ++i) {

			sv::Entity entity = sv::ecs_entity_get(scene, i);

			if (sv::ecs_entity_parent_get(scene, entity) == SV_ENTITY_NULL) {
				showEntity(scene, entity);
				i += sv::ecs_entity_childs_count(scene, entity);
			}

		}

		if (!sv::ecs_entity_exist(scene, m_SelectedEntity)) m_SelectedEntity = SV_ENTITY_NULL;

		if (!m_Popup) {

			if (ImGui::BeginPopupContextWindow("Hierarchy Popup", ImGuiMouseButton_Right)) {

				m_Popup = true;

				if (ImGui::Button("Create empty entity")) {
					sv::ecs_entity_create(scene);
				}

				if (ImGui::Button("Create sprite")) {
					sv::Entity entity = sv::ecs_entity_create(scene);
					sv::ecs_component_add<sv::SpriteComponent>(scene, entity);
				}

				ImGui::EndPopup();
			}
		}

		EntityInspectorViewport* vp = (EntityInspectorViewport*)viewport_get("EntityInspector");
		if (vp)
		{
			vp->setEntity(m_SelectedEntity);
		}

		return true;
	}

	void SceneHierarchyViewport::showEntity(sv::ECS* ecs, sv::Entity entity)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (sv::ecs_entity_childs_count(ecs, entity) == 0 ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);

		if (m_SelectedEntity == entity) {
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool active;
		std::string label;
		sv::NameComponent* nameComponent = (sv::NameComponent*) sv::ecs_component_get_by_id(ecs, entity, sv::NameComponent::ID);
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
					sv::ecs_entity_create(ecs, entity);
				}

				if (ImGui::Button("Duplicate")) {
					sv::ecs_entity_duplicate(ecs, entity);
					m_SelectedEntity = 0;
				}

				if (ImGui::Button("Destroy")) {
					sv::ecs_entity_destroy(ecs, entity);
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
			if (sv::ecs_entity_childs_count(ecs, entity) != 0) {

				sv::Entity const* childs;

				for (ui32 i = 0; i < sv::ecs_entity_childs_count(ecs, entity); ++i) {

					sv::ecs_entity_childs_get(ecs, entity, &childs);

					sv::Entity e = childs[i];
					i += sv::ecs_entity_childs_count(ecs, e);
					showEntity(ecs, e);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}
	}

}