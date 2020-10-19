#pragma once

#include "high_level/asset_system.h"

namespace sv {

	constexpr float ASSET_LIFETIME = 10.f;

	struct TextureAsset_internal : public AssetRef {

		GPUImage*	image = nullptr;

	};

	struct ShaderLibraryAsset_internal : public AssetRef {

		ShaderLibrary shaderLibrary;

	};

	struct MaterialAsset_internal : public AssetRef {

		Material material;

	};

	Result	assets_initialize(const char* assetsFolderPath);
	Result	assets_close();
	void	assets_update(float dt);

	Result assets_destroy(TextureAsset_internal* texture);
	Result assets_destroy(ShaderLibraryAsset_internal* library);
	Result assets_destroy(MaterialAsset_internal* material);

}