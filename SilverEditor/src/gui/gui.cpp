#include "core_editor.h"

#include "gui.h"
#include "external/ImGui/imgui_internal.h"
#include "editor.h"
#include "panel_manager.h"
#include "panels/MaterialPanel.h"
#include "panels/SpriteAnimationPanel.h"

namespace sv {

	Result gui_initialize()
	{
		return Result_Success;
	}

	Result gui_close()
	{
		return Result_Success;
	}

	void gui_begin()
	{

	}

	void gui_end()
	{

	}

	void gui_asset_picker_open(AssetType type)
	{
		ImGui::OpenPopup((const char*)type);
	}

	const char* gui_asset_picker_show(AssetType type, bool* showing)
	{
		if (type == nullptr) return nullptr;

		const char* res = nullptr;

		if (ImGui::BeginPopup((const char*)type)) {

			if (showing)* showing = true;

			auto& files = asset_map_get();

			for (auto& file : files) {
				if (file.second.assetType == type) {

					if (ImGui::Button(file.first.c_str())) {
						res = file.first.c_str();
					}

				}
			}
			ImGui::EndPopup();
		}
		else if (showing)* showing = false;

		return res;
	}

	float getColumnWidth()
	{
		return std::min(ImGui::GetWindowWidth() * 0.3f, 130.f);
	}

