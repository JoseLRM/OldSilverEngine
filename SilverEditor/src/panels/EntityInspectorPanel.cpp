#include "core_editor.h"

#include "EntityInspectorPanel.h"
#include "simulation.h"
#include "editor.h"

#include "gui.h"

namespace sv {

	void showSpriteComponentInfo(SpriteComponent* comp)
	{
		gui_component_item_begin();

		gui_component_item_next("Color");
		gui_component_item_color_picker(comp->color);

		gui_component_item_next("TexCoord");
		ImGui::DragFloat4("##TexCoord", &comp->sprite.texCoord.x, 0.001f);

		gui_component_item_next("Texture");
		gui_component_item_texture(comp->sprite.texture);

		gui_component_item_next("Material");
		gui_component_item_material(comp->material);

		gui_component_item_end();

	}

	void showNameComponentInfo(NameComponent* comp)
	{
		gui_component_item_string(comp->name);
	}

	void showCameraComponentInfo(CameraComponent* comp)
	{
		ProjectionType projectionType = comp->camera.getProjectionType();
		Camera& cam = comp->camera;

		if (ImGui::BeginCombo("Projection", (projectionType == ProjectionType_Orthographic) ? "Orthographic" : "Perspective")) {

			if (projectionType == ProjectionType_Orthographic) {
				if (ImGui::Button("Perspective")) {
					cam.setProjectionType(ProjectionType_Perspective);
					cam.setWidth(1.f);
					cam.setHeight(1.f);
					cam.setNear(0.01f);
					cam.setFar(100000.f);
				}
			}
			else if (projectionType == ProjectionType_Orthographic) {
				if (ImGui::Button("Orthographic")) {
					cam.setProjectionType(ProjectionType_Orthographic);
					cam.setWidth(10.f);
					cam.setHeight(10.f);
					cam.setNear(-100000.f);
					cam.setFar(100000.f);
				}
			}

			ImGui::EndCombo();
		}

		float far = cam.getFar();
		float near = cam.getNear();
		float length = cam.getProjectionLength();

		if (ImGui::DragFloat("Near", &near, 0.01f)) {
			cam.setNear(near);
		}
		if (ImGui::DragFloat("Far", &far, 0.1f)) {
			cam.setFar(far);
		}
		if (ImGui::DragFloat("Length", &length, 0.01f)) {
			cam.setProjectionLength(length);
		}
	}

	void showRigidBody2DComponentInfo(RigidBody2DComponent* comp)
	{
		ImGui::DragFloat2("Velocity", &comp->velocity.x, 0.1f);
		ImGui::DragFloat("Angular Velocity", &comp->angularVelocity, 0.1f);
		ImGui::Checkbox("Dynamic", &comp->dynamic);
		ImGui::Checkbox("FixedRotation", &comp->fixedRotation);

		ImGui::Separator();

		ImGui::Text("Colliders");

		static ui32 selectedCollider = 0u;

		for (ui32 i = 0; i < comp->collidersCount; ++i) {

			if (ImGui::SmallButton(std::to_string(i).c_str())) {
				selectedCollider = i;
			}

			ImGui::SameLine();

		}

		if (ImGui::Button("Add"))
		{
			if (comp->collidersCount != 8u) {

				Collider2D collider = comp->colliders[comp->collidersCount];
				collider.box.size = { 1.f, 1.f };
				collider.box.angularOffset = 0.f;
				collider.type = Collider2DType_Box;

				comp->collidersCount++;

			}
		}

		if (selectedCollider < comp->collidersCount) {

			Collider2D& collider = comp->colliders[selectedCollider];

			ImGui::DragFloat("Density", &collider.density, 0.1f);
			ImGui::DragFloat("Friction", &collider.friction, 0.1f);
			ImGui::DragFloat("Restitution", &collider.restitution, 0.1f);
			ImGui::DragFloat2("Offset", &collider.offset.x, 0.01f);

			switch (collider.type)
			{
			case Collider2DType_Box:
				ImGui::Text("Box Collider");
				ImGui::DragFloat2("Size", &collider.box.size.x, 0.1f);
				ImGui::DragFloat("Angular Offset", &collider.box.angularOffset, 0.01f);
				break;

			case Collider2DType_Circle:
				ImGui::Text("Circle Collider");
				ImGui::DragFloat2("Radius", &collider.circle.radius, 0.1f);
				break;
			}

			if (collider.density < 0.f) collider.density = 0.f;
		}
	}

	void showComponentInfo(CompID ID, BaseComponent* comp)
	{
		if (ID == SpriteComponent::ID)				showSpriteComponentInfo(reinterpret_cast<SpriteComponent*>(comp));
		else if (ID == NameComponent::ID)			showNameComponentInfo(reinterpret_cast<NameComponent*>(comp));
		else if (ID == CameraComponent::ID)			showCameraComponentInfo(reinterpret_cast<CameraComponent*>(comp));
		else if (ID == RigidBody2DComponent::ID)	showRigidBody2DComponentInfo(reinterpret_cast<RigidBody2DComponent*>(comp));
	}

	EntityInspectorViewport::EntityInspectorViewport() : Panel("Entity Inspector")
	{

	}

	void EntityInspectorViewport::setEntity(Entity entity)
	{
		m_Entity = entity;
	}

	bool EntityInspectorViewport::onDisplay()
	{
		ECS* ecs = simulation_scene_get().getECS();

		if (m_Entity != SV_ENTITY_NULL) {

			ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

			NameComponent* nameComponent = ecs_component_get<NameComponent>(ecs, m_Entity);
			if (nameComponent) {
				ImGui::Text(nameComponent->name.c_str(), nameComponent->name.c_str());
			}
			else {
				ImGui::Text("Entity");
			}

			ImGui::PopFont();

			ImGui::Separator();


			// Show Transform Data
			gui_transform_show(ecs_entity_transform_get(ecs, m_Entity));

			// Show components
			for (ui32 i = 0; i < ecs_entity_component_count(ecs, m_Entity); ++i) {

				auto [compID, comp] = ecs_component_get_by_index(ecs, m_Entity, i);
				gui_component_show(ecs, compID, comp, showComponentInfo);
				
			}


			if (ImGui::BeginCombo("##Add", "Add Component")) {

				for (ui16 ID = 0; ID < ecs_register_count(ecs); ++ID) {
					const char* NAME = ecs_register_nameof(ecs, ID);
					if (ecs_component_get_by_id(ecs, m_Entity, ID) != nullptr) continue;
					size_t SIZE = ecs_register_sizeof(ecs, ID);
					if (ImGui::Button(NAME)) {
						ecs_component_add_by_id(ecs, m_Entity, ID);
					}
				}

				ImGui::EndCombo();
			}
			if (ImGui::BeginCombo("##Rmv", "Remove Component")) {

				for (ui32 i = 0; i < ecs_entity_component_count(ecs, m_Entity); ++i) {
					CompID ID = ecs_component_get_by_index(ecs, m_Entity, i).first;
					const char* NAME = ecs_register_nameof(ecs, ID);
					size_t SIZE = ecs_register_sizeof(ecs, ID);
					if (ImGui::Button(NAME)) {
						ecs_component_remove_by_id(ecs, m_Entity, ID);
						break;
					}
				}

				ImGui::EndCombo();
			}

		}

		return true;
	}

}