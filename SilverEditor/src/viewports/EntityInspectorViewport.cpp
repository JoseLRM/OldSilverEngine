#include "core_editor.h"

#include "EntityInspectorViewport.h"
#include "simulation.h"
#include "editor.h"

namespace sve {

	EntityInspectorViewport::EntityInspectorViewport() : Viewport("Entity Inspector")
	{

	}

	void EntityInspectorViewport::setEntity(sv::Entity entity)
	{
		m_Entity = entity;
	}

	bool EntityInspectorViewport::onDisplay()
	{
		sv::ECS* ecs = simulation_scene_get().getECS();

		if (m_Entity != SV_ENTITY_NULL) {

			sv::NameComponent* nameComponent = sv::ecs_component_get<sv::NameComponent>(ecs, m_Entity);
			if (nameComponent) {
				ImGui::Text("%s[%u]", nameComponent->name.c_str(), m_Entity);
			}
			else {
				ImGui::Text("Entity[%u]", m_Entity);
			}

			ImGui::Separator();

			// Show Transform Data
			sv::Transform trans = sv::ecs_entity_transform_get(ecs, m_Entity);

			sv::vec3f position = trans.GetLocalPosition();
			sv::vec3f rotation = ToDegrees(trans.GetLocalRotation());
			sv::vec3f scale = trans.GetLocalScale();

			ImGui::DragFloat3("Position", &position.x, 0.3f);
			ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
			ImGui::DragFloat3("Scale", &scale.x, 0.01f);

			trans.SetPosition(position);
			trans.SetRotation(ToRadians(rotation));
			trans.SetScale(scale);

			ImGui::Separator();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

			for (ui32 i = 0; i < sv::ecs_entity_component_count(ecs, m_Entity); ++i) {

				auto [compID, comp] = sv::ecs_component_get_by_index(ecs, m_Entity, i);

				ImGui::Separator();

				if (ImGui::TreeNodeEx(sv::ecs_register_nameof(ecs, compID), flags)) {
					showComponentInfo(compID, comp);
					ImGui::TreePop();
				}

				ImGui::Separator();
			}


			if (ImGui::BeginCombo("Add", "Add Component")) {

				for (ui16 ID = 0; ID < sv::ecs_register_count(ecs); ++ID) {
					const char* NAME = sv::ecs_register_nameof(ecs, ID);
					if (sv::ecs_component_get_by_id(ecs, m_Entity, ID) != nullptr) continue;
					size_t SIZE = sv::ecs_register_sizeof(ecs, ID);
					if (ImGui::Button(NAME)) {
						sv::ecs_component_add_by_id(ecs, m_Entity, ID);
					}
				}

				ImGui::EndCombo();
			}
			if (ImGui::BeginCombo("Rmv", "Remove Component")) {

				for (ui32 i = 0; i < sv::ecs_entity_component_count(ecs, m_Entity); ++i) {
					sv::CompID ID = sv::ecs_component_get_by_index(ecs, m_Entity, i).first;
					const char* NAME = sv::ecs_register_nameof(ecs, ID);
					size_t SIZE = sv::ecs_register_sizeof(ecs, ID);
					if (ImGui::Button(NAME)) {
						sv::ecs_component_remove_by_id(ecs, m_Entity, ID);
						break;
					}
				}

				ImGui::EndCombo();
			}

		}

		return true;
	}

	void EntityInspectorViewport::showComponentInfo(sv::CompID ID, sv::BaseComponent* comp)
	{
		if (ID == sv::SpriteComponent::ID)				showSpriteComponentInfo(reinterpret_cast<sv::SpriteComponent*>(comp));
		else if (ID == sv::NameComponent::ID)			showNameComponentInfo(reinterpret_cast<sv::NameComponent*>(comp));
		else if (ID == sv::CameraComponent::ID)			showCameraComponentInfo(reinterpret_cast<sv::CameraComponent*>(comp));
		else if (ID == sv::RigidBody2DComponent::ID)	showRigidBody2DComponentInfo(reinterpret_cast<sv::RigidBody2DComponent*>(comp));
	}

