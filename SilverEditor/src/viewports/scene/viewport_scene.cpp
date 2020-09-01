#include "core_editor.h"

#include "viewports/viewport_scene_editor.h"
#include "editor.h"
#include "viewports.h"

#include "input.h"
#include "simulation.h"
#include "scene.h"
#include "editor.h"
#include "scene_editor.h"

namespace sve {

	static sv::Entity g_SelectedEntity = SV_ENTITY_NULL;
	static bool g_ShowHierarchyToolTip = false;

	static sv::uvec2 g_ViewportSize;
	static bool g_Visible = false;
	static bool g_Focused = false;

	void ShowEntity(sv::Entity entity, sv::ECS& ecs)
	{
		auto& entities = ecs.entities;
		auto& entityData = ecs.entityData;

		sv::EntityData& ed = entityData[entity];

		bool empty = ed.childsCount == 0;
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (empty ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);
		
		if (g_SelectedEntity == entity) {
			treeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		bool active;
		sv::NameComponent* nameComponent = (sv::NameComponent*) sv::ecs_component_get_by_id(entity, sv::NameComponent::ID, ecs);
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
					ShowEntity(e, ecs);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}
	};

	bool viewport_scene_hierarchy_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SCENE_HIERARCHY))) {
			sv::Scene& scene = simulation_scene_get();

			auto& entities = scene.ecs.entities;
			auto& entityData = scene.ecs.entityData;

			for (size_t i = 1; i < entities.size(); ++i) {

				sv::Entity entity = entities[i];
				sv::EntityData& ed = entityData[entity];

				if (ed.parent == SV_ENTITY_NULL) {
					ShowEntity(entity, scene.ecs);
					i += ed.childsCount;
				}

			}

			if (g_SelectedEntity >= entityData.size()) g_SelectedEntity = SV_ENTITY_NULL;

			if (sv::input_mouse_released(SV_MOUSE_RIGHT)) g_ShowHierarchyToolTip = !g_ShowHierarchyToolTip;

			ImGui::Separator();

			if (ImGui::Button("Create")) {
				sv::ecs_entity_create(SV_ENTITY_NULL, scene.ecs);
			}

			if (g_SelectedEntity != SV_ENTITY_NULL) {

				ImGui::SameLine();

				if (ImGui::Button("Create Child")) {
					sv::ecs_entity_create(g_SelectedEntity, scene.ecs);
				}

				if (ImGui::Button("Duplicate")) {
					sv::ecs_entity_duplicate(g_SelectedEntity, scene.ecs);
				}

				ImGui::SameLine();

				if (ImGui::Button("Destroy")) {
					sv::ecs_entity_destroy(g_SelectedEntity, scene.ecs);
					g_SelectedEntity = SV_ENTITY_NULL;
				}
			}

		}

		ImGui::End();

		return true;
	}

	void ShowSpriteComponentInfo(sv::SpriteComponent* comp)
	{
		sv::Color4f col = { float(comp->color.x) / 255.f, float(comp->color.y) / 255.f, float(comp->color.z) / 255.f, float(comp->color.w) / 255.f };
		ImGui::ColorEdit4("SpriteColor", &col.x);
		comp->color = { ui8(col.x * 255.f), ui8(col.y * 255.f) , ui8(col.z * 255.f) , ui8(col.w * 255.f) };

		ImGui::Separator();

		// Sprite
		if (comp->sprite.texture.Get()) {
			ImGuiDevice& device = editor_device_get();
			ImGui::ImageButton(device.ParseImage(comp->sprite.texture->GetImage()), { 50.f, 50.f });
		}
		else {
			//ImGui::ImageButton(0, { 50.f, 50.f });

		}

		static bool addTexturePopup = false;

		if (ImGui::Button("Add Texture")) {
			addTexturePopup = true;
		}

		if (addTexturePopup) {
			
			if (ImGui::Begin("TexturePopup", 0, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar)) {

				auto& textures = sv::scene_assets_texturesmap_get();
				for (auto it = textures.begin(); it != textures.end(); ++it) {

					if (ImGui::Button(it->first.c_str())) {

						sv::Scene& scene = simulation_scene_get();
						sv::scene_assets_load_texture(scene, it->first.c_str(), comp->sprite.texture);
						comp->sprite.index = 0u;

						addTexturePopup = false;
						break;
					}

				}

				if (!ImGui::IsWindowFocused()) addTexturePopup = false;
			}
			ImGui::End();
		}
	}

	void ShowCameraComponentInfo(sv::CameraComponent* comp)
	{
		sv::CameraType camType = comp->settings.projection.cameraType;

		if (ImGui::BeginCombo("Projection", (camType == sv::CameraType_Orthographic) ? "Orthographic" : "Perspective")) {

			if (camType == sv::CameraType_Orthographic) {
				if (ImGui::Button("Perspective")) {
					comp->settings.projection = { sv::CameraType_Perspective, 1.f, 1.f, 0.01f, 100000.f };
				}
			}
			else if (camType == sv::CameraType_Perspective) {
				if (ImGui::Button("Orthographic")) {
					comp->settings.projection = { sv::CameraType_Orthographic, 10.f, 10.f, -100000.f, 100000.f };
				}
			}

			ImGui::EndCombo();
		}

		ImGui::DragFloat("Near", &comp->settings.projection.near, 0.01f);
		ImGui::DragFloat("Far", &comp->settings.projection.far, 0.1f);
		float zoom = sv::renderer_projection_zoom_get(comp->settings.projection);
		ImGui::DragFloat("Zoom", &zoom, 0.01f);
		sv::renderer_projection_zoom_set(comp->settings.projection, zoom);
		
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
		ImGui::DragFloat2("Velocity", &comp->velocity.x, 0.1f);
		ImGui::DragFloat("Angular Velocity", &comp->angularVelocity, 0.1f);
		ImGui::Checkbox("Dynamic", &comp->dynamic);
		ImGui::Checkbox("FixedRotation", &comp->fixedRotation);
	}

	void ShowQuadComponentInfo(sv::QuadComponent* comp)
	{
		ImGui::DragFloat("Density", &comp->density, 0.1f);
		ImGui::DragFloat("Friction", &comp->friction, 0.1f);
		ImGui::DragFloat("Restitution", &comp->restitution, 0.1f);
		ImGui::DragFloat2("Size", &comp->size.x, 0.1f);
		ImGui::DragFloat2("Offset", &comp->offset.x, 0.01f);
		ImGui::DragFloat("Angular Offset", &comp->angularOffset, 0.01f);

		if (comp->density < 0.f) comp->density = 0.f;
	}

	void ShowMeshComponentInfo(sv::MeshComponent* comp)
	{
		if (comp->mesh.Get() != nullptr) {
			sv::Mesh& mesh = *comp->mesh;
		}

		if (comp->material.Get() != nullptr) {
			sv::Material& mat = *comp->material;
			sv::MaterialData data = mat.GetMaterialData();

			ImGui::ColorEdit4("Diffuse color", &data.diffuseColor.x);

			mat.SetMaterialData(data);
		}
	}

	void ShowLightComponentInfo(sv::LightComponent* comp)
	{
		ImGui::ColorEdit3("Light Color", &comp->color.x);
		ImGui::DragFloat("Intensity", &comp->intensity, 0.01f);
		ImGui::DragFloat("Range", &comp->range, 0.01f);
		ImGui::DragFloat("Smoothness", &comp->smoothness, 0.01f);
	}

	void ShowComponentInfo(sv::CompID ID, sv::BaseComponent* comp)
	{
		if (ID == sv::SpriteComponent::ID) ShowSpriteComponentInfo(reinterpret_cast<sv::SpriteComponent*>(comp));
		else if (ID == sv::NameComponent::ID) ShowNameComponentInfo(reinterpret_cast<sv::NameComponent*>(comp));
		else if (ID == sv::CameraComponent::ID) ShowCameraComponentInfo(reinterpret_cast<sv::CameraComponent*>(comp));
		else if (ID == sv::RigidBody2DComponent::ID) ShowRigidBody2DComponentInfo(reinterpret_cast<sv::RigidBody2DComponent*>(comp));
		else if (ID == sv::QuadComponent::ID) ShowQuadComponentInfo(reinterpret_cast<sv::QuadComponent*>(comp));
		else if (ID == sv::MeshComponent::ID) ShowMeshComponentInfo(reinterpret_cast<sv::MeshComponent*>(comp));
		else if (ID == sv::LightComponent::ID) ShowLightComponentInfo(reinterpret_cast<sv::LightComponent*>(comp));
	}

	bool viewport_scene_entity_inspector_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_ENTITY_INSPECTOR))) {

			sv::ECS& ecs = simulation_scene_get().ecs;
			auto& entities = ecs.entities;
			auto& entityData = ecs.entityData;

			if (g_SelectedEntity != SV_ENTITY_NULL) {

				sv::NameComponent* nameComponent = sv::ecs_component_get<sv::NameComponent>(g_SelectedEntity, ecs);
				if (nameComponent) {
					ImGui::Text("%s[%u]", nameComponent->GetName().c_str(), g_SelectedEntity);
				}
				else {
					ImGui::Text("Entity[%u]", g_SelectedEntity);
				}

				ImGui::Separator();

				sv::EntityData& ed = entityData[g_SelectedEntity];

				// Show Transform Data
				sv::Transform trans = sv::ecs_entity_get_transform(g_SelectedEntity, ecs);

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

					std::vector<ui8>& componentList = ecs.components[compID];
					sv::BaseComponent* comp = (sv::BaseComponent*)(&componentList[index]);

					ImGui::Separator();
					bool remove = false;
					if (ImGui::TreeNodeEx(sv::ecs_components_get_name(compID), flags)) {
						ShowComponentInfo(compID, comp);
						ImGui::TreePop();
					}

					if (remove) {
						sv::ecs_component_remove_by_id(g_SelectedEntity, compID, ecs);
						break;
					}
					ImGui::Separator();
				}


				if (ImGui::BeginCombo("Add", "Add Component")) {

					for (ui16 ID = 0; ID < sv::ecs_components_get_count(); ++ID) {
						const char* NAME = sv::ecs_components_get_name(ID);
						if (sv::ecs_component_get_by_id(g_SelectedEntity, ID, ecs) != nullptr) continue;
						size_t SIZE = sv::ecs_components_get_size(ID);
						if (ImGui::Button(NAME)) {
							sv::ecs_component_add_by_id(g_SelectedEntity, ID, ecs);
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
							sv::ecs_component_remove_by_id(g_SelectedEntity, ID, ecs);
							break;
						}
					}

					ImGui::EndCombo();
				}

			}
		}

		ImGui::End();

		return true;
	}

	bool viewport_scene_editor_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SCENE_EDITOR))) {

			g_Visible = true;
			g_Focused = ImGui::IsWindowFocused();
			ImVec2 size = ImGui::GetWindowSize();
			g_ViewportSize = { ui32(size.x), ui32(size.y) };

			if (ImGui::Button(simulation_running() ? "Stop" : "Start")) {
				if (simulation_running()) {
					simulation_stop();
				}
				else simulation_run();
			}


			if (simulation_running()) {

				ImGui::SameLine();

				if (ImGui::Button(simulation_paused() ? "Continue" : "Pause")) {
					if (simulation_paused()) {
						simulation_continue();
					}
					else {
						simulation_pause();
					}
				}

			}


			auto& camera = scene_editor_camera_get();
			ImVec2 v = ImGui::GetWindowSize();
			auto& offscreen = camera.offscreen;

			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(offscreen.renderTarget), { v.x, v.y });

		}
		else g_Visible = false;

		ImGui::End();

		return true;
	}

	bool viewport_scene_editor_visible()
	{
		return g_Visible;
	}

	sv::uvec2 viewport_scene_editor_size()
	{
		return g_ViewportSize;
	}

	bool viewport_scene_editor_has_focus()
	{
		return g_Focused;
	}

}