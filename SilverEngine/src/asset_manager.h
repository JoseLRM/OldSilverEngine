#pragma once

#include "renderer.h"

namespace sv {

	typedef ui64 AssetID;

	AssetID			asset_manager_get_id(const char* assetPath);
	const char*		asset_manager_get_path(AssetID ID);
	void			asset_manager_refresh();

	SharedRef<Mesh> asset_manager_load_mesh(AssetID assetID);

}