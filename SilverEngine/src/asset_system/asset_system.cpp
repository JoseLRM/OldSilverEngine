#include "core.h"

#include "asset_system_internal.h"

namespace fs = std::filesystem;

namespace sv {

	static std::string g_FolderPath;

	static std::map<std::string, Asset> g_AssetMap;
	static std::map<size_t, std::string> g_HashCodes;

	static std::vector<std::pair<std::string, SharedRef<TextureAsset>*>> g_ActiveTextures;

	// INTERNAL FUNCTIONS

	Result assets_initialize(const char* assetsFolderPath)
	{
		// Create folder path
		g_FolderPath = assetsFolderPath;

#ifdef SV_SRC_PATH
		g_FolderPath = SV_SRC_PATH + g_FolderPath;
#endif

		// Create Assets
		svCheck(assets_refresh());

		return Result_Success;
	}

	Result assets_close()
	{
		// TODO: Clear assets
		g_AssetMap.clear();
		g_ActiveTextures.clear();
		g_HashCodes.clear();

		return Result_Success;
	}

	Result assets_destroy(Texture& texture) {
		return texture.Destroy();
	}

	template <typename T>
	Result assets_ckeck(std::vector<std::pair<std::string, SharedRef<T>*>>& list)
	{
		for (auto it = list.begin(); it != list.end();) {

			SharedRef<T>& ref = *it->second;
			if (ref.RefCount() <= 1u) {

				// Destroy Resource
				svCheck(assets_destroy(ref.Get()->texture));

				log_info("Asset freed successfully '%s'", it->first.c_str());

				// Free Reference
				g_AssetMap[it->first].ref.Delete();
				ref.Delete();

				ui32 itCount = ui32(it - list.cbegin());
				list.erase(it);
				it = list.begin() + itCount;
			}
			else ++it;
		}

		return Result_Success;
	}

	Result assets_update(float dt)
	{
		static float lifeTimeCount = 0.f;

		// Destroy Unused Resources
		lifeTimeCount += dt;
		if (lifeTimeCount >= SV_SCENE_ASSET_LIFETIME) {

			svCheck(assets_ckeck(g_ActiveTextures));

			lifeTimeCount -= SV_SCENE_ASSET_LIFETIME;
		}

		return Result_Success;
	}

	// REFRESH

	Result scene_assets_refresh_folder(const char* folderPath)
	{
		auto it = fs::directory_iterator(folderPath);
		for (const auto& file : it) {

			if (file.is_directory()) {
				std::string path = sv::utils_string_parse(file.path().c_str());
				svCheck(scene_assets_refresh_folder(path.c_str()));
			}

			else {
				std::string path = sv::utils_string_parse(file.path().c_str() + g_FolderPath.size());
				std::string extension = sv::utils_string_parse(file.path().extension().c_str());

				// Prepare path
				for (auto it = path.begin(); it != path.end(); ++it) {
					if (*it == '\\') {
						*it = '/';
					}
				}

				// Choose asset type
				AssetType type = AssetType_Invalid;

				if (strcmp(extension.c_str(), ".png") == 0 || strcmp(extension.c_str(), ".jpg") == 0) {
					type = AssetType_Texture;
				}

				// Save hash string
				if (type != AssetType_Invalid) {

					g_AssetMap[path.c_str()] = { type, SharedRef<ui32>() };
					size_t hash = utils_hash_string(path.c_str());
					g_HashCodes[hash] = path.c_str();

				}
			}

		}

		return Result_Success;
	}

	Result assets_refresh()
	{
		// Check if assets folder exists
		if (!fs::exists(g_FolderPath.c_str())) {
			log_error("Asset folder not found '%s'", g_FolderPath.c_str());
			return Result_NotFound;
		}

		g_AssetMap.clear();

		svCheck(scene_assets_refresh_folder(g_FolderPath.c_str()));

		return Result_Success;
	}

	// LOADER FUNCTIONS

	Result assets_load_texture(const char* filePath, SharedRef<TextureAsset>& ref)
	{
		auto it = g_AssetMap.find(filePath);

		if (it != g_AssetMap.end()) {

			if (it->second.ref.Get()) {
				ref = *reinterpret_cast<SharedRef<TextureAsset>*>(&it->second.ref);
				return Result_Success;
			}

			// Create Texture
			std::string path = g_FolderPath + filePath;

			SharedRef<TextureAsset> asset = sv::create_shared<sv::TextureAsset>();
			if (asset->texture.CreateFromFile(path.c_str(), true, sv::SamplerAddressMode_Wrap) == Result_Success) {

				asset->hashCode = utils_hash_string(filePath);
				it->second = Asset{ AssetType_Texture, *reinterpret_cast<SharedRef<ui32>*>(&asset) };
				ref = asset;

				g_ActiveTextures.emplace_back(std::make_pair(it->first.c_str(), reinterpret_cast<SharedRef<TextureAsset>*>(&it->second.ref)));

				log_info("Texture Asset loaded successfully '%s'", filePath);
				return Result_Success;
			}
		}

		sv::log_error("Texture '%s' not found", filePath);
		return Result_NotFound;
	}

	Result assets_load_texture(size_t hashCode, SharedRef<TextureAsset>& ref)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;

		else return assets_load_texture(it->second.c_str(), ref);
	}

	// GETTERS

	const std::map<std::string, Asset>& assets_map_get()
	{
		return g_AssetMap;
	}

}