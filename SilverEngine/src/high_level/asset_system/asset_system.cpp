#include "core.h"

#include "asset_system_internal.h"
#include "utils/allocator.h"
#include "utils/io.h"
#include "simulation/entity_system.h"
#include "utils/io.h"

namespace fs = std::filesystem;

namespace sv {

	static std::string g_FolderPath;

	static std::unordered_map<std::string, AssetRegister> g_AssetMap;
	static std::unordered_map<size_t, const char*> g_HashCodes;

	static InstanceAllocator<TextureAsset_internal>			g_TextureAllocator;
	static InstanceAllocator<ShaderLibraryAsset_internal>	g_ShaderLibraryAllocator;
	static InstanceAllocator<MaterialAsset_internal>		g_MaterialAllocator;

	static std::vector<TextureAsset_internal*>			g_ActiveTextures;
	static std::vector<ShaderLibraryAsset_internal*>	g_ActiveShaderLibraries;
	static std::vector<MaterialAsset_internal*>			g_ActiveMaterials;

#define ASSERT_PTR() SV_ASSERT(pInternal != nullptr)
#define PARSE_REFERENCE() sv::AssetRef& ref = *reinterpret_cast<sv::AssetRef*>(pInternal);
#define PARSE_TEXTURE() sv::TextureAsset_internal& tex = *reinterpret_cast<sv::TextureAsset_internal*>(pInternal);
#define PARSE_SHADER_LIBRARY() sv::ShaderLibraryAsset_internal& lib = *reinterpret_cast<sv::ShaderLibraryAsset_internal*>(pInternal);
#define PARSE_MATERIAL() sv::MaterialAsset_internal& mat = *reinterpret_cast<sv::MaterialAsset_internal*>(pInternal);

	// INTERNAL FUNCTIONS

	Result assets_initialize(const char* assetsFolderPath)
	{
		// Create folder path
		g_FolderPath = assetsFolderPath;

#ifdef SV_RES_PATH
		g_FolderPath = SV_RES_PATH + g_FolderPath;
#endif

		// Create Assets
		svCheck(assets_refresh());

		return Result_Success;
	}

	Result assets_close()
	{
		Result res;

		// Free textures
		for (TextureAsset_internal* tex : g_ActiveTextures) {
			res = assets_destroy(tex);
			if (res != Result_Success) SV_LOG_ERROR("Can't destroy texture: Error code %u", res);
			g_TextureAllocator.destroy(tex);
		}

		// Free materials
		for (MaterialAsset_internal* mat : g_ActiveMaterials) {
			res = assets_destroy(mat);
			if (res != Result_Success) SV_LOG_ERROR("Can't destroy material: Error code %u", res);
			g_MaterialAllocator.destroy(mat);
		}

		// Free shader libraries
		for (ShaderLibraryAsset_internal* lib : g_ActiveShaderLibraries) {
			res = assets_destroy(lib);
			if (res != Result_Success) SV_LOG_ERROR("Can't destroy shader library: Error code %u", res);
			g_ShaderLibraryAllocator.destroy(lib);
		}

		g_AssetMap.clear();
		g_ActiveTextures.clear();
		g_HashCodes.clear();

		return Result_Success;
	}

	// ASSET CREATION

	Result assets_create(TextureAsset_internal* texture, const char* filePath) 
	{
		svCheck(assets_destroy(texture));

		// Get file data
		void* data;
		ui32 width;
		ui32 height;
		svCheck(load_image(filePath, &data, &width, &height));

		// Create Image
		GPUImageDesc desc;

		desc.pData = data;
		desc.size = width * height * 4u;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.layout = GPUImageLayout_ShaderResource;
		desc.type = GPUImageType_ShaderResource;
		desc.usage = ResourceUsage_Static;
		desc.CPUAccess = CPUAccess_None;
		desc.dimension = 2u;
		desc.width = width;
		desc.height = height;
		desc.depth = 1u;
		desc.layers = 1u;

		Result res = graphics_image_create(&desc, &texture->image);

		delete[] data;
		return res;
	}

	Result assets_create(ShaderLibraryAsset_internal* library, const char* filePath)
	{
		return library->shaderLibrary.createFromFile(filePath);
	}

	// ASSET DESTRUCTION

	Result assets_destroy(TextureAsset_internal* texture) {
		return graphics_destroy(texture->image);
	}

	Result assets_destroy(ShaderLibraryAsset_internal* library) {
		return library->shaderLibrary.destroy();
	}

	Result assets_destroy(MaterialAsset_internal* material) {
		return Result_TODO;
	}

	// UPDATE

