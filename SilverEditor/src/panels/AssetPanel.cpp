#include "core_editor.h"

#include "AssetPanel.h"
#include "gui.h"
#include "panel_manager.h"
#include "TextEditorPanel.h"
#include "rendering/material_system.h"

namespace fs = std::filesystem;

namespace sv {
	
	AssetPanel::AssetPanel() : Panel("Assets")
	{
		m_AssetFolder.name = "assets";
		m_AssetFolder.path = "assets";

#ifdef SV_RES_PATH
		m_AssetFolder.path = SV_RES_PATH + m_AssetFolder.name;
#endif
		m_CurrentFolder = &m_AssetFolder;

		refreshFolder(m_AssetFolder);
	}

	bool AssetPanel::onDisplay()
	{
		if (m_CurrentFolder == nullptr) m_CurrentFolder = &m_AssetFolder;

		float totalWidth = ImGui::GetWindowSize().x;

		// FOLDER ROOT
		ImGui::Columns(2u);
		{
			float rootSize = ImGui::GetColumnWidth(ImGui::GetColumnIndex());
			ImGui::SetColumnWidth(ImGui::GetColumnIndex(), std::min(rootSize, totalWidth * 0.1f));
			AssetEditorFolder* newCurrentFolder = folderTree(&m_AssetFolder);
			if (newCurrentFolder) m_CurrentFolder = newCurrentFolder;
		}
		ImGui::NextColumn();

		if (ImGui::BeginChild(6969)) {
			ui32 columns = ui32(std::max(ImGui::GetWindowSize().x / 105.f, 1.f));
			ui32 count = columns;
			ImGui::Columns(columns, 0, false);

			bool shouldRefresh = false;

			// ASSETS
			for (const auto& folder : m_CurrentFolder->folders) {
				if (ImGui::Button(folder->name.c_str(), { 100.f, 70.f })) {
					m_CurrentFolder = folder.get();
				}

				if (count == 0u) {
					ImGui::Columns(columns, 0, false);
					count = columns;
				}
				count--;
				ImGui::NextColumn();
			}

			for (const auto& file : m_CurrentFolder->files) {
				if (ImGui::Button(file.name.c_str(), { 100.f, 70.f })) {

					if (strcmp(file.extension.c_str(), "shader") == 0) {
						SV_LOG("Alo");
					}

				}

				if (ImGui::BeginPopupContextItem(file.name.c_str(), ImGuiMouseButton_Right)) {

					if (ImGui::MenuItem("Rename")) {

					}

					if (ImGui::MenuItem("Delete")) {
						fs::remove(file.folder->path + "/" + file.name);
						shouldRefresh = true;
					}

					ImGui::EndPopup();
				}

				if (count == 0u) {
					ImGui::Columns(columns, 0, false);
					count = columns;
				}
				count--;
				ImGui::NextColumn();
			}

			while (count--) ImGui::NextColumn();

			if (shouldRefresh) refresh();
		}

		// Asset options

		static std::string nameSelection;
		static const char* assetName = nullptr;

		enum Option {
			Option_None,
			Option_CreateFolder,
			Option_CreateMaterial,
			Option_CreateEmptyShader,
			Option_CreateSpriteShader,
			Option_CreateSpriteAnimation,
		};

		Option option = Option_None;

		if (ImGui::BeginPopupContextWindow("AssetPopup", 1, false)) {

			if (ImGui::MenuItem("Create Folder")) {
				option = Option_CreateFolder;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Create material")) {
				option = Option_CreateMaterial;
			}
			if (ImGui::BeginMenu("Create shader library")) {

				if (ImGui::MenuItem("Empty Shader")) option = Option_CreateEmptyShader;
				if (ImGui::MenuItem("Sprite Shader")) option = Option_CreateSpriteShader;

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Create Animation")) {

				if (ImGui::MenuItem("Sprite Animation")) option = Option_CreateSpriteAnimation;

				ImGui::EndMenu();
			}

			ImGui::EndPopup();
		}

		ImGui::EndChild();

		// Do the option

		switch (option)
		{
		case Option_CreateFolder:
			ImGui::OpenPopup("CreateFolderPopup");
			break;
		case Option_CreateMaterial:
			ImGui::OpenPopup("CreateMaterialPopup");
			break;
		case Option_CreateEmptyShader:
			ImGui::OpenPopup("CreateEmptyShaderPopup");
			break;
		case Option_CreateSpriteShader:
			SV_LOG_ERROR("For now you can't do that");
			break;
		case Option_CreateSpriteAnimation:
			ImGui::OpenPopup("CreateSpriteAnimationPopup");
			break;
		}

		// CREATE FOLDER
		if (ImGui::BeginPopup("CreateFolderPopup")) {

			gui_component_item_string(nameSelection);

			if (ImGui::Button("Done"))
			{
				std::string filePath = m_CurrentFolder->path;
				filePath += '/';
				filePath += nameSelection;

				fs::create_directories(filePath);

				refresh();

				nameSelection = "";
			}

			ImGui::EndPopup();
		}

		// CREATE MATERIAL
		if (ImGui::BeginPopup("CreateMaterialPopup")) {

			gui_component_item_string(nameSelection);

			sv::AssetType type = asset_type_get("Shader Library");

			if (ImGui::BeginCombo("Select Shader", "##nothingxd")) {
				
				auto& files = asset_map_get();

				for (auto& file : files) {
					if (file.second.assetType == type) {
						if (ImGui::MenuItem(file.first.c_str())) {
							assetName = file.first.c_str();
						}
					}
				}

				ImGui::EndCombo();
			}

			if (ImGui::Button("Done"))
			{
				ShaderLibraryAsset lib;
				lib.load(assetName);

				MaterialAsset mat;
				std::string filePath = m_CurrentFolder->path;
				filePath += '/';
				filePath += nameSelection;
				mat.createFile(filePath.c_str() + asset_folderpath_get().size(), lib);

				refresh();

				assetName = nullptr;
				nameSelection = "";
			}

			ImGui::EndPopup();
		}


		// CREATE EMPTY SHADER
		if (ImGui::BeginPopup("CreateEmptyShaderPopup")) {

			gui_component_item_string(nameSelection);

			if (ImGui::Button("Done"))
			{
				std::string filePath = m_CurrentFolder->path;
				filePath += '/';
				filePath += nameSelection;

				std::ofstream file(filePath);

				if (!file.is_open()) {
					SV_LOG_ERROR("Can't create empty shader at '%s'", filePath.c_str());
				}

				file << "#name " << nameSelection;

				file.close();

				refresh();

				nameSelection = "";
			}

			ImGui::EndPopup();
		}

		// CREATE SPRITE ANIMATION
		if (ImGui::BeginPopup("CreateSpriteAnimationPopup")) {

			gui_component_item_string(nameSelection);

			if (ImGui::Button("Done"))
			{
				std::string filePath = m_CurrentFolder->path;
				filePath += '/';
				filePath += nameSelection;

				ArchiveO file;

				file << ui32(1u) << size_t(0u) << vec4f(0.f, 0.f, 1.f, 1.f);

				file.save_file(filePath.c_str());

				refresh();

				nameSelection = "";
			}

			ImGui::EndPopup();
		}

		return true;
	}

