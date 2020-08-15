#include "core_editor.h"

#include "viewport_manager.h"
#include "editor.h"

#include "scene/scene_internal.h"
#include "input.h"
#include "simulation.h"
#include "components.h"
#include "scene_editor.h"

namespace sve {

	static sv::Entity g_SelectedEntity = SV_INVALID_ENTITY;
	static bool g_ShowHierarchyToolTip = false;

	void ShowEntity(sv::Entity entity)
	{
		auto& entities = sv::scene_ecs_get_entities();
		auto& entityData = sv::scene_ecs_get_entity_data();

		sv::EntityData& ed = entityData[entity];

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
		auto& entities = sv::scene_ecs_get_entities();
		auto& entityData = sv::scene_ecs_get_entity_data();


		for (size_t i = 1; i < entities.size(); ++i) {

			sv::Entity entity = entities[i];
			sv::EntityData& ed = entityData[entity];

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

		// Render Layers
		ui32 count = sv::renderLayer_count_get();

		ImGui::Separator();

		int layer = comp->renderLayer;
		ImGui::DragInt("Render Layer", &layer, 1);
		if (layer < 0) layer = 0;
		else if (layer >= count) layer = count - 1;
		comp->renderLayer = layer;

		sv::RenderLayerSortMode sortMode = sv::renderLayer_sortMode_get(comp->renderLayer);

		auto getSortModeStr = [](sv::RenderLayerSortMode mode) {
			switch (mode)
			{
			case sv::RenderLayerSortMode_none:
				return "None";
			case sv::RenderLayerSortMode_coordX:
				return "Coord X";
			case sv::RenderLayerSortMode_coordY:
				return "Coord Y";
			case sv::RenderLayerSortMode_coordZ:
				return "Coord Z";
			default:
				return "";
			}
		};

		if (ImGui::BeginCombo("Sort Mode", getSortModeStr(sortMode))) {

			if (sortMode != sv::RenderLayerSortMode_none) if (ImGui::Button(getSortModeStr(sv::RenderLayerSortMode_none))) sortMode = sv::RenderLayerSortMode_none;
			if (sortMode != sv::RenderLayerSortMode_coordX) if (ImGui::Button(getSortModeStr(sv::RenderLayerSortMode_coordX))) sortMode = sv::RenderLayerSortMode_coordX;
			if (sortMode != sv::RenderLayerSortMode_coordY) if (ImGui::Button(getSortModeStr(sv::RenderLayerSortMode_coordY))) sortMode = sv::RenderLayerSortMode_coordY;
			if (sortMode != sv::RenderLayerSortMode_coordZ) if (ImGui::Button(getSortModeStr(sv::RenderLayerSortMode_coordZ))) sortMode = sv::RenderLayerSortMode_coordZ;
			
			ImGui::EndCombo();
		}

		sv::renderLayer_sortMode_set(comp->renderLayer, sortMode);
	}

	void ShowCameraComponentInfo(sv::CameraComponent* comp)
	{
		sv::CameraType camType = comp->projection.cameraType;

		if (ImGui::BeginCombo("Projection", (camType == sv::CameraType_Orthographic) ? "Orthographic" : "Perspective")) {

			if (camType == sv::CameraType_Orthographic) {
				if (ImGui::Button("Perspective")) {
					comp->projection = { sv::CameraType_Perspective, 1.f, 1.f, 0.01f, 100000.f };
				}
			}
			else if (camType == sv::CameraType_Perspective) {
				if (ImGui::Button("Orthographic")) {
					comp->projection = { sv::CameraType_Orthographic, 10.f, 10.f, -100000.f, 100000.f };
				}
			}

			ImGui::EndCombo();
		}

		ImGui::DragFloat("Near", &comp->projection.near, 0.01f);
		ImGui::DragFloat("Far", &comp->projection.far, 0.1f);
		float zoom = sv::renderer_projection_zoom_get(comp->projection);
		ImGui::DragFloat("Zoom", &zoom, 0.01f);
		sv::renderer_projection_zoom_set(comp->projection, zoom);
		
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

	void ShowRigidBody2DComponentInfo(sv::RigidBody2DComponent* comp)
	{
		float density = comp->GetDensity();
		bool dynamic = comp->IsDynamic();
		bool fixedRotation = comp->IsFixedRotation();

		ImGui::DragFloat("Density", &density, 0.1f);
		ImGui::Checkbox("Dynamic", &dynamic);
		ImGui::Checkbox("FixedRotation", &fixedRotation);

		if (density < 0.f) density = 0.f;

		comp->SetDensity(density);
		comp->SetDynamic(dynamic);
		comp->SetFixedRotation(fixedRotation);
	}

	void ShowComponentInfo(sv::CompID ID, sv::BaseComponent* comp)
	{
		if (ID == sv::SpriteComponent::ID) ShowSpriteComponentInfo(reinterpret_cast<sv::SpriteComponent*>(comp));
		else if (ID == sv::NameComponent::ID) ShowNameComponentInfo(reinterpret_cast<sv::NameComponent*>(comp));
		else if (ID == sv::CameraComponent::ID) ShowCameraComponentInfo(reinterpret_cast<sv::CameraComponent*>(comp));
		else if (ID == sv::RigidBody2DComponent::ID) ShowRigidBody2DComponentInfo(reinterpret_cast<sv::RigidBody2DComponent*>(comp));
	}

	bool viewport_scene_entity_display()
	{
		auto& entities = sv::scene_ecs_get_entities();
		auto& entityData = sv::scene_ecs_get_entity_data();

		if (g_SelectedEntity != SV_INVALID_ENTITY) {

			sv::NameComponent* nameComponent = sv::scene_ecs_component_get<sv::NameComponent>(g_SelectedEntity);
			if (nameComponent) {
				ImGui::Text("%s[%u]", nameComponent->GetName().c_str(), g_SelectedEntity);
			}
			else {
				ImGui::Text("Entity[%u]", g_SelectedEntity);
			}

			ImGui::Separator();

			sv::EntityData& ed = entityData[g_SelectedEntity];

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

				std::vector<ui8>& componentList = sv::scene_ecs_get_components(compID);
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

				sv::EntityData& ed = entityData[g_SelectedEntity];
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
		//if (ImGui::Button(simulation_running() ? "Pause" : "Start")) {
		//	if (simulation_running()) {
		//		simulation_stop();
		//	}
		//	else simulation_run();
		//}

		auto& camera = scene_editor_camera_get();
		ImVec2 v = ImGui::GetWindowSize();
		auto& offscreen = camera.offscreen;

		ImGuiDevice& device = editor_device_get();
		ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		return true;
	}

}