#include "core.h"

#include "asset_system_internal.h"
#include "utils/allocators/InstanceAllocator.h"

namespace fs = std::filesystem;

namespace sv {

	static std::string g_FolderPath;

	static std::map<std::string, AssetRegister> g_AssetMap;
	static std::map<size_t, const char*> g_HashCodes;

	static InstanceAllocator<TextureAsset_internal>			g_TextureAllocator;
	static InstanceAllocator<ShaderLibraryAsset_internal>	g_ShaderLibraryAllocator;
	static InstanceAllocator<MaterialAsset_internal>		g_MaterialAllocator;

	static std::vector<TextureAsset_internal*>			g_ActiveTextures;
	static std::vector<MaterialAsset_internal*>			g_ActiveMaterials;
	static std::vector<ShaderLibraryAsset_internal*>	g_ActiveShaderLibraries;

	static std::unordered_map<size_t, ShaderLibraryAsset_internal*> g_ShaderLibraryMapID;

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
		Result res;

		// Free textures
		for (TextureAsset_internal* tex : g_ActiveTextures) {
			res = assets_destroy(tex);
			if (res != Result_Success) svLogError("Can't destroy texture: Error code %u", res);
			g_TextureAllocator.destroy(tex);
		}

		// Free materials
		for (MaterialAsset_internal* mat : g_ActiveMaterials) {
			res = assets_destroy(mat);
			if (res != Result_Success) svLogError("Can't destroy material: Error code %u", res);
			g_MaterialAllocator.destroy(mat);
		}