	void assets_update(float dt)
	{
		Result auxRes;
		static float lifeTimeCount = 0.f;

		// Destroy Unused Resources
		lifeTimeCount += dt;
		if (lifeTimeCount >= ASSET_LIFETIME) {

			// Textures
			for (auto it = g_ActiveTextures.begin(); it != g_ActiveTextures.end();) {

				TextureAsset_internal* tex = *it;

				if (tex->refCount.load() <= 0u) {

					auxRes = assets_destroy(*it);
					SV_LOG_INFO("Texture freed: '%s'", tex->path);

					if (auxRes != Result_Success) {
						SV_LOG_ERROR("Can't destroy texture '%s'", tex->path);
					}
					g_AssetMap[tex->path].pInternal = nullptr;
					g_TextureAllocator.destroy(tex);

					it = g_ActiveTextures.erase(it);
				}
				else ++it;
			}

			lifeTimeCount -= ASSET_LIFETIME;
		}
	}

	// REFRESH

	Result assets_refresh_folder(const char* folderPath)
	{
		auto it = fs::directory_iterator(folderPath);
		for (const auto& file : it) {

			if (file.is_directory()) {
				std::string path = parse_string(file.path().c_str());
				svCheck(assets_refresh_folder(path.c_str()));
			}

			else {
				std::string path = parse_string(file.path().c_str() + g_FolderPath.size());
				std::string extension = parse_string(file.path().extension().c_str());

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
				else if (strcmp(extension.c_str(), ".mat") == 0) {
					type = AssetType_Material;
				}
				else if (strcmp(extension.c_str(), ".shader") == 0) {
					// TODO: update shaderlibrary
				}

				// Save or update asset register
				if (type != AssetType_Invalid) {

					auto it = g_AssetMap.find(path.c_str());
					if (it != g_AssetMap.end()) {

						auto lastModification = fs::last_write_time(file);
						
						if (it->second.lastModification != lastModification) {
							it->second.lastModification = lastModification;
							if (it->second.pInternal) {

								// Reload assets
								AssetRegister& asset = it->second;
								std::string assetPath = g_FolderPath + path;

								switch (asset.assetType)
								{
								case AssetType_Texture:
								{
									TextureAsset_internal& tex = *reinterpret_cast<TextureAsset_internal*>(asset.pInternal);
									svCheck(assets_create(&tex, assetPath.c_str()));

									SV_LOG_INFO("Texture updated: '%s'", path.c_str());
								}
								break;
								}
							}
						}

						g_AssetMap[path.c_str()] = it->second;

					}
					else {
						std::string name = path.c_str();
						g_AssetMap[name] = { fs::last_write_time(file), type, nullptr };

						size_t hash = hash_string(path.c_str());
						g_HashCodes[hash] = g_AssetMap.find(path.c_str())->first.c_str();
					}

				}
			}

		}

		return Result_Success;
	}

	Result assets_refresh()
	{
		// Check if assets folder exists
		if (!fs::exists(g_FolderPath.c_str())) {
			SV_LOG_ERROR("Asset folder not found '%s'", g_FolderPath.c_str());
			return Result_NotFound;
		}

		svCheck(assets_refresh_folder(g_FolderPath.c_str()));
	
		return Result_Success;
	}

	const std::unordered_map<std::string, AssetRegister>& assets_registers_get()
	{
		return g_AssetMap;
	}

	// GLOBAL ASSET LOADER

	template<typename T>
	Result assets_load(const char* filePath, const char* assetTypeName, void** pInternal, InstanceAllocator<T>& allocator, std::vector<T*>& activeList)
	{
		auto it = g_AssetMap.find(filePath);

		if (it == g_AssetMap.end()) {
			SV_LOG_ERROR("%s '%s' not found", assetTypeName, filePath);
			return Result_NotFound;
		}

		if (it->second.pInternal) {

			*pInternal = it->second.pInternal;
			it->second.pInternal->refCount.fetch_add(1);

		}
		else {

			const char* absoluteFilePath = filePath;

#ifdef SV_RES_PATH
			std::string filePathStr = absoluteFilePath;
			filePathStr = g_FolderPath + filePathStr;
			absoluteFilePath = filePathStr.c_str();
#endif

			// Create and allocate
			T* obj = allocator.create();
			Result result = assets_create(obj, absoluteFilePath);
			if (result_fail(result)) {
				allocator.destroy(obj);
				return result;
			}

			// Add ptr to active list and map
			activeList.emplace_back(obj);
			it->second.pInternal = obj;

			// Compute hash and save const char* name
			obj->hashCode = hash_string(filePath);
			obj->path = g_HashCodes[obj->hashCode];

			obj->refCount.fetch_add(1);

			// Save allocated ptr
			*pInternal = obj;

			SV_LOG_INFO("%s loaded: '%s'", assetTypeName, obj->path);

		}

		return Result_Success;
	}

	// ASSET REF

	inline void assets_unload(void*& pInternal)
	{
		if (pInternal) {
			PARSE_REFERENCE();
			ref.refCount.fetch_sub(1);
			pInternal = nullptr;
		}
	}

	// TEXTURE ASSET

