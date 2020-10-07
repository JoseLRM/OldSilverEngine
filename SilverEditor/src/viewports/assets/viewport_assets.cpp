#include "core_editor.h"

#include "viewports.h"
#include "asset_system.h"
#include "viewports/viewport_assets.h"

namespace sve {

	namespace fs = std::filesystem;

	struct AssetFolder;

	struct AssetFile {
		std::string		name;
		std::string		extension;
		AssetFolder*	folder = nullptr;
	};

	struct AssetFolder {
		std::string									name;
		std::string									path;
		std::vector<AssetFile>						files;
		std::vector<std::unique_ptr<AssetFolder>>	folders;
		AssetFolder*								folder = nullptr;
	};

	// Globals

	static AssetFolder	g_AssetFolder;
	static AssetFolder* g_CurrentFolder = nullptr;

	void viewport_assets_refresh_folder(AssetFolder& folder)
	{
		folder.files.clear();
		folder.folders.clear();

		auto iterator = fs::directory_iterator(folder.path.c_str());

		for (const auto& item : iterator) {

			if (item.is_directory()) {

				std::unique_ptr<AssetFolder>& newFolder = folder.folders.emplace_back();
				newFolder = std::make_unique<AssetFolder>();
				newFolder->path = sv::utils_string_parse(item.path().c_str());
				newFolder->name = sv::utils_string_parse(item.path().filename().c_str());
				newFolder->folder = &folder;
				viewport_assets_refresh_folder(*newFolder.get());
				
			}
			else {

				AssetFile& file = folder.files.emplace_back();
				file.name = sv::utils_string_parse(item.path().filename().c_str());
				file.extension = sv::utils_string_parse(item.path().extension().c_str());
				file.folder = &folder;

			}

		}
	}

	void viewport_assets_refresh()
	{
		viewport_assets_refresh_folder(g_AssetFolder);
		g_CurrentFolder = &g_AssetFolder;
	}

	AssetFolder* viewport_assets_folder_tree(AssetFolder* folder)
	{
		AssetFolder* res = nullptr;

		bool actived = ImGui::TreeNodeEx(folder->name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow); 
		
		if (ImGui::IsItemClicked()) res = folder;

		if (actived) {

			for (const std::unique_ptr<AssetFolder>& f : folder->folders) {
				AssetFolder* newFolder = viewport_assets_folder_tree(f.get());
				if (newFolder) res = newFolder;
			}

			for (const AssetFile& file : folder->files) {
				if (ImGui::Selectable(file.name.c_str())) {
				}
			}

			ImGui::TreePop();
		}

		return res;
	}

	bool viewport_assets_display()
	{
		static bool firstCall = true;
		if (firstCall) {
			g_AssetFolder.name = "assets";
			g_AssetFolder.path = "assets";
			
#ifdef SV_SRC_PATH
			g_AssetFolder.path = SV_SRC_PATH + g_AssetFolder.name;
#endif
			g_CurrentFolder = &g_AssetFolder;

			viewport_assets_refresh_folder(g_AssetFolder);

			firstCall = false;
		}

		if (g_CurrentFolder == nullptr) g_CurrentFolder = &g_AssetFolder;

		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_ASSETS))) {

			// POPUP
			if (ImGui::BeginPopupContextWindow("Asset popup", ImGuiMouseButton_Right)) {
					
				ImGui::EndPopup();
			}

			float totalWidth = ImGui::GetWindowSize().x;

			// FOLDER ROOT
			ImGui::Columns(2u);
			{
				float rootSize = ImGui::GetColumnWidth(ImGui::GetColumnIndex());
				ImGui::SetColumnWidth(ImGui::GetColumnIndex(), std::min(rootSize, totalWidth * 0.1f));
				AssetFolder* newCurrentFolder = viewport_assets_folder_tree(&g_AssetFolder);
				if (newCurrentFolder) g_CurrentFolder = newCurrentFolder;
			}
			ImGui::NextColumn();

			if (ImGui::BeginChild(6969)) {
				ui32 columns = std::max(ImGui::GetWindowSize().x / 105.f, 1.f);
				ui32 count = columns;
				ImGui::Columns(columns, 0, false);

				// ASSETS
				for (const auto& folder : g_CurrentFolder->folders) {
					if (ImGui::Button(folder->name.c_str(), { 100.f, 70.f })) {
						g_CurrentFolder = folder.get();
					}

					if (count == 0u) {
						ImGui::Columns(columns, 0, false);
						count = columns;
					}
					count--;
					ImGui::NextColumn();
				}

				for (const auto& file : g_CurrentFolder->files) {
					if (ImGui::Button(file.name.c_str(), { 100.f, 70.f })) {
					}
					if (count == 0u) {
						ImGui::Columns(columns, 0, false);
						count = columns;
					}
					count--;
					ImGui::NextColumn();
				}

				while (count--) ImGui::NextColumn();

			}
			ImGui::EndChild();

			if (ImGui::BeginPopupContextWindow("AssetPopup")) {

				if (ImGui::Button("Create material")) {
					
					sv::MaterialAsset mat;
					

				}
				if (ImGui::Button("Create shader library"));

				ImGui::EndPopup();
			}

		}

		ImGui::End();

		return true;
	}

	bool viewport_assets_texture_popmenu(const char** pName)
	{
		bool res = true;
		*pName = nullptr;

		if (ImGui::Begin("TexturePopup", 0, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar)) {

			auto& assets = sv::assets_registers_get();
			for (auto it = assets.begin(); it != assets.end(); ++it) {

				if (it->second.assetType == sv::AssetType_Texture) {
					if (ImGui::Button(it->first.c_str())) {

						*pName = it->first.c_str();

						res = false;
						break;
					}
				}
			}

			if (!ImGui::IsWindowFocused()) res = false;
		}
		ImGui::End();
		return res;
	}

}