	void AssetPanel::refresh()
	{
		refreshFolder(m_AssetFolder);
		m_CurrentFolder = &m_AssetFolder;
	}
	void AssetPanel::refreshFolder(AssetEditorFolder& folder)
	{
		folder.files.clear();
		folder.folders.clear();

		const char* folderPath = folder.path.c_str();

#ifdef SV_RES_PATH
		std::string folderPathStr = SV_RES_PATH;
		folderPathStr += folderPath;
		folderPath = folderPathStr.c_str();
#endif

		auto iterator = fs::directory_iterator(folderPath);

		for (const auto& item : iterator) {

			if (item.is_directory()) {

				std::unique_ptr<AssetEditorFolder>& newFolder = folder.folders.emplace_back();
				newFolder = std::make_unique<AssetEditorFolder>();
#ifdef SV_RES_PATH
				newFolder->path = parse_string(item.path().c_str() + strlen(SV_RES_PATH));
#else
				newFolder->path = parse_string(item.path().c_str());
#endif
				newFolder->name = parse_string(item.path().filename().c_str());
				newFolder->folder = &folder;
				refreshFolder(*newFolder.get());

				// Clear path
				for (auto& c : newFolder->path) {
					if (c == '\\') c = '/';
				}

				if (newFolder->path.back() == '\0') newFolder->path.pop_back();

			}
			else {

				AssetEditorFile& file = folder.files.emplace_back();
				file.name = parse_string(item.path().filename().c_str());
				std::wstring ext = item.path().extension();
				if (ext.size())
					file.extension = parse_string(ext.c_str() + 1);
				file.folder = &folder;

			}

		}
	}
	AssetEditorFolder* AssetPanel::folderTree(AssetEditorFolder* folder)
	{
		AssetEditorFolder* res = nullptr;

		bool actived = ImGui::TreeNodeEx(folder->name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow);

		if (ImGui::IsItemClicked()) res = folder;

		if (actived) {

			for (const std::unique_ptr<AssetEditorFolder>& f : folder->folders) {
				AssetEditorFolder* newFolder = folderTree(f.get());
				if (newFolder) res = newFolder;
			}

			for (const AssetEditorFile& file : folder->files) {
				if (ImGui::Selectable(file.name.c_str())) {
				}
			}

			ImGui::TreePop();
		}

		return res;
	}
}