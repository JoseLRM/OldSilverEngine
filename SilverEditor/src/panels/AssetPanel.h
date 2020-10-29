#pragma once

#include "Panel.h"
#include "simulation/asset_system.h"

namespace sv {

	struct AssetEditorFolder;

	struct AssetEditorFile {
		std::string		name;
		std::string		extension;
		AssetEditorFolder* folder = nullptr;
	};

	struct AssetEditorFolder {
		std::string										name;
		std::string										path;
		std::vector<AssetEditorFile>					files;
		std::vector<std::unique_ptr<AssetEditorFolder>>	folders;
		AssetEditorFolder*								folder = nullptr;
	};

	class AssetPanel : public Panel {

	public:
		AssetPanel();

	protected:
		bool onDisplay() override;

	private:
		void refresh();
		void refreshFolder(AssetEditorFolder& folder);
		AssetEditorFolder* folderTree(AssetEditorFolder* folder);

	private:
		AssetEditorFolder	m_AssetFolder;
		AssetEditorFolder*	m_CurrentFolder = nullptr;

	};

}