	TextureAsset::~TextureAsset()
	{
		unload();
	}
	TextureAsset::TextureAsset(const TextureAsset& other)
	{
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_TEXTURE();
			tex.refCount.fetch_add(1);
		}
	}
	TextureAsset::TextureAsset(TextureAsset&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}
	TextureAsset& TextureAsset::operator=(const TextureAsset& other)
	{
		if (pInternal) {
			PARSE_TEXTURE();
			tex.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_TEXTURE();
			tex.refCount.fetch_add(1);
		}
		return *this;
	}
	TextureAsset& TextureAsset::operator=(TextureAsset&& other) noexcept
	{
		if (pInternal) {
			PARSE_TEXTURE();
			tex.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result TextureAsset::load(const char* filePath)
	{
		unload();
		return assets_load(filePath, "Texture", &pInternal, g_TextureAllocator, g_ActiveTextures);
	}
	Result TextureAsset::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}

	void TextureAsset::unload()
	{
		assets_unload(pInternal);
	}

	GPUImage* TextureAsset::getImage() const noexcept
	{
		if (pInternal) return reinterpret_cast<TextureAsset_internal*>(pInternal)->image;
		return nullptr;
	}
	size_t TextureAsset::getHashCode() const noexcept
	{
		if (pInternal) return reinterpret_cast<TextureAsset_internal*>(pInternal)->hashCode;
		return 0u;
	}
	const char* TextureAsset::getPath() const noexcept
	{
		ASSERT_PTR();
		PARSE_TEXTURE();
		return tex.path;
	}

	// SHADER LIBRARY ASSET

	ShaderLibraryAsset::~ShaderLibraryAsset()
	{
		unload();
	}
	ShaderLibraryAsset::ShaderLibraryAsset(const ShaderLibraryAsset& other)
	{
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_SHADER_LIBRARY();
			lib.refCount.fetch_add(1);
		}
	}
	ShaderLibraryAsset::ShaderLibraryAsset(ShaderLibraryAsset&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}
	ShaderLibraryAsset& ShaderLibraryAsset::operator=(const ShaderLibraryAsset& other)
	{
		if (pInternal) {
			PARSE_SHADER_LIBRARY();
			lib.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_SHADER_LIBRARY();
			lib.refCount.fetch_add(1);
		}
		return *this;
	}
	ShaderLibraryAsset& ShaderLibraryAsset::operator=(ShaderLibraryAsset&& other) noexcept
	{
		if (pInternal) {
			PARSE_SHADER_LIBRARY();
			lib.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result ShaderLibraryAsset::createSpriteShader(const char* filePath)
	{
		return Result_TODO;
	}

	Result ShaderLibraryAsset::load(const char* filePath)
	{
		unload();
		return assets_load(filePath, "Shader Library", &pInternal, g_ShaderLibraryAllocator, g_ActiveShaderLibraries);
	}
	Result ShaderLibraryAsset::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}
	void ShaderLibraryAsset::unload()
	{
		assets_unload(pInternal);
	}

	ShaderLibrary& ShaderLibraryAsset::getShaderLibrary() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.shaderLibrary;
	}
	const char* ShaderLibraryAsset::getPath() const noexcept
	{
		ASSERT_PTR();
		PARSE_SHADER_LIBRARY();
		return lib.path;
	}

	// MATERIAL ASSET

	MaterialAsset::~MaterialAsset()
	{
		unload();
	}
	MaterialAsset::MaterialAsset(const MaterialAsset& other)
	{
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_MATERIAL();
			mat.refCount.fetch_add(1);
		}
	}
	MaterialAsset::MaterialAsset(MaterialAsset&& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}
	MaterialAsset& MaterialAsset::operator=(const MaterialAsset& other)
	{
		if (pInternal) {
			PARSE_MATERIAL();
			mat.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		if (pInternal) {
			PARSE_MATERIAL();
			mat.refCount.fetch_add(1);
		}
		return *this;
	}
	MaterialAsset& MaterialAsset::operator=(MaterialAsset&& other) noexcept
	{
		if (pInternal) {
			PARSE_MATERIAL();
			mat.refCount.fetch_sub(1);
		}
		pInternal = other.pInternal;
		other.pInternal = nullptr;
		return *this;
	}

	Result MaterialAsset::create(const char* filePath, ShaderLibrary& shaderLibrary)
	{
		return Result_TODO;
	}
	Result MaterialAsset::serialize()
	{
		return Result_TODO;
	}

	Result MaterialAsset::load(const char* filePath)
	{
		return Result_TODO;
	}
	Result MaterialAsset::load(size_t hashCode)
	{
		return Result_TODO;
	}
	void MaterialAsset::unload()
	{
		assets_unload(pInternal);
	}

	Result MaterialAsset::setTexture(const char* name, TextureAsset& texture)
	{
		ASSERT_PTR();
		return Result_TODO;
	}
	Result MaterialAsset::getTexture(const char* name, TextureAsset& texture)
	{
		ASSERT_PTR();
		return Result_TODO;
	}

	Material* MaterialAsset::getMaterial() const noexcept
	{
		if (pInternal) return &reinterpret_cast<MaterialAsset_internal*>(pInternal)->material;
		return nullptr;
	}
	const char* MaterialAsset::getPath() const noexcept
	{
		ASSERT_PTR();
		PARSE_MATERIAL();
		return mat.path;
	}

}