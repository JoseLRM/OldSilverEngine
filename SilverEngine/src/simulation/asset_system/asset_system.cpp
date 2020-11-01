#include "core.h"

#include "asset_system_internal.h"

namespace fs = std::filesystem;

namespace sv {

	static std::vector<std::unique_ptr<AssetType_internal>> g_Types;

	static std::unordered_map<std::string, AssetType_internal*> g_Extensions;
	static std::unordered_map<size_t, const char*>				g_HashCodes;

	// Asset file loading
	static std::string									g_FolderPath;
	static std::unordered_map<std::string, AssetFile>	g_Files;
	static ui32											g_RefreshID = 0u; // ID used to know if a file is removed

	// Asset Unused
	static float g_UnusedTimeCount = 0.f;
	static ui32 g_UnusedIndex = 0u;

	Result asset_initialize(const char* assetsFolderPath)
	{
		SV_ASSERT(assetsFolderPath);

		// Set folder filepath
		g_FolderPath = assetsFolderPath;

		return Result_Success;
	}

	Result asset_close()
	{
		for (std::unique_ptr<AssetType_internal>& type : g_Types) {
			for (Asset_internal* asset : type->activeAssets) {
				type->destroyFn((ui8*)(asset) + sizeof(Asset_internal));
				type->allocator.free(asset);
			}

			SV_ASSERT(!type->allocator.has_unfreed());
			type->allocator.clear();
		}

		g_Types.clear();
		g_Extensions.clear();
		g_HashCodes.clear();
		g_FolderPath.clear();
		g_Files.clear();
		g_RefreshID = 0u;

		return Result_Success;
	}

	Result asset_destroy(AssetType_internal& type,  Asset_internal* asset, void* pObject)
	{
		Result res = type.destroyFn(pObject);
		SV_ASSERT(result_okay(res));

		if (result_fail(res)) {
			SV_LOG_ERROR("Can't destroy the %s asset: '%s'. ErrorCode: %u", type.name.c_str(), asset->filePath, res);
		}
		else {
			SV_LOG_INFO("%s asset freed: '%s'", type.name.c_str(), asset->filePath);
		}

		g_Files[asset->filePath].pInternalAsset = nullptr;
		type.allocator.free(asset);

		return res;
	}

	void asset_update(float dt)
	{
		g_UnusedTimeCount += dt;

		if (g_UnusedTimeCount > UNUSED_CHECK_TIME) {
			g_UnusedTimeCount -= UNUSED_CHECK_TIME;

			float now = timer_now();
			AssetType_internal& type = *g_Types[g_UnusedIndex].get();

			for (auto it = type.activeAssets.begin(); it != type.activeAssets.end(); ) {

				SV_ASSERT(*it);
				Asset_internal* asset = *it;

				if (asset->refCount.load() <= 0) {

					void* pObject = (ui8*)asset + sizeof(Asset_internal);
					if (!type.isUnusedFn || (type.isUnusedFn && type.isUnusedFn(pObject))) {

						if (asset->unusedTime == float_max) {

							asset->unusedTime = timer_now();
							
						}
						else if (now - asset->unusedTime >= type.unusedLifeTime) {

							asset_destroy(type, asset, pObject);
							it = type.activeAssets.erase(it);
							continue;
						}
					}
				}

				++it;
			}

			g_UnusedIndex = (g_UnusedIndex + 1) % ui32(g_Types.size());
		}
	}

