#include "core_editor.h"

#include "gui.h"
#include "external/ImGui/imgui_internal.h"
#include "editor.h"
#include "panel_manager.h"
#include "panels/MaterialPanel.h"
#include "panels/SpriteAnimationPanel.h"
#include "panels/SimulationPanel.h"
#include "panels/SceneEditorPanel.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/EntityInspectorPanel.h"
#include "panels/AssetPanel.h"
#include "panels/SimulationToolsPanel.h"

#include "utils/io.h"

namespace sv {

	static GuiStyle g_Style;
	bool g_SimulationMode;

	Result gui_initialize()
	{
		gui_style_set(GuiStyle_Black);
		gui_style_editor();

		// Get last state
		ArchiveI file;
		if (result_okay(bin_read(hash_string("Editor State"), file))) {

			ui32 count;
			file >> count;

			std::string name;
			for (ui32 i = 0; i < count; ++i) {

				file >> name;

				if (strcmp(name.c_str(), "Simulation") == 0) panel_manager_add(name.c_str(), new SimulationPanel());
				else if (strcmp(name.c_str(), "Scene Editor") == 0) panel_manager_add(name.c_str(), new SceneEditorPanel());
				else if (strcmp(name.c_str(), "Scene Hierarchy") == 0) panel_manager_add(name.c_str(), new SceneHierarchyPanel());
				else if (strcmp(name.c_str(), "Entity Inspector") == 0) panel_manager_add(name.c_str(), new EntityInspectorPanel());
				else if (strcmp(name.c_str(), "Assets") == 0) panel_manager_add(name.c_str(), new AssetPanel());
				else if (strcmp(name.c_str(), "Simulation Tools") == 0) panel_manager_add(name.c_str(), new SimulationToolsPanel());
			}
		}
		else SV_LOG_ERROR("Can't deserialize Editor State");

		return Result_Success;
	}

	Result gui_close()
	{
		// Save state
		{
			ArchiveO file;

			std::vector<const char*> panels;

			if (panel_manager_get("Simulation")) panels.emplace_back("Simulation");
			if (panel_manager_get("Scene Editor")) panels.emplace_back("Scene Editor");
			if (panel_manager_get("Scene Hierarchy")) panels.emplace_back("Scene Hierarchy");
			if (panel_manager_get("Entity Inspector")) panels.emplace_back("Entity Inspector");
			if (panel_manager_get("Assets")) panels.emplace_back("Assets");
			if (panel_manager_get("Simulation Tools")) panels.emplace_back("Simulation Tools");

			file << ui32(panels.size());

			for (ui32 i = 0; i < panels.size(); ++i) {
				file << panels[i];
			}

			if (result_fail(bin_write(hash_string("Editor State"), file))) SV_LOG_ERROR("Can't serialize Editor State");
		}


		return Result_Success;
	}
	
	template<typename T>
	void panelCheckbox(const char* name) 
	{
		T* panel = (T*)panel_manager_get(name);
		if (ImGui::MenuItem(name, nullptr, nullptr, panel == nullptr)) {
			panel_manager_add(name, new T());
		}
	}

	void gui_begin()
	{

		if (ImGui::BeginMainMenuBar()) {

			ImGui::PushStyleColor(ImGuiCol_PopupBg, { 0.1f, 0.1f, 0.1f, 1.f });

			if (ImGui::BeginMenu("Panels")) {

				panelCheckbox<SimulationPanel>("Simulation");
				panelCheckbox<SceneEditorPanel>("Scene Editor");
				panelCheckbox<SceneHierarchyPanel>("Scene Hierarchy");
				panelCheckbox<EntityInspectorPanel>("Entity Inspector");
				panelCheckbox<AssetPanel>("Assets");
				panelCheckbox<SimulationToolsPanel>("Simulation Tools");

				ImGui::EndMenu();
			}

			ImGui::PopStyleColor();

			ImGui::EndMainMenuBar();
		}
	}

	void gui_end()
	{

	}

