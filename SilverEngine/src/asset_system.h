#pragma once

#include "renderer.h"

namespace sv {

	enum AssetType : ui32 {
		AssetType_Invalid,
		AssetType_Texture
	};

	struct Asset {
		AssetType assetType;
		SharedRef<ui32> ref;
	};

	struct TextureAsset {
		Texture	texture;
		size_t	hashCode;
	};

	Result	assets_refresh();

	Result assets_load_texture(const char* filePath, SharedRef<TextureAsset>& ref);

	Result assets_load_texture(size_t hashCode, SharedRef<TextureAsset>& ref);


	const std::map<std::string, Asset>& assets_map_get();

}