		// Free shaders
		for (ShaderLibraryAsset_internal* lib : g_ActiveShaderLibraries) {
			res = assets_destroy(lib);
			if (res != Result_Success) svLogError("Can't destroy shader library: Error code %u", res);
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
		svCheck(utils_loader_image(filePath, &data, &width, &height));

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

	Result assets_create(MaterialAsset_internal* material, const char* filePath)
	{
		ArchiveI file;
		svCheck(file.open_file(filePath));

		file >> material->shaderID;

		auto it = g_ShaderLibraryMapID.find(material->shaderID);
		if (it == g_ShaderLibraryMapID.end()) {
			svLogError("Material '%s' has an invalid shader ID", material->name);
			return Result_InvalidFormat;
		}

		svCheck(matsys_material_create(it->second->library, &material->material));
		return Result_Success;
	}

	Result assets_create(ShaderLibraryAsset_internal* library, const char* filePath)
	{
		// TODO: Load from bin

		std::ifstream file(filePath, std::ios::ate);
		if (!file.is_open()) return Result_NotFound;

		size_t fileSize = file.tellg();
		file.seekg(0u);

		bool hasVS = false;
		bool hasPS = false;
		bool hasGS = false;

		std::string name;
		std::string package;

		std::string line;
		std::getline(file, line);

		while (true) {

			if (line.size() <= 2u) {
				std::getline(file, line);
				continue;
			}

			if (line[0] == '#') {

				// shader
				if (line.size() >= 10 && line[1] == 's' && line[2] == 'h' && line[3] == 'a' && line[4] == 'd' && line[5] == 'e' && line[6] == 'r' && line[7] == ' ') {

					std::string_view value = line.c_str() + 8;
					if (value[1] != 's') svLogError("Shader '%s' has unknown shader type: '%s'", filePath, line.c_str());

					if (value[0] == 'v') {
						hasVS = true;
					}
					else if (value[0] == 'p') {
						hasPS = true;
					}
					else if (value[0] == 'g') {
						hasGS = true;
					}

				}

				// name
				if (line.size() >= 7 && line[1] == 'n' && line[2] == 'a' && line[3] == 'm' && line[4] == 'e' && line[5] == ' ') {
					name = line.c_str() + 6;
				}

				// package
				if (line.size() >= 10 && line[1] == 'p' && line[2] == 'a' && line[3] == 'c' && line[4] == 'k' && line[5] == 'a' && line[6] == 'g' && line[7] == 'e' && line[8] == ' ') {
					package = line.c_str() + 9;
				}

				// start
				if (line.size() >= 6 && line[1] == 's' && line[2] == 't' && line[3] == 'a' && line[4] == 'r' && line[5] == 't') {
					break;
				}
			}

			std::getline(file, line);
		}

		if (name.empty()) {
			svLogError("The shader must have a name '%s'", library->name);
			return Result_InvalidFormat;
		}

		if (package.empty()) package = "default";

		// TODO: Check if the name or package modified
		library->shaderName = package + '/' + name;

		fileSize -= file.tellg();
		std::string src;
		src.resize(fileSize);
		file.read(src.data(), fileSize);
		file.close();
		fileSize = strlen(src.c_str());

		// Compile shaders
		std::vector<ui8> vsBin, psBin, gsBin;
		{
			ShaderCompileDesc compDesc;

			compDesc.api = graphics_api_get();
			compDesc.entryPoint = "main";
			compDesc.majorVersion = 6u;
			compDesc.minorVersion = 0u;

			if (hasVS) {
				compDesc.shaderType = ShaderType_Vertex;
				svCheck(graphics_shader_compile_string(&compDesc, src.c_str(), fileSize, vsBin));
			}
			if (hasPS) {
				compDesc.shaderType = ShaderType_Pixel;
				svCheck(graphics_shader_compile_string(&compDesc, src.c_str(), fileSize, psBin));
			}
			if (hasGS) {
				compDesc.shaderType = ShaderType_Geometry;
				svCheck(graphics_shader_compile_string(&compDesc, src.c_str(), fileSize, gsBin));
			}
		}

		// Create shaders
		Shader* vs = nullptr;
		Shader* ps = nullptr;
		Shader* gs = nullptr;

		{
			ShaderDesc desc;

			if (hasVS) {
				desc.pBinData = vsBin.data();
				desc.binDataSize = vsBin.size();
				desc.shaderType = ShaderType_Vertex;

				svCheck(graphics_shader_create(&desc, &vs));
			}
			if (hasPS) {
				desc.pBinData = psBin.data();
				desc.binDataSize = psBin.size();
				desc.shaderType = ShaderType_Pixel;

				svCheck(graphics_shader_create(&desc, &ps));
			}
			if (hasGS) {
				desc.pBinData = gsBin.data();
				desc.binDataSize = gsBin.size();
				desc.shaderType = ShaderType_Geometry;

				svCheck(graphics_shader_create(&desc, &gs));
			}
		}

		// Create Shader Library
		ShaderLibraryDesc desc;
		desc.vertexShader = vs;
		desc.pixelShader = ps;
		desc.geometryShader = gs;

		if (library->library == nullptr) svCheck(matsys_shaderlibrary_create(&desc, &library->library));
		else {
			svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Vertex)));
			svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Pixel)));
			svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Geometry)));
			svCheck(matsys_shaderlibrary_recreate(&desc, library->library));
		}

		// Compute shader ID
		library->id = utils_hash_string(library->shaderName.c_str());

		return Result_Success;
	}

	// ASSET DESTRUCTION

	Result assets_destroy(TextureAsset_internal* texture) {
		return graphics_destroy(texture->image);
	}

	Result assets_destroy(MaterialAsset_internal* material) {
		return matsys_material_destroy(material->material);
	}

	Result assets_destroy(ShaderLibraryAsset_internal* library) {
		svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Vertex)));
		svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Pixel)));
		svCheck(graphics_destroy(matsys_shaderlibrary_shader_get(library->library, ShaderType_Geometry)));
		return matsys_shaderlibrary_destroy(library->library);
	}

	// UPDATE

	void assets_update(float dt)
	{
		Result auxRes;
		static float lifeTimeCount = 0.f;

		// Destroy Unused Resources
		lifeTimeCount += dt;
		if (lifeTimeCount >= SV_SCENE_ASSET_LIFETIME) {

			// Textures
			for (auto it = g_ActiveTextures.begin(); it != g_ActiveTextures.end();) {

				TextureAsset_internal* tex = *it;

				if (tex->refCount == 0u) {

					auxRes = assets_destroy(*it);

					if (auxRes != Result_Success) {
						svLogError("Can't destroy texture '%s'", tex->name);
					}
					g_AssetMap[tex->name].pInternal = nullptr;
					g_TextureAllocator.destroy(tex);

					svLog("Texture freed: '%s'", tex->name);

					it = g_ActiveTextures.erase(it);
				}
				else ++it;
			}

			lifeTimeCount -= SV_SCENE_ASSET_LIFETIME;
		}
	}

	// REFRESH

	Result assets_refresh_folder(const char* folderPath)
	{
		auto it = fs::directory_iterator(folderPath);
		for (const auto& file : it) {

			if (file.is_directory()) {
				std::string path = utils_string_parse(file.path().c_str());
				svCheck(assets_refresh_folder(path.c_str()));
			}

			else {
				std::string path = utils_string_parse(file.path().c_str() + g_FolderPath.size());
				std::string extension = utils_string_parse(file.path().extension().c_str());

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
					type = AssetType_ShaderLibrary;
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

									svLog("Texture updated: '%s'", path.c_str());
								}
									break;

								case AssetType_ShaderLibrary:
								{
									ShaderLibraryAsset_internal& lib = *reinterpret_cast<ShaderLibraryAsset_internal*>(asset.pInternal);
									if (assets_create(&lib, assetPath.c_str()) != Result_Success) svLogError("Shader '%s' can't compile", path.c_str());
									else svLog("Shader updated: '%s'", path.c_str());
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

						size_t hash = utils_hash_string(path.c_str());
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
			svThrow("Asset folder not found '%s'", g_FolderPath.c_str());
			return Result_NotFound;
		}

		svCheck(assets_refresh_folder(g_FolderPath.c_str()));
	
		return Result_Success;
	}

	const std::map<std::string, AssetRegister>& assets_registers_get()
	{
		return g_AssetMap;
	}

	// GLOBAL ASSET LOADER

	template<typename T>
	Result assets_load(const char* filePath, const char* assetTypeName, void** pInternal, InstanceAllocator<T>& allocator, std::vector<T*>& activeList)
	{
		auto it = g_AssetMap.find(filePath);

		if (it == g_AssetMap.end()) {
			svLogError("%s '%s' not found", assetTypeName, filePath);
			return Result_NotFound;
		}

		if (it->second.pInternal) {

			*pInternal = it->second.pInternal;

		}
		else {

			const char* absoluteFilePath = filePath;

#ifdef SV_SRC_PATH
			std::string filePathStr = absoluteFilePath;
			filePathStr = g_FolderPath + filePathStr;
			absoluteFilePath = filePathStr.c_str();
#endif

			// Create and allocate
			T aux;
			svCheck(assets_create(&aux, absoluteFilePath));
			T* obj = allocator.create();

			// Add ptr to active list and map
			activeList.emplace_back(obj);
			it->second.pInternal = obj;

			// Move data into allocated data
			*obj = std::move(aux);

			// Compute hash and save const char* name
			obj->hashCode = utils_hash_string(filePath);
			obj->name = g_HashCodes[obj->hashCode];

			// Save allocated ptr
			*pInternal = obj;

			svLog("%s loaded: '%s'", assetTypeName, obj->name);

		}

		return Result_Success;
	}

	// ASSET REF

	void Asset::add_ref()
	{
		AssetRef* ref = reinterpret_cast<AssetRef*>(pInternal);
		++ref->refCount;
	}

	void Asset::remove_ref()
	{
		AssetRef* ref = reinterpret_cast<AssetRef*>(pInternal);
		--ref->refCount;
	}

	void Asset::copy(const Asset& other)
	{
		if (other.pInternal) {
			pInternal = other.pInternal;
			add_ref();
		}
	}

	void Asset::move(Asset& other) noexcept
	{
		pInternal = other.pInternal;
		other.pInternal = nullptr;
	}

	void Asset::unload()
	{
		if (pInternal) {
			remove_ref();
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
		copy(other);
	}
	TextureAsset::TextureAsset(TextureAsset&& other) noexcept
	{
		move(other);
	}
	TextureAsset& TextureAsset::operator=(const TextureAsset& other)
	{
		copy(other);
		return *this;
	}
	TextureAsset& TextureAsset::operator=(TextureAsset&& other) noexcept
	{
		move(other);
		return *this;
	}

	Result TextureAsset::load(const char* filePath)
	{
		unload();
		svCheck(assets_load(filePath, "Texture", &pInternal, g_TextureAllocator, g_ActiveTextures));
		add_ref();
		return Result_Success;
	}

	Result TextureAsset::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}

	GPUImage* TextureAsset::get_image() const noexcept
	{
		return pInternal ? reinterpret_cast<TextureAsset_internal*>(pInternal)->image : nullptr;
	}

	size_t TextureAsset::get_hashcode() const noexcept
	{
		return pInternal ? reinterpret_cast<TextureAsset_internal*>(pInternal)->hashCode : 0u;
	}

	// SHADER LIBRARY ASSET

	ShaderLibraryAsset::~ShaderLibraryAsset()
	{
		unload();
	}
	ShaderLibraryAsset::ShaderLibraryAsset(const ShaderLibraryAsset& other)
	{
		copy(other);
	}
	ShaderLibraryAsset::ShaderLibraryAsset(ShaderLibraryAsset&& other) noexcept
	{
		move(other);
	}
	ShaderLibraryAsset& ShaderLibraryAsset::operator=(const ShaderLibraryAsset& other)
	{
		copy(other);
		return *this;
	}
	ShaderLibraryAsset& ShaderLibraryAsset::operator=(ShaderLibraryAsset&& other) noexcept
	{
		move(other);
		return *this;
	}

	Result ShaderLibraryAsset::load(const char* filePath)
	{
		unload();
		svCheck(assets_load(filePath, "Shader", &pInternal, g_ShaderLibraryAllocator, g_ActiveShaderLibraries));
		g_ShaderLibraryMapID[get_id()] = reinterpret_cast<ShaderLibraryAsset_internal*>(pInternal);
		add_ref();
		return Result_Success;
	}

	Result ShaderLibraryAsset::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}

	ShaderLibrary* ShaderLibraryAsset::get_shader() const noexcept
	{
		return pInternal ? reinterpret_cast<ShaderLibraryAsset_internal*>(pInternal)->library : nullptr;
	}

	size_t ShaderLibraryAsset::get_id() const noexcept
	{
		return pInternal ? reinterpret_cast<ShaderLibraryAsset_internal*>(pInternal)->id : 0u;
	}

	// MATERIAL ASSET

	MaterialAsset::~MaterialAsset()
	{
		unload();
	}
	MaterialAsset::MaterialAsset(const MaterialAsset& other)
	{
		copy(other);
	}
	MaterialAsset::MaterialAsset(MaterialAsset&& other) noexcept
	{
		move(other);
	}
	MaterialAsset& MaterialAsset::operator=(const MaterialAsset& other)
	{
		copy(other);
		return *this;
	}
	MaterialAsset& MaterialAsset::operator=(MaterialAsset&& other) noexcept
	{
		move(other);
		return *this;
	}

	Result MaterialAsset::create(const char* filePath, ShaderLibraryAsset& shaderLibrary)
	{
		std::string absoluteFilePath = g_FolderPath + filePath;

		// Exist asset
		if (g_AssetMap.find(filePath) != g_AssetMap.end()) {
			svLogError("Material file '%s' is currently created", filePath);
			return Result_InvalidUsage;
		}

		// Check if file root exists
		std::ofstream file(absoluteFilePath, std::ios::binary);
		if (!file.is_open()) {
			svLogError("Can't create a material file in '%s'", filePath);
			return Result_NotFound;
		}
		file.close();

		// Create material
		Material* material;
		ShaderLibrary* lib = shaderLibrary.get_shader();
		if (lib == nullptr) return Result_InvalidUsage;
		svCheck(matsys_material_create(lib, &material));

		// Allocate material asset and set some data
		MaterialAsset_internal* matAsset = g_MaterialAllocator.create();
		pInternal = matAsset;
		matAsset->material = material;
		matAsset->refCount = 0u;
		matAsset->hashCode = utils_hash_string(filePath);
		matAsset->shaderID = shaderLibrary.get_id();

		// Set material register
		g_AssetMap[filePath] = { fs::file_time_type::clock::now(), AssetType_Material, matAsset };
		const char* nameStrPtr = g_AssetMap.find(filePath)->first.c_str();
		g_HashCodes[matAsset->hashCode] = nameStrPtr;

		// Set name ptr
		matAsset->name = nameStrPtr;

		// Set material to active vector
		g_ActiveMaterials.push_back(matAsset);

		// Serialize
		svCheck(serialize());

		return Result_Success;
	}

	Result MaterialAsset::serialize()
	{
		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(pInternal);
		ArchiveO file;

		file << mat.shaderID;

		std::string absoluteFilePath = g_FolderPath + mat.name;
		const char* filePath = absoluteFilePath.c_str();
#ifdef SV_SRC_PATH
		filePath = absoluteFilePath.c_str() + strlen(SV_SRC_PATH);
#endif

		return file.save_file(filePath);
	}

	Result MaterialAsset::load(const char* filePath)
	{
		unload();
		svCheck(assets_load(filePath, "Material", &pInternal, g_MaterialAllocator, g_ActiveMaterials));
		add_ref();
		return Result_Success;
	}

	Result MaterialAsset::load(size_t hashCode)
	{
		auto it = g_HashCodes.find(hashCode);
		if (it == g_HashCodes.end()) return Result_NotFound;
		return load(it->second);
	}

	ShaderAttributeType MaterialAsset::get_type(const char* name)
	{
		SV_ASSERT(pInternal);
		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(pInternal);
		return matsys_shaderlibrary_attribute_type(matsys_material_shader_get(mat.material), name);
	}
	Result MaterialAsset::set(const char* name, const void* data, ShaderAttributeType type)
	{
		SV_ASSERT(pInternal);
		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(pInternal);
		return matsys_material_set(mat.material, name, data, type);
	}
	Result MaterialAsset::get(const char* name, void* data, ShaderAttributeType type)
	{
		SV_ASSERT(pInternal);
		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(pInternal);
		return matsys_material_get(mat.material, name, data, type);
	}

	Material* MaterialAsset::get_material() const noexcept
	{
		return pInternal ? reinterpret_cast<MaterialAsset_internal*>(pInternal)->material : nullptr;
	}

	const char* MaterialAsset::get_name() const noexcept
	{
		return pInternal ? reinterpret_cast<MaterialAsset_internal*>(pInternal)->name : nullptr;
	}

}