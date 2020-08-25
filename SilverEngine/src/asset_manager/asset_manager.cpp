#include "core.h"

#include "asset_manager_internal.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace sv {

	template<typename T>
	struct Asset {
		std::string		path;
		SharedRef<T>	ref;
	};

	static std::string g_FolderPath;
	static std::map<AssetID, Asset<Mesh>> g_Meshes;

	static float g_UpdateCount = 0.f;

	bool asset_manager_initialize()
	{
		// Create folder path
		g_FolderPath = "assets/";

#ifdef SV_SRC_PATH
		g_FolderPath = SV_SRC_PATH + g_FolderPath;
#endif

		// Check if exists
		svCheck(fs::exists(g_FolderPath.c_str()));

		// Compute Hashes
		auto it = fs::directory_iterator(g_FolderPath.c_str());
		for (const auto& file : it) {
			const fs::path& path = file.path();
			std::string name = path.filename().extension().string();
			log(name.c_str());
		}

		return true;
	}

	template<typename T>
	void asset_manager_update_map(std::map<AssetID, Asset<T>> map)
	{
		for (auto it = map.begin(); it != map.end(); ++it) {

			Asset<T>& asset = it->second;

			if (asset.ref.RefCount() <= 1u) {
				map.erase(it);
				it = map.begin();
			}
		}
	}

	void asset_manager_update(float dt)
	{
		constexpr float UPDATE_RATE = 5.f;
		g_UpdateCount += dt;

		if (g_UpdateCount >= UPDATE_RATE) {

			asset_manager_update_map(g_Meshes);

			g_UpdateCount -= UPDATE_RATE;
		}
	}

	bool asset_manager_close()
	{
		return true;
	}

	AssetID asset_manager_compute_hash(const char* assetPath) {
		AssetID ID;
		utils_hash_string(ID, assetPath);
		return ID;
	}

	AssetID asset_manager_get_id(const char* assetPath)
	{
		return asset_manager_compute_hash(assetPath);
	}

	SharedRef<Mesh> asset_manager_load_mesh(AssetID assetID)
	{
		auto it = g_Meshes.find(assetID);
		if (it != g_Meshes.end()) {

			if (!it->second.ref->HasValidBuffers()) {
				// Create Mesh
			}
			return it->second.ref;
		}
		return SharedRef<Mesh>();
	}

}