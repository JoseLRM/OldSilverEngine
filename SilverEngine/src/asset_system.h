#pragma once

#include "renderer.h"

namespace sv {

	enum AssetType : ui32 {
		AssetType_Invalid,
		AssetType_Texture,
		AssetType_Material,
		AssetType_ShaderLibrary
	};

	struct AssetRef {
		ui32		refCount = 0u;
		const char* name = nullptr;
	};

	struct AssetRegister {

		std::filesystem::file_time_type lastModification;
		AssetType						assetType;
		AssetRef*						pInternal;

	};

	Result assets_refresh();

	const std::map<std::string, AssetRegister>& assets_registers_get();

	// Assets parent

	class Asset {
	
	protected:
		void* pInternal = nullptr;

	protected:
		void add_ref();
		void remove_ref();

		void copy(const Asset& other);
		void move(Asset& other) noexcept;

	public:
		void unload();

	};

	// Texture Asset

	class TextureAsset : public Asset {
	public:
		TextureAsset() = default;
		~TextureAsset();
		TextureAsset(const TextureAsset& other);
		TextureAsset(TextureAsset&& other) noexcept;
		TextureAsset& operator=(const TextureAsset& other);
		TextureAsset& operator=(TextureAsset&& other) noexcept;
		inline bool operator==(const TextureAsset& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const TextureAsset& other) const noexcept { return pInternal != other.pInternal; }

		Result	load(const char* filePath);
		Result	load(size_t hashCode);

		GPUImage*	get_image() const noexcept;
		size_t		get_hashcode() const noexcept;

	};

	// Shader Library Asset

	class ShaderLibraryAsset : public Asset {
	public:
		ShaderLibraryAsset() = default;
		~ShaderLibraryAsset();
		ShaderLibraryAsset(const ShaderLibraryAsset& other);
		ShaderLibraryAsset(ShaderLibraryAsset&& other) noexcept;
		ShaderLibraryAsset& operator=(const ShaderLibraryAsset& other);
		ShaderLibraryAsset& operator=(ShaderLibraryAsset&& other) noexcept;
		inline bool operator==(const ShaderLibraryAsset& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const ShaderLibraryAsset& other) const noexcept { return pInternal != other.pInternal; }

		Result create(const char* filePath); // Create a file with default shader code

		Result	load(const char* filePath);
		Result	load(size_t hashCode);

		ShaderLibrary* get_shader() const noexcept;
		size_t get_id() const noexcept;

	};

	// Material Asset

	class MaterialAsset : public Asset {
	public:
		MaterialAsset() = default;
		~MaterialAsset();
		MaterialAsset(const MaterialAsset& other);
		MaterialAsset(MaterialAsset&& other) noexcept;
		MaterialAsset& operator=(const MaterialAsset& other);
		MaterialAsset& operator=(MaterialAsset&& other) noexcept;
		inline bool operator==(const MaterialAsset& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const MaterialAsset& other) const noexcept { return pInternal != other.pInternal; }

		Result create(const char* filePath, ShaderLibraryAsset& shaderLibrary);
		
		Result serialize(); // Save material in file

		Result load(const char* filePath);
		Result load(size_t hashCode);

		ShaderAttributeType get_type(const char* name);
		Result				set(const char* name, const void* data, ShaderAttributeType type);
		Result				get(const char* name, void* data, ShaderAttributeType type);

		Material* get_material() const noexcept;
		const char* get_name() const noexcept;

	};

}