	Result assets_refresh_folder(const char* folderPath)
	{
		auto it = fs::directory_iterator(folderPath);

		for (auto& file : it) {

			if (file.is_directory()) {
				svCheck(assets_refresh_folder(file.path().string().c_str()));
			}
			else {

				size_t folderPathSize = g_FolderPath.size();
#ifdef SV_RES_PATH
				folderPathSize += strlen(SV_RES_PATH);
#endif

				std::string path = std::move(file.path().string());
				if (path.size() <= folderPathSize) continue;
				path = path.c_str() + folderPathSize;

				// Prepare path
				for (auto it0 = path.begin(); it0 != path.end(); ++it0) {
					if (*it0 == '\\') {
						*it0 = '/';
					}
				}

				auto find = g_Files.find(path);
				
				// File not found, create asset file
				if (find == g_Files.end()) {

					// Check if has available extensions
					std::string extension = file.path().extension().string();
					if (extension.size() <= 1u) continue;

					auto findExt = g_Extensions.find(extension.c_str() + 1u);
					if (findExt == g_Extensions.end()) continue;

					AssetFile assetFile;
					assetFile.assetType = findExt->second;
					assetFile.pInternalAsset = nullptr;
					assetFile.lastModification = fs::last_write_time(file);
					assetFile.refreshID = g_RefreshID;
					
					g_Files[path] = assetFile;

					// Add hashcode
					g_HashCodes[hash_string(path.c_str())] = g_Files.find(path)->first.c_str();

				}
				// File found, check if it is active
				else {

					// Set the refresh ID
					find->second.refreshID = g_RefreshID;

					// if it's active, recreate and it is modified
					auto lastModification = fs::last_write_time(file);
					if (find->second.pInternalAsset && lastModification != find->second.lastModification) {

						find->second.lastModification = lastModification;

						AssetType_internal* type = reinterpret_cast<AssetType_internal*>(find->second.assetType);
						SV_ASSERT(type);

						std::string absFilePath = g_FolderPath + path;
						Result res = type->recreateFn(absFilePath.c_str(), (ui8*)(find->second.pInternalAsset) + sizeof(Asset_internal));

						if (result_okay(res)) {
							SV_LOG_INFO("%s asset updated: '%s'", type->name.c_str(), reinterpret_cast<Asset_internal*>(find->second.pInternalAsset)->filePath);
						}
						else {
							SV_LOG_ERROR("fail to update a %s asset: '%s'", type->name.c_str(), reinterpret_cast<Asset_internal*>(find->second.pInternalAsset)->filePath);
						}
					}
				}
			}
		}
		return Result_Success;
	}

	Result asset_refresh()
	{
		++g_RefreshID;

		const char* folderPath = g_FolderPath.c_str();

#ifdef SV_RES_PATH
		std::string folderPathStr = SV_RES_PATH + g_FolderPath;
		folderPath = folderPathStr.c_str();
#endif

		svCheck(assets_refresh_folder(folderPath));

		// Check if has removed files
		for (auto it = g_Files.begin(); it != g_Files.end(); ) {
			if (it->second.refreshID != g_RefreshID) {
				g_HashCodes.erase(hash_string(it->first.c_str()));
				it = g_Files.erase(it);
			}
			else ++it;
		}

		return Result_Success;
	}

	void asset_free_unused()
	{
		for (auto it = g_Types.rbegin(); it != g_Types.rend(); ++it) {
			asset_free_unused(reinterpret_cast<AssetType>((*it).get()));
		}
	}

	void asset_free_unused(AssetType assetType)
	{
		AssetType_internal& type = *reinterpret_cast<AssetType_internal*>(assetType);

		for (auto it = type.activeAssets.begin(); it != type.activeAssets.end(); ) {

			SV_ASSERT(*it);
			Asset_internal* asset = *it;

			if (asset->refCount.load() <= 0) {

				void* pObject = (ui8*)asset + sizeof(Asset_internal);
				if (!type.isUnusedFn || (type.isUnusedFn && type.isUnusedFn(pObject))) {

					asset_destroy(type, asset, pObject);
					it = type.activeAssets.erase(it);
					continue;

				}
			}

			++it;
		}
	}

	Result asset_register_type(const AssetRegisterTypeDesc* desc, AssetType* pAssetType)
	{
		SV_ASSERT(desc->name);
		SV_ASSERT(desc->pExtensions);
		SV_ASSERT(desc->extensionsCount);
		SV_ASSERT(desc->createFn);
		SV_ASSERT(desc->destroyFn);
		SV_ASSERT(desc->recreateFn);
		SV_ASSERT(desc->assetSize);

		// Compute hashCode
		size_t hashCode = hash_string(desc->name);

		// Check if repeat extensions
		for (ui32 i = 0; i < desc->extensionsCount; ++i) {
			if (g_Extensions.find(desc->pExtensions[i]) != g_Extensions.end()) {

				SV_LOG_ERROR("The extension %s is in use", desc->pExtensions[i]);

				return Result_Duplicated;
			}
		}

		// Find if it exist
		for (auto& type : g_Types) {
			if (type->hashCode == hashCode) {

				SV_LOG_ERROR("The asset type %s it already exist", desc->name);

				return Result_Duplicated;
			}
		}

		// Create type
		std::unique_ptr<AssetType_internal>& type = g_Types.emplace_back();
		type = std::make_unique<AssetType_internal>(sizeof(Asset_internal) + desc->assetSize);

		type->name = desc->name;
		type->hashCode = hashCode;
		type->createFn = desc->createFn;
		type->destroyFn = desc->destroyFn;
		type->recreateFn = desc->recreateFn;
		type->isUnusedFn = desc->isUnusedFn;
		type->unusedLifeTime = desc->unusedLifeTime;

		type->extensions.resize(desc->extensionsCount);
		for (ui32 i = 0; i < desc->extensionsCount; ++i) {
			type->extensions[i] = desc->pExtensions[i];
		}

		if (pAssetType) *pAssetType = type.get();

		// Add extensions
		for (ui32 i = 0; i < desc->extensionsCount; ++i) {
			g_Extensions[desc->pExtensions[i]] = type.get();
		}

		asset_refresh();
		return Result_Success;
	}