	void gui_style_update()
	{
		if (g_SimulationMode) {
			switch (g_Style)
			{
			case sv::GuiStyle_Black:
				ImVec4* colors = ImGui::GetStyle().Colors;
				colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
				colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 0.62f);
				colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.82f);
				colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.73f);
				colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.20f);
				colors[ImGuiCol_FrameBgHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
				colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
				colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
				colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
				colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
				colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.40f);
				colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
				colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.98f, 0.32f, 1.00f);
				colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.88f, 0.27f, 1.00f);
				colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.98f, 0.26f, 1.00f);
				colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 0.54f);
				colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
				colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
				colors[ImGuiCol_Header] = ImVec4(0.77f, 0.77f, 0.77f, 0.31f);
				colors[ImGuiCol_HeaderHovered] = ImVec4(0.66f, 0.66f, 0.66f, 0.43f);
				colors[ImGuiCol_HeaderActive] = ImVec4(0.63f, 0.63f, 0.63f, 0.63f);
				colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_SeparatorHovered] = ImVec4(0.58f, 0.58f, 0.58f, 0.78f);
				colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
				colors[ImGuiCol_ResizeGrip] = ImVec4(0.56f, 0.56f, 0.56f, 0.25f);
				colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.48f, 0.48f, 0.48f, 0.67f);
				colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
				colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 0.64f);
				colors[ImGuiCol_TabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
				colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_TabUnfocused] = ImVec4(0.30f, 0.30f, 0.30f, 0.44f);
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
				colors[ImGuiCol_DockingPreview] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
				colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
				colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
				colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.98f, 0.37f, 0.17f);
				colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
				colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
				break;
			}
		}
		else {
			switch (g_Style)
			{
			case sv::GuiStyle_Black:
				ImVec4* colors = ImGui::GetStyle().Colors;
				colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
				colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
				colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
				colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg] = ImVec4(0.24f, 0.24f, 0.24f, 0.54f);
				colors[ImGuiCol_FrameBgHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
				colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
				colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
				colors[ImGuiCol_TitleBgActive] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
				colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
				colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
				colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
				colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.98f, 0.32f, 1.00f);
				colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.88f, 0.27f, 1.00f);
				colors[ImGuiCol_SliderGrabActive] = ImVec4(0.28f, 0.98f, 0.26f, 1.00f);
				colors[ImGuiCol_Button] = ImVec4(0.24f, 0.24f, 0.24f, 0.54f);
				colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 0.40f);
				colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.44f, 0.44f, 0.67f);
				colors[ImGuiCol_Header] = ImVec4(0.77f, 0.77f, 0.77f, 0.31f);
				colors[ImGuiCol_HeaderHovered] = ImVec4(0.66f, 0.66f, 0.66f, 0.43f);
				colors[ImGuiCol_HeaderActive] = ImVec4(0.63f, 0.63f, 0.63f, 0.63f);
				colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_SeparatorHovered] = ImVec4(0.58f, 0.58f, 0.58f, 0.78f);
				colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
				colors[ImGuiCol_ResizeGrip] = ImVec4(0.56f, 0.56f, 0.56f, 0.25f);
				colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.48f, 0.48f, 0.48f, 0.67f);
				colors[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.50f, 0.50f, 0.95f);
				colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
				colors[ImGuiCol_TabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
				colors[ImGuiCol_TabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_TabUnfocused] = ImVec4(0.30f, 0.30f, 0.30f, 0.44f);
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
				colors[ImGuiCol_DockingPreview] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
				colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
				colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
				colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.98f, 0.37f, 0.17f);
				colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
				colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
				break;
			}
		}
	}

	void gui_style_set(GuiStyle style)
	{
		g_Style = style;
		gui_style_update();
	}

	GuiStyle gui_style_get()
	{
		return g_Style;
	}

	void gui_style_editor()
	{
		g_SimulationMode = false;
		gui_style_update();
	}

	void gui_style_simulation()
	{
		g_SimulationMode = true;
		gui_style_update();
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