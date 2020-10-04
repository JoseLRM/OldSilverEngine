#pragma once

#include "asset_system.h"

#define svThrow(x) SV_THROW("ASSET_SYSTEM_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Green, "[ASSET_SYSTEM] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Green, "[ASSET_SYSTEM_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[ASSET_SYSTEM_ERROR] "#x, __VA_ARGS__)

namespace sv {

	struct TextureAsset_internal : public AssetRef {

		GPUImage*	image = nullptr;
		size_t		hashCode = 0u;

	};

	struct ShaderLibraryAsset_internal : public AssetRef {

		ShaderLibrary*	library = nullptr;
		std::string		shaderName; // package + name
		size_t			id; // shaderName hascode used to serialize materials
		size_t			hashCode = 0u;

	};

	struct MaterialAsset_internal : public AssetRef {

		Material*	material = nullptr;
		size_t		hashCode = 0u;
		size_t		shaderID = 0u;

	};

	Result assets_initialize(const char* assetsFolderPath);
	Result assets_close();
	void assets_update(float dt);

	Result assets_destroy(TextureAsset_internal* texture);
	Result assets_destroy(MaterialAsset_internal* material);
	Result assets_destroy(ShaderLibraryAsset_internal* library);

}