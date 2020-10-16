#pragma once

#include "Viewport.h"
#include "high_level/asset_system.h"

namespace sve {

	struct AssetFolder;

	struct AssetFile {
		std::string		name;
		std::string		extension;
		AssetFolder* folder = nullptr;
	};

	struct AssetFolder {
		std::string									name;
		std::string									path;
		std::vector<AssetFile>						files;
		std::vector<std::unique_ptr<AssetFolder>>	folders;
		AssetFolder*								folder = nullptr;
	};

	class AssetViewport : public Viewport {

	public:
		AssetViewport();

	protected:
		bool onDisplay() override;

	private:
		void refresh();
		void refreshFolder(AssetFolder& folder);
		AssetFolder* folderTree(AssetFolder* folder);

	private:
		AssetFolder		m_AssetFolder;
		AssetFolder*	m_CurrentFolder = nullptr;

	};

}