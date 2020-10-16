#include "core_editor.h"
#include "AssetViewport.h"

namespace fs = std::filesystem;

namespace sve {
	
	AssetViewport::AssetViewport() : Viewport("Assets")
	{
		m_AssetFolder.name = "assets";
		m_AssetFolder.path = "assets";

#ifdef SV_SRC_PATH
		m_AssetFolder.path = SV_SRC_PATH + m_AssetFolder.name;
#endif
		m_CurrentFolder = &m_AssetFolder;

		refreshFolder(m_AssetFolder);
	}

	bool AssetViewport::onDisplay()
	{
		if (m_CurrentFolder == nullptr) m_CurrentFolder = &m_AssetFolder;

		float totalWidth = ImGui::GetWindowSize().x;

		// FOLDER ROOT
		ImGui::Columns(2u);
		{
			float rootSize = ImGui::GetColumnWidth(ImGui::GetColumnIndex());
			ImGui::SetColumnWidth(ImGui::GetColumnIndex(), std::min(rootSize, totalWidth * 0.1f));
			AssetFolder* newCurrentFolder = folderTree(&m_AssetFolder);
			if (newCurrentFolder) m_CurrentFolder = newCurrentFolder;
		}
		ImGui::NextColumn();

		if (ImGui::BeginChild(6969)) {
			ui32 columns = std::max(ImGui::GetWindowSize().x / 105.f, 1.f);
			ui32 count = columns;
			ImGui::Columns(columns, 0, false);

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

		return true;
	}

	void AssetViewport::refresh()
	{
		refreshFolder(m_AssetFolder);
		m_CurrentFolder = &m_AssetFolder;
	}
	void AssetViewport::refreshFolder(AssetFolder& folder)
	{
		folder.files.clear();
		folder.folders.clear();

		auto iterator = fs::directory_iterator(folder.path.c_str());

		for (const auto& item : iterator) {

			if (item.is_directory()) {

				std::unique_ptr<AssetFolder>& newFolder = folder.folders.emplace_back();
				newFolder = std::make_unique<AssetFolder>();
				newFolder->path = sv::parse_string(item.path().c_str());
				newFolder->name = sv::parse_string(item.path().filename().c_str());
				newFolder->folder = &folder;
				refreshFolder(*newFolder.get());

			}
			else {

				AssetFile& file = folder.files.emplace_back();
				file.name = sv::parse_string(item.path().filename().c_str());
				file.extension = sv::parse_string(item.path().extension().c_str());
				file.folder = &folder;

			}

		}
	}
	AssetFolder* AssetViewport::folderTree(AssetFolder* folder)
	{
		AssetFolder* res = nullptr;

		bool actived = ImGui::TreeNodeEx(folder->name.c_str(), ImGuiTreeNodeFlags_OpenOnArrow);

		if (ImGui::IsItemClicked()) res = folder;

		if (actived) {

			for (const std::unique_ptr<AssetFolder>& f : folder->folders) {
				AssetFolder* newFolder = folderTree(f.get());
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
}