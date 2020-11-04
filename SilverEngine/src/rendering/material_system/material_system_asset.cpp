#include "core.h"

#include "material_system_internal.h"
#include "utils/io.h"

namespace sv {

	// Material

	Result createMaterialAsset(const char* filePath, void* pObject)
	{
		new(pObject) MaterialAsset_internal();
		MaterialAsset_internal& asset = *reinterpret_cast<MaterialAsset_internal*>(pObject);

		ArchiveI file;
		file.open_file(filePath);

		file >> asset.hashCode;

		if (asset.hashCode == 0u) return Result_InvalidFormat;

		ShaderLibraryAsset lib;
		svCheck(lib.load(asset.hashCode));

		svCheck(asset.material.create(lib.get()));

		const auto& attributes = lib->getAttributes();

		XMMATRIX rawData;
		std::string name;
		ShaderAttributeType type;

		ui32 attCount;
		file >> attCount;

		for (ui32 i = 0; i < attCount; ++i) {

			file >> name >> type;

			Result res = Result_Success;

			if (type == ShaderAttributeType_Other) {

				size_t hashCode;
				file >> hashCode;

				if (hashCode) {

					TextureAsset tex;
					res = tex.load(hashCode);

					if (result_okay(res)) {
						res = asset.material.setTexture(name.c_str(), tex);
					}
				}
			}
			else {

				ui32 typeSize = graphics_shader_attribute_size(type);
				file.read(&rawData, typeSize);

				res = asset.material.setRaw(name.c_str(), type, &rawData);
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
		Result res = asset.material.destroy();
		return res;
	}

	Result recreateMaterialAsset(const char* filePath, void* pObject)
	{
		svCheck(destroyMaterialAsset(pObject));
		return createMaterialAsset(filePath, pObject);
	}

	// Shader library

	Result createShaderLibraryAsset(const char* filePath, void* pObject)
	{
		new(pObject) ShaderLibrary_internal();
		ShaderLibrary& lib = *reinterpret_cast<ShaderLibrary*>(pObject);
		return lib.createFromFile(filePath);
	}

	Result destroyShaderLibraryAsset(void* pObject)
	{
		ShaderLibrary& lib = *reinterpret_cast<ShaderLibrary*>(pObject);
		return lib.destroy();
	}

	Result recreateShaderLibraryAsset(const char* filePath, void* pObject)
	{
		ShaderLibrary& lib = *reinterpret_cast<ShaderLibrary*>(pObject);
		return lib.createFromFile(filePath);
	}

	bool isUsedShaderLibraryAsset(void* pObject)
	{
		ShaderLibrary_internal* lib = *reinterpret_cast<ShaderLibrary_internal**>(pObject);
		return lib->materials.empty();
	}

	Result matsys_asset_register()
	{
		const char* extension;
		AssetRegisterTypeDesc desc;
		desc.pExtensions = &extension;
		desc.extensionsCount = 1u;

		extension = "shader";
		desc.name = "Shader Library";
		desc.createFn = createShaderLibraryAsset;
		desc.destroyFn = destroyShaderLibraryAsset;
		desc.recreateFn = recreateShaderLibraryAsset;
		desc.isUnusedFn = isUsedShaderLibraryAsset;
		desc.assetSize = sizeof(ShaderLibrary);
		desc.unusedLifeTime = 10.f;

		svCheck(asset_register_type(&desc, nullptr));
		
		extension = "mat";
		desc.name = "Material";
		desc.createFn = createMaterialAsset;
		desc.destroyFn = destroyMaterialAsset;
		desc.recreateFn = recreateMaterialAsset;
		desc.isUnusedFn = nullptr;
		desc.assetSize = sizeof(MaterialAsset_internal);
		desc.unusedLifeTime = 4.f;

		svCheck(asset_register_type(&desc, nullptr));

		return Result_Success;
	}

	Result MaterialAsset::createFile(const char* filePath, ShaderLibraryAsset& libAsset)
	{
		std::string absFilePath = asset_folderpath_get() + filePath;

#ifdef SV_RES_PATH
		absFilePath = absFilePath.c_str() + strlen(SV_RES_PATH);
#endif

		ArchiveO file;

		ShaderLibrary* shaderLibrary_ = libAsset.get();
		if (shaderLibrary_ == nullptr) return Result_InvalidUsage;

		ShaderLibrary_internal& lib = **reinterpret_cast<ShaderLibrary_internal**>(shaderLibrary_);
		file << libAsset.getHashCode() << ui32(lib.attributes.size());

		XMMATRIX rawData;
		svZeroMemory(&rawData, sizeof(XMMATRIX));
		
		for (auto& att : lib.attributes) {
			file << att.name << att.type;

			ui32 typeSize = (att.type == ShaderAttributeType_Other) ? sizeof(size_t) : graphics_shader_attribute_size(att.type);
			file.write(&rawData, typeSize);
		}

		return file.save_file(absFilePath.c_str());
	}

	Result MaterialAsset::serialize()
	{
		const char* filePath = getFilePath();
		if (filePath == nullptr) return Result_InvalidUsage;

		MaterialAsset_internal& mat = *reinterpret_cast<MaterialAsset_internal*>(m_Ref.get());
		ShaderLibrary_internal& lib = **reinterpret_cast<ShaderLibrary_internal**>(mat.material.getShaderLibrary());

		ArchiveO file;
		file << mat.hashCode << ui32(lib.attributes.size());

		XMMATRIX rawData;

		for (auto& att : lib.attributes) {
			file << att.name << att.type;

			if (att.type == ShaderAttributeType_Other) {
				TextureAsset tex;
				Result res = mat.material.getTexture(att.name.c_str(), tex);
				SV_ASSERT(result_okay(res));
				file << tex.getHashCode();
			}
			else {
				ui32 typeSize = graphics_shader_attribute_size(att.type);
				Result res = mat.material.getRaw(att.name.c_str(), att.type, &rawData);
				SV_ASSERT(result_okay(res));
				file.write(&rawData, typeSize);
			}
		}

		// Save File
		std::string absFilePath = asset_folderpath_get() + filePath;
		return file.save_file(absFilePath.c_str());
	}

}