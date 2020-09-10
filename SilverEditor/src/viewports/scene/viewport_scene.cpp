#include "core_editor.h"

#include "viewports/viewport_scene_editor.h"
#include "editor.h"
#include "viewports.h"

#include "simulation.h"
#include "scene.h"
#include "editor.h"
#include "scene_editor.h"

namespace sve {

	static sv::Entity g_SelectedEntity = SV_ENTITY_NULL;

	static bool g_HierarchyPopup = false;

	static sv::uvec2 g_ViewportSize;
	static bool g_Visible = false;
	static bool g_Focused = false;

	void ShowEntity(sv::ECS* ecs, sv::Entity entity)
	{
		ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow | (sv::ecs_entity_childs_count(ecs, entity) == 0 ? ImGuiTreeNodeFlags_Bullet : ImGuiTreeNodeFlags_AllowItemOverlap);
		
		if (g_SelectedEntity == entity) {
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

		if (!g_HierarchyPopup) {
			if (ImGui::BeginPopupContextItem(label.c_str(), ImGuiMouseButton_Right)) {

				g_HierarchyPopup = true;

				if (ImGui::Button("Create child")) {
					sv::ecs_entity_create(ecs, entity);
				}

				if (ImGui::Button("Duplicate")) {
					sv::ecs_entity_duplicate(ecs, entity);
					g_SelectedEntity = 0;
				}

				if (ImGui::Button("Destroy")) {
					sv::ecs_entity_destroy(ecs, entity);
					g_SelectedEntity = 0;
				}

				ImGui::EndPopup();
			}
		}

		if (ImGui::IsItemClicked()) {
			if (g_SelectedEntity != entity) {
				g_SelectedEntity = entity;
			}
		}
		if (active) {
			if (sv::ecs_entity_childs_count(ecs, entity) != 0) {

				sv::Entity const* childs;

				for (ui32 i = 0; i < sv::ecs_entity_childs_count(ecs, entity); ++i) {

					sv::ecs_entity_childs_get(ecs, entity, &childs);

					sv::Entity e = childs[i];
					i += sv::ecs_entity_childs_count(ecs, e);
					ShowEntity(ecs, e);
				}
				ImGui::TreePop();
			}
			else ImGui::TreePop();
		}

	};

	bool viewport_scene_hierarchy_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SCENE_HIERARCHY))) {
			g_HierarchyPopup = false;
			sv::Scene* scene = simulation_scene_get();
			sv::ECS* ecs = sv::scene_ecs_get(scene);

			for (size_t i = 0; i < sv::ecs_entity_count(ecs); ++i) {

				sv::Entity entity = sv::ecs_entity_get(ecs, i);

				if (sv::ecs_entity_parent_get(ecs, entity) == SV_ENTITY_NULL) {
					ShowEntity(ecs, entity);
					i += sv::ecs_entity_childs_count(ecs, entity);
				}

			}

			if (!sv::ecs_entity_exist(ecs, g_SelectedEntity)) g_SelectedEntity = SV_ENTITY_NULL;

			if (!g_HierarchyPopup) {
			
				if (ImGui::BeginPopupContextWindow("Hierarchy Popup", ImGuiMouseButton_Right)) {
					
					g_HierarchyPopup = true;

					if (ImGui::Button("Create empty entity")) {
						sv::ecs_entity_create(ecs);
					}

					if (ImGui::Button("Create sprite")) {
						sv::Entity entity = sv::ecs_entity_create(ecs);
						sv::ecs_component_add<sv::SpriteComponent>(ecs, entity);
					}

					ImGui::EndPopup();
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

		ImGui::DragFloat4("TexCoord", &comp->sprite.texCoord.x, 0.001f);

		ImGui::Separator();

		// Sprite
		if (comp->sprite.texture.Get()) {
			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(comp->sprite.texture->texture.GetImage()), { 50.f, 50.f });
		}

		static bool addTexturePopup = false;

		if (ImGui::Button("Add Texture")) {
			addTexturePopup = true;
		}

		if (comp->sprite.texture.Get()) {
			if (ImGui::Button("Remove Texture")) comp->sprite.texture.Delete();
		}

		if (addTexturePopup) {
			
			if (ImGui::Begin("TexturePopup", 0, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar)) {

				auto& assets = sv::assets_map_get();
				for (auto it = assets.begin(); it != assets.end(); ++it) {

					if (it->second.assetType == sv::AssetType_Texture) {
						if (ImGui::Button(it->first.c_str())) {

							sv::Scene* scene = simulation_scene_get();
							sv::assets_load_texture(it->first.c_str(), comp->sprite.texture);

							addTexturePopup = false;
							break;
						}
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

	void ShowRigidBody2DComponentInfo(sv::RigidBody2DComponent* comp)
	{
		ImGui::DragFloat2("Velocity", &comp->velocity.x, 0.1f);
		ImGui::DragFloat("Angular Velocity", &comp->angularVelocity, 0.1f);
		ImGui::Checkbox("Dynamic", &comp->dynamic);
		ImGui::Checkbox("FixedRotation", &comp->fixedRotation);
	}

	void ShowQuadComponentInfo(sv::Box2DComponent* comp)
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
		else if (ID == sv::Box2DComponent::ID) ShowQuadComponentInfo(reinterpret_cast<sv::Box2DComponent*>(comp));
		else if (ID == sv::MeshComponent::ID) ShowMeshComponentInfo(reinterpret_cast<sv::MeshComponent*>(comp));
		else if (ID == sv::LightComponent::ID) ShowLightComponentInfo(reinterpret_cast<sv::LightComponent*>(comp));
	}

	bool viewport_scene_entity_inspector_display()
	{
		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_ENTITY_INSPECTOR))) {

			sv::ECS* ecs = sv::scene_ecs_get(simulation_scene_get());

			if (g_SelectedEntity != SV_ENTITY_NULL) {

				sv::NameComponent* nameComponent = sv::ecs_component_get<sv::NameComponent>(ecs, g_SelectedEntity);
				if (nameComponent) {
					ImGui::Text("%s[%u]", nameComponent->name.c_str(), g_SelectedEntity);
				}
				else {
					ImGui::Text("Entity[%u]", g_SelectedEntity);
				}

				ImGui::Separator();

				// Show Transform Data
				sv::Transform trans = sv::ecs_entity_transform_get(ecs, g_SelectedEntity);

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

				for (ui32 i = 0; i < sv::ecs_entity_component_count(ecs, g_SelectedEntity); ++i) {

					auto [compID, comp] = sv::ecs_component_get_by_index(ecs, g_SelectedEntity, i);

					ImGui::Separator();
					
					if (ImGui::TreeNodeEx(sv::ecs_register_nameof(compID), flags)) {
						ShowComponentInfo(compID, comp);
						ImGui::TreePop();
					}

					ImGui::Separator();
				}


				if (ImGui::BeginCombo("Add", "Add Component")) {

					for (ui16 ID = 0; ID < sv::ecs_register_count(); ++ID) {
						const char* NAME = sv::ecs_register_nameof(ID);
						if (sv::ecs_component_get_by_id(ecs, g_SelectedEntity, ID) != nullptr) continue;
						size_t SIZE = sv::ecs_register_sizeof(ID);
						if (ImGui::Button(NAME)) {
							sv::ecs_component_add_by_id(ecs, g_SelectedEntity, ID);
						}
					}

					ImGui::EndCombo();
				}
				if (ImGui::BeginCombo("Rmv", "Remove Component")) {

					for (ui32 i = 0; i < sv::ecs_entity_component_count(ecs, g_SelectedEntity); ++i) {
						sv::CompID ID = sv::ecs_component_get_by_index(ecs, g_SelectedEntity, i).first;
						const char* NAME = sv::ecs_register_nameof(ID);
						size_t SIZE = sv::ecs_register_sizeof(ID);
						if (ImGui::Button(NAME)) {
							sv::ecs_component_remove_by_id(ecs, g_SelectedEntity, ID);
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
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });

		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_SCENE_EDITOR), 0, ImGuiWindowFlags_NoScrollbar)) {

			g_Visible = true;
			g_Focused = ImGui::IsWindowFocused();
			ImVec2 size = ImGui::GetWindowContentRegionMax();
			size.x -= ImGui::GetWindowContentRegionMin().x;
			size.y -= ImGui::GetWindowContentRegionMin().y;
			g_ViewportSize = { ui32(size.x), ui32(size.y) };

			auto& camera = scene_editor_camera_get();
			auto& offscreen = camera.offscreen;

			ImGuiDevice& device = editor_device_get();
			ImGui::Image(device.ParseImage(offscreen.renderTarget), { size.x, size.y });

		}
		else g_Visible = false;

		ImGui::End();

		ImGui::PopStyleVar(2u);

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