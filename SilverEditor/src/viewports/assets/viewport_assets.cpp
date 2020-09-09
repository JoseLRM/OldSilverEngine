#include "core_editor.h"

#include "viewports.h"
#include "asset_system.h"
#include "viewports/viewport_assets.h"

namespace sve {

	namespace fs = std::filesystem;

	enum AssetType : ui32 {
		AssetType_Folder,
		AssetType_File,
	};

	struct AssetFolder;

	struct AssetItem {
		const AssetType		type;
		std::wstring		name;
		AssetFolder*		parent = nullptr;
		AssetItem(AssetType type) : type(type) {}
	};

	struct AssetFile : public AssetItem {
		std::wstring extension;

		AssetFile() : AssetItem(AssetType_File)
		{}
	};

	struct AssetFolder : public AssetItem {
		std::wstring								path;
		std::vector<std::pair<ui32, AssetType>>		items;
		std::vector<AssetFile>						files;
		std::vector<std::unique_ptr<AssetFolder>>	folders;

		AssetFolder() : AssetItem(AssetType_Folder) 
		{}
	};

	// Runtime Structures

	struct FolderRoot {
		std::string		name;
		AssetFolder* pFolder;
	};

	struct RuntimeFolderRoot {
		std::vector<FolderRoot> folders;
	};

	// Globals

	static AssetFolder	g_AssetFolder;
	static AssetFolder* g_CurrentFolder = nullptr;

	static RuntimeFolderRoot g_RuntimeFolderRoot;

	void viewport_assets_generate_runtime()
	{
		g_RuntimeFolderRoot.folders.clear();

		ui32 foldersCount = 0u;
		AssetFolder* next = g_CurrentFolder;
		while (next != nullptr) {
			foldersCount++;
			next = next->parent;
		}

		g_RuntimeFolderRoot.folders.resize(foldersCount);
		next = g_CurrentFolder;
		while (next != nullptr) {

			FolderRoot& root = g_RuntimeFolderRoot.folders[--foldersCount];

			root.name = sv::utils_string_parse(next->name.c_str());
			root.pFolder = next;

			next = next->parent;
		}
	}

	void viewport_assets_refresh_folder(AssetFolder& folder)
	{
		folder.files.clear();
		folder.folders.clear();
		folder.items.clear();

		auto iterator = fs::directory_iterator(folder.path.c_str());

		for (const auto& item : iterator) {

			std::pair<ui32, AssetType> pair;
			
			if (item.is_directory()) {

				pair.first = ui32(folder.folders.size());
				pair.second = AssetType_Folder;
				std::unique_ptr<AssetFolder>& newFolder = folder.folders.emplace_back();
				newFolder = std::make_unique<AssetFolder>();
				newFolder->path = item.path().c_str();
				newFolder->name = item.path().filename().c_str();
				newFolder->parent = &folder;
				viewport_assets_refresh_folder(*newFolder.get());
				
			}
			else {

				pair.first = ui32(folder.files.size());
				pair.second = AssetType_File;
				AssetFile& file = folder.files.emplace_back();
				file.name = item.path().filename().c_str();
				file.extension = item.path().extension().c_str();
				file.parent = &folder;

			}

			folder.items.emplace_back(pair);
			
		}

		viewport_assets_generate_runtime();
	}

	bool viewport_assets_display()
	{
		static bool firstCall = true;
		if (firstCall) {
			g_AssetFolder.name = L"assets";
			g_AssetFolder.path = L"assets";
			
#ifdef SV_SRC_PATH
			g_AssetFolder.path = SV_SRC_PATH_W + g_AssetFolder.name;
#endif
			g_CurrentFolder = &g_AssetFolder;

			viewport_assets_refresh_folder(g_AssetFolder);

			firstCall = false;
		}

		if (g_CurrentFolder == nullptr) g_CurrentFolder = &g_AssetFolder;
		AssetFolder* nextFolder = g_CurrentFolder;

		if (ImGui::Begin(viewports_get_name(SVE_VIEWPORT_ASSETS))) {

			// POPUP
			if (ImGui::BeginPopupContextWindow("Asset popup", ImGuiMouseButton_Right)) {

				if (ImGui::Button("Refresh")) {
					viewport_assets_refresh_folder(g_AssetFolder);
					SV_ASSERT(sv::assets_refresh() == sv::Result_Success);
				}
					
				ImGui::EndPopup();
			}

			// FOLDER ROOT
			for (ui32 i = 0u; i < g_RuntimeFolderRoot.folders.size(); ++i) {
				auto& root = g_RuntimeFolderRoot.folders[i];
				bool end = i == (g_RuntimeFolderRoot.folders.size() - 1u);

				if (ImGui::Button(root.name.c_str())) {
					nextFolder = root.pFolder;
					break;
				}
				if (!end) ImGui::SameLine();
			}

			ImGui::Separator();

			// ASSETS
			ImGui::Columns(6);

			for (auto [index, type] : g_CurrentFolder->items) {

				switch (type)
				{
				case AssetType_File:
				{
					AssetFile& file = g_CurrentFolder->files[index];
					std::string name = sv::utils_string_parse(file.name.c_str());
					ImGui::Button(name.c_str());
				}
				break;
				case AssetType_Folder:
				{
					std::unique_ptr<AssetFolder>& folder = g_CurrentFolder->folders[index];
					std::string name = sv::utils_string_parse(folder->name.c_str());
					if (ImGui::Button(name.c_str())) {
						nextFolder = folder.get();
					}
				}
				break;
				}

				ImGui::NextColumn();

			}

			if (g_CurrentFolder != nextFolder) {
				g_CurrentFolder = nextFolder;

				viewport_assets_generate_runtime();
			}

		}

		ImGui::End();

		return true;
	}

}