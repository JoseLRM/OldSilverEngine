#pragma once

#include "simulation/asset_system.h"
#include "utils/allocator.h"

namespace sv {

	// Time to update one asset type, after this time will update the next type
	constexpr float UNUSED_CHECK_TIME = 1.f;

	struct AssetType_internal;

	struct Asset_internal {
		
		std::atomic<int>	refCount = 0;
		float				unusedTime = float_max;
		const char*			filePath = nullptr;
		size_t				hashCode = 0u;
		AssetType_internal* assetType = nullptr;

	};

	struct AssetType_internal {

		std::string						name;
		size_t							hashCode;
		std::vector<std::string>		extensions;
		AssetCreateFn					createFn;
		AssetDestroyFn					destroyFn;
		AssetCreateFn					recreateFn;
		AssetIsUnusedFn					isUnusedFn;
		float							unusedLifeTime;
		SizedInstanceAllocator			allocator;
		std::vector<Asset_internal*>	activeAssets;

		AssetType_internal(size_t assetSize) : allocator(assetSize) {}

	};

	Result	asset_initialize(const char* assetsFolderPath);
	Result	asset_close();
	void	asset_update(float dt);

}