	AssetType asset_type_get(const char* name)
	{
		for (std::unique_ptr<AssetType_internal>& type : g_Types) {
			if (strcmp(type->name.c_str(), name) == 0) return type.get();
		}
		return nullptr;
	}

	const std::string& asset_folderpath_get()
	{
		return g_FolderPath;
	}

	const std::unordered_map<std::string, AssetFile>& asset_map_get()
	{
		return g_Files;
	}

	// ASSET

	AssetRef::~AssetRef()
	{
		unload();
	}

	AssetRef::AssetRef(const AssetRef& other)
	{
		pInternal = other.pInternal;
		if (pInternal)
			reinterpret_cast<Asset_internal*>(pInternal)->refCount.fetch_add(1);
	}

	AssetRef& AssetRef::operator=(const AssetRef& other)
	{
		unload();
		if (other.pInternal) {
			pInternal = other.pInternal;
			reinterpret_cast<Asset_internal*>(pInternal)->refCount.fetch_add(1);
		}
		return *this;
	}

	AssetRef::AssetRef(AssetRef&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}

	AssetRef& AssetRef::operator=(AssetRef&& other) noexcept
	{
		unload();
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result AssetRef::load(const char* filePath)
	{
		unload();

		auto it = g_Files.find(filePath);

		if (it == g_Files.end()) {
			SV_LOG_ERROR("Asset not found %s", filePath);
			return Result_NotFound;
		}

		if (it->second.pInternalAsset) {
			pInternal = it->second.pInternalAsset;
			Asset_internal* asset = reinterpret_cast<Asset_internal*>(pInternal);
			asset->refCount.fetch_add(1);
			asset->unusedTime = float_max;
			return Result_Success;
		}
		else {

			SV_ASSERT(it->second.assetType);
			AssetType_internal& type = *reinterpret_cast<AssetType_internal*>(it->second.assetType);
			
			Asset_internal* pObject = reinterpret_cast<Asset_internal*>(type.allocator.alloc());
			new(pObject) Asset_internal();

			std::string absFilePath = g_FolderPath + filePath;

			Result result = type.createFn(absFilePath.c_str(), (ui8*)(pObject) + sizeof(Asset_internal));
			if (result_fail(result)) {
				SV_LOG_ERROR("Can't create %s asset: '%s'", type.name.c_str(), filePath);
				type.allocator.free(pObject);
				return result;
			}

			pObject->assetType = &type;
			pObject->filePath = it->first.c_str();
			pObject->hashCode = hash_string(filePath);
			pObject->refCount.fetch_add(1);
			pObject->unusedTime = float_max;

			it->second.pInternalAsset = pObject;
			pInternal = pObject;
			type.activeAssets.push_back(pObject);

			SV_LOG_INFO("%s asset loaded: '%s'", type.name.c_str(), pObject->filePath);
			return Result_Success;

		}

		SV_LOG_ERROR("Unknown error loading the asset: '%s'", filePath);

		return Result_UnknownError;
	}

	Result AssetRef::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}

	void AssetRef::unload()
	{
		if (g_Types.empty()) return;
		if (pInternal) {
			Asset_internal* asset = reinterpret_cast<Asset_internal*>(pInternal);
			asset->refCount.fetch_sub(1);
			pInternal = nullptr;
		}
	}

	void* AssetRef::get() const
	{
		if (pInternal) return (ui8*)(pInternal) + sizeof(Asset_internal);
		return nullptr;
	}

	const char* AssetRef::getAssetTypeStr() const
	{
		if (pInternal) return reinterpret_cast<Asset_internal*>(pInternal)->assetType->name.c_str();
		return nullptr;
	}

	const char* AssetRef::getFilePath() const
	{
		if (pInternal) return reinterpret_cast<Asset_internal*>(pInternal)->filePath;
		return nullptr;
	}

	size_t AssetRef::getHashCode() const
	{
		if (pInternal) return reinterpret_cast<Asset_internal*>(pInternal)->hashCode;
		return 0u;
	}

}