	void EntityInspectorViewport::showSpriteComponentInfo(sv::SpriteComponent* comp)
	{
		sv::Color4f col = { float(comp->color.r) / 255.f, float(comp->color.g) / 255.f, float(comp->color.b) / 255.f, float(comp->color.a) / 255.f };
		ImGui::ColorEdit4("SpriteColor", &col.r);
		comp->color = { ui8(col.r * 255.f), ui8(col.g * 255.f) , ui8(col.b * 255.f) , ui8(col.a * 255.f) };

		ImGui::DragFloat4("TexCoord", &comp->sprite.texCoord.x, 0.001f);

		ImGui::Separator();

		// Sprite
		if (comp->sprite.texture.getImage()) {
			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(comp->sprite.texture.getImage()), { 50.f, 50.f });
		}

		static bool addTexturePopup = false;

		if (ImGui::Button("Add Texture")) {
			addTexturePopup = true;
		}

		if (comp->sprite.texture.getImage()) {
			if (ImGui::Button("Remove Texture")) comp->sprite.texture.unload();
		}

		if (addTexturePopup) {
			const char* name;

			addTexturePopup = m_TexturePicker.getTextureName(&name);

			if (name) {
				sv::Scene& scene = simulation_scene_get();
				comp->sprite.texture.load(name);
			}
		}
	}

	void EntityInspectorViewport::showNameComponentInfo(sv::NameComponent* comp)
	{
		constexpr ui32 MAX_LENGTH = 32;
		char name[MAX_LENGTH];

		// Set actual name into buffer
		{
			ui32 i;
			ui32 size = comp->name.size();
			if (size >= MAX_LENGTH) size = MAX_LENGTH - 1;
			for (i = 0; i < size; ++i) {
				name[i] = comp->name[i];
			}
			name[i] = '\0';
		}

		ImGui::InputText("Name", name, MAX_LENGTH);

		// Set new name into buffer
		comp->name = name;
	}

	void EntityInspectorViewport::showCameraComponentInfo(sv::CameraComponent* comp)
	{
		sv::ProjectionType projectionType = comp->camera.getProjectionType();
		sv::Camera& cam = comp->camera;

		if (ImGui::BeginCombo("Projection", (projectionType == sv::ProjectionType_Orthographic) ? "Orthographic" : "Perspective")) {

			if (projectionType == sv::ProjectionType_Orthographic) {
				if (ImGui::Button("Perspective")) {
					cam.setProjectionType(sv::ProjectionType_Perspective);
					cam.setWidth(1.f);
					cam.setHeight(1.f);
					cam.setNear(0.01f);
					cam.setFar(100000.f);
				}
			}
			else if (projectionType == sv::ProjectionType_Orthographic) {
				if (ImGui::Button("Orthographic")) {
					cam.setProjectionType(sv::ProjectionType_Orthographic);
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

	void EntityInspectorViewport::showRigidBody2DComponentInfo(sv::RigidBody2DComponent* comp)
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

				sv::Collider2D collider = comp->colliders[comp->collidersCount];
				collider.box.size = { 1.f, 1.f };
				collider.box.angularOffset = 0.f;
				collider.type = sv::Collider2DType_Box;

				comp->collidersCount++;

			}
		}

		if (selectedCollider < comp->collidersCount) {

			sv::Collider2D& collider = comp->colliders[selectedCollider];

			ImGui::DragFloat("Density", &collider.density, 0.1f);
			ImGui::DragFloat("Friction", &collider.friction, 0.1f);
			ImGui::DragFloat("Restitution", &collider.restitution, 0.1f);
			ImGui::DragFloat2("Offset", &collider.offset.x, 0.01f);

			switch (collider.type)
			{
			case sv::Collider2DType_Box:
				ImGui::Text("Box Collider");
				ImGui::DragFloat2("Size", &collider.box.size.x, 0.1f);
				ImGui::DragFloat("Angular Offset", &collider.box.angularOffset, 0.01f);
				break;

			case sv::Collider2DType_Circle:
				ImGui::Text("Circle Collider");
				ImGui::DragFloat2("Radius", &collider.circle.radius, 0.1f);
				break;
			}

			if (collider.density < 0.f) collider.density = 0.f;
		}
	}

}