	bool dragTransformVector(const char* name, vec3f& value, float speed, float defaultValue)
	{
		ImGui::PushID(name);

		ImGui::Columns(2u);

		ImGui::SetColumnWidth(ImGui::GetColumnIndex(), getColumnWidth());

		ImGui::Text(name);

		ImGui::NextColumn();

		bool result = false;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0.f, 0.f });
		ImGui::PushMultiItemsWidths(3u, ImGui::CalcItemWidth());

		constexpr float buttonSize = 0.f;

		// X

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.9f, 0.1f, 0.1f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.f, 0.f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.8f, 0.f, 0.f, 1.f });

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		if (ImGui::Button("X", { buttonSize , buttonSize })) {
			value.x = defaultValue;
			result = true;
		}
		ImGui::SameLine();

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		if (ImGui::DragFloat("##x", &value.x, speed, 0.f, 0.f, "%.2f")) result = true;
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Y

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.8f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.0f, 0.7f, 0.0f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.0f, 0.7f, 0.0f, 1.f });

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		if (ImGui::Button("Y", { buttonSize, buttonSize })) {
			value.y = defaultValue;
			result = true;
		}
		ImGui::SameLine();

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		if (ImGui::DragFloat("##y", &value.y, speed, 0.f, 0.f, "%.2f")) result = true;
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Z

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.05f, 0.1f, 0.9f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.f, 0.f, 0.7f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.f, 0.f, 0.7f, 1.f });

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

		if (ImGui::Button("Z", { buttonSize, buttonSize })) {
			value.z = defaultValue;
			result = true;
		}
		ImGui::SameLine();

		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		if (ImGui::DragFloat("##z", &value.z, speed, 0.f, 0.f, "%.2f")) result = true;
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::NextColumn();

		ImGui::PopID();

		return result;
	}

	bool gui_transform_show(Transform& trans)
	{
		constexpr ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed;

		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		bool tree = ImGui::TreeNodeEx("Transform", flags | ImGuiTreeNodeFlags_DefaultOpen);
		ImGui::PopFont();

		if (tree) {
			
			vec3f position = trans.getLocalPosition();
			vec3f scale = trans.getLocalScale();
			vec3f rotation = ToDegrees(trans.getLocalEulerRotation());

			if (dragTransformVector("Position", position, 0.3f, 0.f))
			{
				trans.setPosition(position);
			}
			if (dragTransformVector("Scale", scale, 0.1f, 1.f))
			{
				trans.setScale(scale);
			}
			if (dragTransformVector("Rotation", rotation, 1.f, 0.f)) {
				trans.setEulerRotation(ToRadians(rotation));
			}

			ImGui::Columns(1);

			ImGui::TreePop();
		}
		ImGui::Separator();

		return tree;
	}

	bool gui_component_show(ECS* ecs, CompID compID, BaseComponent* comp, const std::function<void(CompID cpmpID, BaseComponent* comp)>& fn)
	{
		constexpr ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Framed;

		ImGui::PushID(compID);

		bool destroyComponent = false;
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
		bool tree = ImGui::TreeNodeEx(ecs_register_nameof(ecs, compID), flags);
		ImGui::PopFont();

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) ImGui::OpenPopup("ComponentPopup");

		if (ImGui::BeginPopup("ComponentPopup")) {

			if (ImGui::Button("Destroy")) destroyComponent = true;

			ImGui::EndPopup();
		}


		if (tree) {
			fn(compID, comp);
			ImGui::TreePop();
		}

		ImGui::Separator();

		ImGui::PopID();

		if (destroyComponent) {
			ecs_component_remove_by_id(ecs, comp->entity, compID);
		}

		return tree;
	}

	void gui_component_item_begin()
	{
		ImGui::Columns(2);

		ImGui::SetColumnWidth(ImGui::GetColumnIndex(), getColumnWidth());
	}

	void gui_component_item_next(const char* name)
	{
		if (ImGui::GetColumnIndex() == 1) ImGui::NextColumn();

		ImGui::Text(name);

		ImGui::NextColumn();
	}

	void gui_component_item_end()
	{
		ImGui::Columns(1);
	}

	bool gui_component_item_color_picker(Color& color)
	{
		Color4f col = { float(color.r) / 255.f, float(color.g) / 255.f, float(color.b) / 255.f, float(color.a) / 255.f };

		bool res = gui_component_item_color_picker(col);

		if (res) {
			color = { ui8(col.r * 255.f), ui8(col.g * 255.f) , ui8(col.b * 255.f) , ui8(col.a * 255.f) };
		}
		return res;
	}

	bool gui_component_item_color_picker(Color4f& col)
	{
		if (ImGui::ColorButton("##spriteColor", *(ImVec4*)& col)) {
			ImGui::OpenPopup("ColorPicker");
		};

		bool res = false;
		if (ImGui::BeginPopup("ColorPicker")) {

			if (ImGui::ColorPicker4("##colorPicker", &col.r)) {
				res = true;
			}

			ImGui::EndPopup();
		}
		return res;
	}

	bool gui_component_item_color_picker(Color3f& col)
	{
		if (ImGui::ColorButton("##spriteColor", ImVec4{ col.r, col.g, col.b, 1.f })) {
			ImGui::OpenPopup("ColorPicker");
		};

		bool res = false;
		if (ImGui::BeginPopup("ColorPicker")) {

			if (ImGui::ColorPicker3("##colorPicker", &col.r)) {
				res = true;
			}

			ImGui::EndPopup();
		}
		return res;
	}

	bool gui_component_item_texture(TextureAsset& texture)
	{
		AssetType textureType = asset_type_get("Texture");
		if (texture.get()) {
			ImGuiDevice& device = editor_device_get();
			if (ImGui::ImageButton(device.ParseImage(texture.get()), { 50.f, 50.f })) {
				ImGui::OpenPopup("ImagePopUp");
			}


		}
		else if (ImGui::Button("No texture")) gui_asset_picker_open(textureType);

		bool openPopup = false;
		bool result = false;

		if (ImGui::BeginPopup("ImagePopUp")) {

			if (ImGui::MenuItem("New")) {
				ImGui::CloseCurrentPopup();
				openPopup = true;
			}
			if (ImGui::MenuItem("Remove")) {
				texture.unload();
				ImGui::CloseCurrentPopup();
				result = true;
			}

			ImGui::EndPopup();
		}

		if (openPopup) gui_asset_picker_open(textureType);

		const char* name = gui_asset_picker_show(textureType);

		if (name) {
			texture.load(name);
			result = true;
		}

		return result;
	}

	void gui_component_item_material(MaterialAsset& material)
	{
		if (material.get()) {
			if (ImGui::Button(material.getFilePath())) {
				ImGui::OpenPopup("MaterialPopup");
			}
		}
		else {
			if (ImGui::Button("No material")) {
				ImGui::OpenPopup("MaterialPopup");
			}
		}
		
		bool openPopup = false;
		if (ImGui::BeginPopup("MaterialPopup")) {

			if (ImGui::MenuItem("New")) {
				ImGui::CloseCurrentPopup();
				openPopup = true;
			}

			if (ImGui::MenuItem("Remove")) {
				ImGui::CloseCurrentPopup();
				material.unload();
			}

			if (ImGui::MenuItem("Edit")) {
				ImGui::CloseCurrentPopup();
				
				MaterialPanel* matPanel = (MaterialPanel*)panel_manager_get("Material");
				if (matPanel == nullptr) {
					matPanel = new MaterialPanel();
					panel_manager_add("Material", matPanel);
				}

				matPanel->setSelectedMaterial(material);
			}

			ImGui::EndPopup();
		}

		AssetType type = asset_type_get("Material");
		if (openPopup) gui_asset_picker_open(type);

		const char* name = gui_asset_picker_show(type);

		if (name) {
			material.load(name);
		}
	}

	bool gui_component_item_sprite_animation(SpriteAnimationAsset& spriteAnimation)
	{
		AssetType textureType = asset_type_get("Sprite Animation");

		if (spriteAnimation.hasReference()) {
			ImGuiDevice& device = editor_device_get();

			GPUImage* img = spriteAnimation.get()->sprites.front().texture.get();

			if (img) {
				if (ImGui::ImageButton(device.ParseImage(img), { 50.f, 50.f })) {
					ImGui::OpenPopup("SpriteAnimationPopUp");
				}
			}
			else {
				if (ImGui::ColorButton("##WhiteAnimationImage", { 1.f, 1.f, 1.f, 1.f }, 0, { 50.f, 50.f })) {
					ImGui::OpenPopup("SpriteAnimationPopUp");
				}
			}
		}
		else if (ImGui::Button("No animation")) gui_asset_picker_open(textureType);

		bool openPopup = false;
		bool result = false;

		if (ImGui::BeginPopup("SpriteAnimationPopUp")) {

			if (ImGui::MenuItem("New")) {
				ImGui::CloseCurrentPopup();
				openPopup = true;
			}
			if (ImGui::MenuItem("Remove")) {
				spriteAnimation.unload();
				ImGui::CloseCurrentPopup();
				result = true;
			}
			if (ImGui::MenuItem("Edit")) {
				ImGui::CloseCurrentPopup();

				SpriteAnimationPanel* sprPanel = (SpriteAnimationPanel*)panel_manager_get("Sprite Animation");
				if (sprPanel == nullptr) {
					sprPanel = new SpriteAnimationPanel();
					panel_manager_add("Sprite Animation", sprPanel);
				}

				sprPanel->setSelectedAnimation(spriteAnimation);
			}

			ImGui::EndPopup();
		}

		if (openPopup) gui_asset_picker_open(textureType);

		const char* name = gui_asset_picker_show(textureType);

		if (name) {
			spriteAnimation.load(name);
			result = true;
		}

		return result;
	}

	bool gui_component_item_string(std::string& str)
	{
		constexpr ui32 MAX_LENGTH = 32;
		char name[MAX_LENGTH];

		// Set actual name into buffer
		{
			ui32 i;
			ui32 size = ui32(str.size());
			if (size >= MAX_LENGTH) size = MAX_LENGTH - 1;
			for (i = 0; i < size; ++i) {
				name[i] = str[i];
			}
			name[i] = '\0';
		}

		bool res = ImGui::InputText("##Name", name, MAX_LENGTH);

		// Set new name into buffer
		str = name;
		return res;
	}

	bool gui_component_item_mat4(XMMATRIX& matrix)
	{
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, matrix);
		bool res = gui_component_item_mat4(m);
		if (res) matrix = XMLoadFloat4x4(&m);
		return res;
	}

	bool gui_component_item_mat4(XMFLOAT4X4& m)
	{
		// It shows transposed
		std::stringstream preview;
		preview << m._11 << '-' << m._21 << '-' << m._31 << '-' << m._41;
		preview << '\n' << m._12 << '-' << m._22 << '-' << m._32 << '-' << m._42;
		preview << '\n' << m._13 << '-' << m._23 << '-' << m._33 << '-' << m._43;
		preview << '\n' << m._14 << '-' << m._24 << '-' << m._34 << '-' << m._44;

		if (ImGui::Button(preview.str().c_str())) {
			ImGui::OpenPopup("MatrixPopup");
		}

		bool result = false;

		if (ImGui::BeginPopup("MatrixPopup")) {

			// ROW 1
			ImVec4 row = { m._11, m._21, m._31, m._41 };
			if (ImGui::DragFloat4("##r0", &row.x), 0.1f) {
				m._11 = row.x;
				m._21 = row.y;
				m._31 = row.z;
				m._41 = row.w;
				result = true;
			}

			// ROW 2
			row = { m._12, m._22, m._32, m._42 };
			if (ImGui::DragFloat4("##r1", &row.x), 0.1f) {
				m._12 = row.x;
				m._22 = row.y;
				m._32 = row.z;
				m._42 = row.w;
				result = true;
			}

			// ROW 3
			row = { m._13, m._23, m._33, m._43 };
			if (ImGui::DragFloat4("##r2", &row.x), 0.1f) {
				m._13 = row.x;
				m._23 = row.y;
				m._33 = row.z;
				m._43 = row.w;
				result = true;
			}

			// ROW 4
			row = { m._14, m._24, m._34, m._44 };
			if (ImGui::DragFloat4("##r3", &row.x), 0.1f) {
				m._14 = row.x;
				m._24 = row.y;
				m._34 = row.z;
				m._44 = row.w;
				result = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Set Identity")) {
				XMStoreFloat4x4(&m, XMMatrixIdentity());
				result = true;
			}

			if (ImGui::MenuItem("Transpose")) {
				XMMATRIX transposed = XMMatrixTranspose(XMLoadFloat4x4(&m));
				XMStoreFloat4x4(&m, transposed);
				result = true;
			}

			ImGui::EndPopup();
		}

		return result;
	}


}