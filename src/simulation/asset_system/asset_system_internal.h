#pragma once

#include "simulation/asset_system.h"
#include "utils/allocator.h"

namespace sv {

#define MAX_SIZE (const char*)(SIZE_MAX)

	// Time to update one asset type, after this time will update the next type
	constexpr float UNUSED_CHECK_TIME = 1.f;

	struct AssetType_internal;

	// Remember that exist a copy of this struct in the header file. If this is modified the other must coincide to work properly
	struct Asset_internal {
		
		std::atomic<int>	refCount = 0;
		float				unusedTime = f32_max;

		/*
			Used to know the attachment type:
			- nullptr: attached to ID
			- size_max: attached to anything
			- else: attached to filepath, it is the path ptr
		*/
		const char*			filePath = nullptr; 
		size_t				hashCode = 0u; // If isn't attached to a file it stores the ID
		AssetType_internal* assetType = nullptr;

	};

	struct AssetType_internal {

		std::string									name;
		size_t										hashCode;
		std::vector<std::string>					extensions;
		AssetLoadFromFileFn							loadFileFn;
		AssetLoadFromIDFn							loadIDFn;
		AssetCreateFn								createFn;
		AssetDestroyFn								destroyFn;
		AssetLoadFromFileFn							reloadFileFn;
		AssetSerializeFn							serializeFn;
		AssetIsUnusedFn								isUnusedFn;
		float										unusedLifeTime;
		SizedInstanceAllocator						allocator;
		std::vector<Asset_internal*>				activeAssets;
		std::unordered_map<size_t, Asset_internal*>	idMap;

		AssetType_internal(size_t assetSize) : allocator(assetSize, 200u) {}

	};

	Result	asset_initialize(const char* assetsFolderPath);
	Result	asset_close();
	void	asset_update(float dt);

}