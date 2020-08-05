#include "core.h"

#include "viewport_manager.h"
#include "editor.h"

namespace sve {

	static sv::Entity g_SelectedEntity = SV_INVALID_ENTITY;
	static bool g_ShowHierarchyToolTip = false;

	void ShowEntity(sv::Entity entity)
	{
		auto& entities = _sv::scene_ecs_get_entities();
		auto& entityData = _sv::scene_ecs_get_entity_data();

		_sv::EntityData& ed = entityData[entity];

		bool empty = ed.childsCount == 0;
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (empty ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);
		
		if (g_SelectedEntity == entity) {
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool active;
		sv::NameComponent* nameComponent = (sv::NameComponent*) sv::scene_ecs_component_get_by_id(entity, sv::NameComponent::ID);
		if (nameComponent) {
			active = ImGui::TreeNodeEx((nameComponent->GetName() + "[" + std::to_string(entity) + "]").c_str(), treeFlags);
		}
		else {
			active = ImGui::TreeNodeEx(("Entity[" + std::to_string(entity) + "]").c_str(), treeFlags);
		}

		if (ImGui::IsItemClicked()) {
			if (g_SelectedEntity != entity) {
				g_SelectedEntity = entity;
			}
		}
		if (active) {
			if (!empty) {
				for (ui32 i = 0; i < ed.childsCount; ++i) {
					sv::Entity e = entities[ed.handleIndex + i + 1];
					i += entityData[e].childsCount;
					ShowEntity(e);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}
	};

	bool viewport_scene_hierarchy_display()
	{
		auto& entities = _sv::scene_ecs_get_entities();
		auto& entityData = _sv::scene_ecs_get_entity_data();


		for (size_t i = 1; i < entities.size(); ++i) {

			sv::Entity entity = entities[i];
			_sv::EntityData& ed = entityData[entity];

			if (ed.parent == SV_INVALID_ENTITY) {
				ShowEntity(entity);
				i += ed.childsCount;
			}

		}

		if (g_SelectedEntity >= entityData.size()) g_SelectedEntity = SV_INVALID_ENTITY;

		if (sv::input_mouse_released(SV_MOUSE_RIGHT)) g_ShowHierarchyToolTip = !g_ShowHierarchyToolTip;

		ImGui::Separator();

		if (ImGui::Button("Create")) {
			sv::scene_ecs_entity_create();
		}

		if (g_SelectedEntity != SV_INVALID_ENTITY) {

			ImGui::SameLine();

			if (ImGui::Button("Create Child")) {
				sv::scene_ecs_entity_create(g_SelectedEntity);
			}

			if (ImGui::Button("Duplicate")) {
				sv::scene_ecs_entity_duplicate(g_SelectedEntity);
			}

			ImGui::SameLine();

			if (ImGui::Button("Destroy")) {
				sv::scene_ecs_entity_destroy(g_SelectedEntity);
				g_SelectedEntity = SV_INVALID_ENTITY;
			}
		}

		return true;
	}

	void ShowSpriteComponentInfo(sv::SpriteComponent* comp)
	{
		sv::Color4f col = { float(comp->color.x) / 255.f, float(comp->color.y) / 255.f, float(comp->color.z) / 255.f, float(comp->color.w) / 255.f };
		ImGui::ColorEdit4("SpriteColor", &col.x);
		comp->color = { ui8(col.x * 255.f), ui8(col.y * 255.f) , ui8(col.z * 255.f) , ui8(col.w * 255.f) };
	}

	void ShowCameraComponentInfo(sv::CameraComponent* comp)
	{
		SV_REND_CAMERA_TYPE camType = comp->projection.GetCameraType();

		if (ImGui::BeginCombo("Projection", (camType == SV_REND_CAMERA_TYPE_ORTHOGRAPHIC) ? "Orthographic" : "Perspective")) {

			if (camType == SV_REND_CAMERA_TYPE_ORTHOGRAPHIC) {
				if (ImGui::Button("Perspective")) comp->projection.SetCameraType(SV_REND_CAMERA_TYPE_PERSPECTIVE);
			}
			else if (camType == SV_REND_CAMERA_TYPE_PERSPECTIVE) {
				if (ImGui::Button("Orthographic")) comp->projection.SetCameraType(SV_REND_CAMERA_TYPE_ORTHOGRAPHIC);
			}

			ImGui::EndCombo();
		}

		switch (camType)
		{
		case SV_REND_CAMERA_TYPE_ORTHOGRAPHIC:
		{
			float zoom = comp->projection.Orthographic_GetZoom();
			ImGui::DragFloat("Zoom", &zoom, 0.01f);
			comp->projection.Orthographic_SetZoom(zoom);
		}
		break;
		case SV_REND_CAMERA_TYPE_PERSPECTIVE:
			break;
		}
	}

	void ShowNameComponentInfo(sv::NameComponent* comp)
	{
		constexpr ui32 MAX_LENGTH = 32;
		char name[MAX_LENGTH];

		// Set actual name into buffer
		{
			ui32 i;
			ui32 size = comp->GetName().size();
			if (size >= MAX_LENGTH) size = MAX_LENGTH - 1;
			for (i = 0; i < size; ++i) {
				name[i] = comp->GetName()[i];
			}
			name[i] = '\0';
		}

		ImGui::InputText("Name", name, MAX_LENGTH);

		// Set new name into buffer
		comp->SetName(name);
	}

	void ShowComponentInfo(sv::CompID ID, sv::BaseComponent* comp)
	{
		if (ID == sv::SpriteComponent::ID) ShowSpriteComponentInfo(reinterpret_cast<sv::SpriteComponent*>(comp));
		else if (ID == sv::NameComponent::ID) ShowNameComponentInfo(reinterpret_cast<sv::NameComponent*>(comp));
		else if (ID == sv::CameraComponent::ID) ShowCameraComponentInfo(reinterpret_cast<sv::CameraComponent*>(comp));
	}

	bool viewport_scene_entity_display()
	{
		auto& entities = _sv::scene_ecs_get_entities();
		auto& entityData = _sv::scene_ecs_get_entity_data();

		if (g_SelectedEntity != SV_INVALID_ENTITY) {

			sv::NameComponent* nameComponent = sv::scene_ecs_component_get<sv::NameComponent>(g_SelectedEntity);
			if (nameComponent) {
				ImGui::Text("%s[%u]", nameComponent->GetName().c_str(), g_SelectedEntity);
			}
			else {
				ImGui::Text("Entity[%u]", g_SelectedEntity);
			}

			ImGui::Separator();

			_sv::EntityData& ed = entityData[g_SelectedEntity];

			// Show Transform Data
			sv::Transform trans = sv::scene_ecs_entity_get_transform(g_SelectedEntity);

			sv::vec3 position = trans.GetLocalPosition();
			sv::vec3 rotation = ToDegrees(trans.GetLocalRotation());
			sv::vec3 scale = trans.GetLocalScale();

			ImGui::DragFloat3("Position", &position.x, 0.3f);
			ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
			ImGui::DragFloat3("Scale", &scale.x, 0.01f);

			trans.SetPosition(position);
			trans.SetRotation(ToRadians(rotation));
			trans.SetScale(scale);

			ImGui::Separator();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

			for (ui32 i = 0; i < ed.indices.size(); ++i) {

				ui16 compID = ed.indices[i].first;
				size_t index;
				index = ed.indices[i].second;

				std::vector<ui8>& componentList = _sv::scene_ecs_get_components(compID);
				sv::BaseComponent* comp = (sv::BaseComponent*)(&componentList[index]);

				ImGui::Separator();
				bool remove = false;
				if (ImGui::TreeNodeEx(sv::ecs_components_get_name(compID), flags)) {
					ShowComponentInfo(compID, comp);
					ImGui::TreePop();
				}

				if (remove) {
					sv::scene_ecs_component_remove_by_id(g_SelectedEntity, compID);
					break;
				}
				ImGui::Separator();
			}


			if (ImGui::BeginCombo("Add", "Add Component")) {

				for (ui16 ID = 0; ID < sv::ecs_components_get_count(); ++ID) {
					const char* NAME = sv::ecs_components_get_name(ID);
					if (sv::scene_ecs_component_get_by_id(g_SelectedEntity, ID) != nullptr) continue;
					size_t SIZE = sv::ecs_components_get_size(ID);
					if (ImGui::Button(NAME)) {
						sv::scene_ecs_component_add_by_id(g_SelectedEntity, ID);
					}
				}

				ImGui::EndCombo();
			}
			if (ImGui::BeginCombo("Rmv", "Remove Component")) {

				_sv::EntityData& ed = entityData[g_SelectedEntity];
				for (ui32 i = 0; i < ed.indices.size(); ++i) {
					sv::CompID ID = ed.indices[i].first;
					const char* NAME = sv::ecs_components_get_name(ID);
					size_t SIZE = sv::ecs_components_get_size(ID);
					if (ImGui::Button(NAME)) {
						sv::scene_ecs_component_remove_by_id(g_SelectedEntity, ID);
						break;
					}
				}

				ImGui::EndCombo();
			}

		}

		return true;
	}

	bool viewport_scene_editor_display()
	{
		EditorState& state = editor_state_get();

		sv::Scene& scene = state.GetScene();
		ImVec2 v = ImGui::GetWindowSize();
		auto& offscreen = *state.GetDebugCamera().camera.GetOffscreen();

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		return true;

		return true;
	}

}