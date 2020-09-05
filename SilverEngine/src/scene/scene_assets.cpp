#include "core.h"

#include "scene.h"
#include "scene_internal.h"

namespace fs = std::filesystem;

namespace sv {

	static std::string g_FolderPath;

	static std::map<std::string, SharedRef<Texture>> g_TexturesMap;

	static ui32 g_RefreshCount = 0u;

	Result scene_assets_initialize(const char* assetsFolderPath)
	{
		// Create folder path
		g_FolderPath = assetsFolderPath;

#ifdef SV_SRC_PATH
		g_FolderPath = SV_SRC_PATH + g_FolderPath;
#endif

		// Create Assets
		svCheck(scene_assets_refresh());

		return Result_Success;
	}

	Result scene_assets_close()
	{
		return Result_Success;
	}

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

				for (auto it = path.begin(); it != path.end(); ++it) {
					if (*it == '\\') {
						*it = '/';
					}
				}

				if (strcmp(extension.c_str(), ".png") == 0 || strcmp(extension.c_str(), ".jpg") == 0) {
					g_TexturesMap[path.c_str()] = { SharedRef<Texture>() };
				}
			}

		}

		return Result_Success;
	}

	Result scene_assets_refresh()
	{
		// Check if assets folder exists
		if (!fs::exists(g_FolderPath.c_str())) {
			log_error("Asset folder not found '%s'", g_FolderPath.c_str());
			return Result_NotFound;
		}

		g_TexturesMap.clear();

		svCheck(scene_assets_refresh_folder(g_FolderPath.c_str()));
		g_RefreshCount++;

		return Result_Success;
	}

	Result scene_assets_refresh(Scene_internal& scene)
	{
		auto& assets = scene.assets;

		assets.refreshCount++;
		return Result_Success;
	}

	Result scene_assets_create(const SceneDesc* desc, Scene_internal& scene)
	{
		scene.assets.refreshCount = 0u;
		scene_assets_refresh(scene);
		return Result_Success;
	}

	Result scene_assets_destroy(Scene_internal& scene)
	{
		return Result_Success;
	}

	Result asset_manager_destroy(sv::Texture& texture)
	{
		return texture.Destroy();
	}

	template <typename T>
	void scene_assets_ckeck(std::vector<std::pair<std::string, SharedRef<T>>>& list)
	{
		for (auto it = list.begin(); it != list.end();) {

			SharedRef<T>& ref = it->second;
			if (ref.RefCount() <= 2u) {

				// Destroy Resource
				SV_ASSERT(asset_manager_destroy(*ref.Get()) == Result_Success);

				log_info("Asset freed successfully '%s'", it->first.c_str());

				// Free Reference
				g_TexturesMap[it->first].Delete();
				ref.Delete();

				ui32 itCount = ui32(it - list.cbegin());
				list.erase(it);
				it = list.begin() + itCount;
			}
			else ++it;
		}
	}

	Result scene_assets_update(Scene* scene_, float dt)
	{
		parseScene();

		static float lifeTimeCount = 0.f;

		// Refresh resources
		if (scene.assets.refreshCount < g_RefreshCount) {
			svCheck(scene_assets_refresh(scene));
		}

		// Destroy Unused Resources
		lifeTimeCount += dt;
		if (lifeTimeCount >= SV_SCENE_ASSET_LIFETIME) {

			scene_assets_ckeck(scene.assets.textures);

			lifeTimeCount -= SV_SCENE_ASSET_LIFETIME;
		}
	}

	Result scene_assets_load_texture(Scene* scene_, const char* filePath, SharedRef<Texture>& ref)
	{
		parseScene();

		auto& assets = scene.assets;

		auto it = g_TexturesMap.find(filePath);

		if (it != g_TexturesMap.end()) {

			if (it->second.Get()) {
				ref = it->second;
				return Result_Success;
			}

			// Create Texture
			std::string path = g_FolderPath + filePath;

			SharedRef<Texture> texture = sv::create_shared<sv::Texture>();
			if (texture->CreateFromFile(path.c_str(), true, sv::SamplerAddressMode_Wrap) == Result_Success) {

				it->second = texture;
				ref = texture;

				assets.textures.emplace_back(std::make_pair(it->first.c_str(), it->second));

				log_info("Texture Asset loaded successfully '%s'", filePath);
				return Result_Success;
			}
		}

		sv::log_error("Texture '%s' not found", filePath);
		return Result_NotFound;
	}

	const std::map<std::string, SharedRef<Texture>>& scene_assets_texturesmap_get()
	{
		return g_TexturesMap;
	}

}