#include "core.h"

#include "material_system_internal.h"
#include "utils/io.h"

namespace sv {

	// Material

	Result loadFromFileMaterialAsset(const char* filePath, void* pObject)
	{
		new(pObject) MaterialAsset_internal();
		MaterialAsset_internal& asset = *reinterpret_cast<MaterialAsset_internal*>(pObject);

		ArchiveI file;
		file.open_file(filePath);

		file >> asset.hashCode;

		if (asset.hashCode == 0u) return Result_InvalidFormat;

		ShaderLibraryAsset lib;
		svCheck(lib.loadFromFile(asset.hashCode));

		svCheck(matsys_material_create(lib.get(), false, &asset.material));

		const std::vector<MaterialAttribute>& attributes = matsys_shaderlibrary_attributes(lib.get());

		XMMATRIX rawData;
		std::string name;
		ShaderAttributeType type;

		u32 attCount;
		file >> attCount;

		for (u32 i = 0; i < attCount; ++i) {

			file >> name >> type;

			Result res = Result_Success;

			if (type == u32_max) {

				TextureAsset tex;
				res = tex.load(file);

				if (result_okay(res)) {
					res = matsys_material_texture_set(asset.material, name.c_str(), tex);
				}
			}
			else {

				u32 typeSize = graphics_shader_attribute_size(type);
				file.read(&rawData, typeSize);

				res = matsys_material_attribute_set(asset.material, type, name.c_str(), &rawData);
			}

			if (result_fail(res)) {
				SV_LOG_ERROR("Unknown attribute loading a material asset: '%s'", name.c_str());
			}
		}

		return Result_Success;
	}

	Result destroyMaterialAsset(void* pObject)
	{
		MaterialAsset_internal& asset = *reinterpret_cast<MaterialAsset_internal*>(pObject);
		Result res = matsys_material_destroy(asset.material);
		return res;
	}

	Result reloadMaterialAsset(const char* filePath, void* pObject)
	{
		svCheck(destroyMaterialAsset(pObject));
		return loadFromFileMaterialAsset(filePath, pObject);
	}

	Result serializeMaterial(ArchiveO& file, void* pObject)
	{
		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(pObject);
		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(matsys_material_get_shaderlibrary(mat.material));

		file << mat.hashCode << u32(lib.matInfo.attributes.size() + lib.matInfo.textures.size());

		XMMATRIX rawData;

		for (const MaterialAttribute& att : lib.matInfo.attributes) {
			file << att.name << att.type;

			Result res = matsys_material_attribute_get(mat.material, att.type, att.name.c_str(), &rawData);
			SV_ASSERT(result_okay(res));
			file.write(&rawData, graphics_shader_attribute_size(att.type));
		}

		for (const std::string& texName : lib.matInfo.textures) {
			file << texName << u32_max;
			TextureAsset tex;
			Result res = matsys_material_texture_get(mat.material, texName.c_str(), tex);
			SV_ASSERT(result_okay(res));
			tex.save(file);
		}

		return Result_Success;
	}

	// Shader library

	Result loadFromFileShaderLibraryAsset(const char* filePath, void* pObject)
	{
		new(pObject) ShaderLibrary_internal();
		ShaderLibrary*& lib = reinterpret_cast<ShaderLibrary*&>(pObject);
		return matsys_shaderlibrary_create_from_file(filePath, &lib);
	}

	Result destroyShaderLibraryAsset(void* pObject)
	{
		ShaderLibrary* lib = reinterpret_cast<ShaderLibrary*>(pObject);
		return matsys_shaderlibrary_destroy(lib);
	}

	Result reloadShaderLibraryAsset(const char* filePath, void* pObject)
	{
		ShaderLibrary*& lib = reinterpret_cast<ShaderLibrary*&>(pObject);
		return matsys_shaderlibrary_create_from_file(filePath, &lib);
	}

	bool isUnusedShaderLibraryAsset(void* pObject)
	{
		ShaderLibrary_internal* lib = reinterpret_cast<ShaderLibrary_internal*>(pObject);
		return lib->matCount.load() == 0;
	}

	Result matsys_asset_register()
	{
		const char* extension;
		AssetRegisterTypeDesc desc;
		desc.pExtensions = &extension;
		desc.extensionsCount = 1u;

		extension = "shader";
		desc.name = "Shader Library";
		desc.loadFileFn = loadFromFileShaderLibraryAsset;
		desc.loadIDFn = nullptr;
		desc.createFn = nullptr;
		desc.destroyFn = destroyShaderLibraryAsset;
		desc.reloadFileFn = reloadShaderLibraryAsset;
		desc.serializeFn = nullptr;
		desc.isUnusedFn = isUnusedShaderLibraryAsset;
		desc.assetSize = sizeof(ShaderLibrary_internal*);
		desc.unusedLifeTime = 10.f;

		svCheck(asset_register_type(&desc, nullptr));
		
		extension = "mat";
		desc.name = "Material";
		desc.loadFileFn = loadFromFileMaterialAsset;
		desc.loadIDFn = nullptr;
		desc.createFn = nullptr;
		desc.destroyFn = destroyMaterialAsset;
		desc.reloadFileFn = reloadMaterialAsset;
		desc.serializeFn = serializeMaterial;
		desc.isUnusedFn = nullptr;
		desc.assetSize = sizeof(MaterialAsset_internal);
		desc.unusedLifeTime = 4.f;

		svCheck(asset_register_type(&desc, nullptr));

		return Result_Success;
	}

	Result MaterialAsset::createFile(const char* filePath, ShaderLibraryAsset& libAsset)
	{
		std::string absFilePath = asset_folderpath_get() + filePath;

		ArchiveO file;

		ShaderLibrary* shaderLibrary_ = libAsset.get();
		if (shaderLibrary_ == nullptr) return Result_InvalidUsage;

		ShaderLibrary_internal& lib = *reinterpret_cast<ShaderLibrary_internal*>(shaderLibrary_);
		file << libAsset.getHashCode() << u32(lib.matInfo.attributes.size());

		XMMATRIX rawData;
		SV_ZERO_MEMORY(&rawData, sizeof(XMMATRIX));

		for (const MaterialAttribute& att : lib.matInfo.attributes) {
			file << att.name << att.type;
			file.write(&rawData, graphics_shader_attribute_size(att.type));
		}

		for (const std::string& tex : lib.matInfo.textures) {
			file << tex << u32_max;
			file.write(&rawData, sizeof(size_t));
		}

		return file.save_file(absFilePath.c_str());
	}

}