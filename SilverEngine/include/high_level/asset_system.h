#pragma once

#include "rendering/material_system.h"

namespace sv {

	enum AssetType : ui32 {
		AssetType_Invalid,
		AssetType_Texture,
		AssetType_ShaderLibrary,
		AssetType_Material
	};

	struct AssetRef {
		std::atomic<int>	refCount = 0u;
		const char*			path = nullptr;
		size_t				hashCode = 0u;
	};

	struct AssetRegister {

		std::filesystem::file_time_type lastModification;
		AssetType						assetType;
		AssetRef*						pInternal;

	};

	Result assets_refresh();

	const std::unordered_map<std::string, AssetRegister>& assets_registers_get();

	// Texture Asset

	class TextureAsset {
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
		void	unload();

		GPUImage*	getImage() const noexcept;
		size_t		getHashCode() const noexcept;
		const char*	getPath() const noexcept;

		inline bool hasReference() const noexcept { return pInternal != nullptr; }

	private:
		void* pInternal = nullptr;

	};

	// ShaderLibrary Asset

	class ShaderLibraryAsset {
	public:
		ShaderLibraryAsset() = default;
		~ShaderLibraryAsset();
		ShaderLibraryAsset(const ShaderLibraryAsset& other);
		ShaderLibraryAsset(ShaderLibraryAsset&& other) noexcept;
		ShaderLibraryAsset& operator=(const ShaderLibraryAsset& other);
		ShaderLibraryAsset& operator=(ShaderLibraryAsset&& other) noexcept;
		inline bool operator==(const ShaderLibraryAsset& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const ShaderLibraryAsset& other) const noexcept { return pInternal != other.pInternal; }

		Result createSpriteShader(const char* filePath);

		Result	load(const char* filePath);
		Result	load(size_t hashCode);
		void	unload();

		ShaderLibrary& getShaderLibrary() const noexcept;
		inline ShaderLibrary* operator->() const noexcept { return &getShaderLibrary(); }
		const char* getPath() const noexcept;

		inline bool hasReference() const noexcept { return pInternal != nullptr; }

	private:
		void* pInternal = nullptr;

	};

	// Material Asset

	class MaterialAsset {
	public:
		MaterialAsset() = default;
		~MaterialAsset();
		MaterialAsset(const MaterialAsset& other);
		MaterialAsset(MaterialAsset&& other) noexcept;
		MaterialAsset& operator=(const MaterialAsset& other);
		MaterialAsset& operator=(MaterialAsset&& other) noexcept;
		inline bool operator==(const MaterialAsset& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const MaterialAsset& other) const noexcept { return pInternal != other.pInternal; }

		Result create(const char* filePath, ShaderLibrary& shaderLibrary);
		Result serialize(); // Save material in file

		Result	load(const char* filePath);
		Result	load(size_t hashCode);
		void	unload();

		Result setTexture(const char* name, TextureAsset& texture);
		Result getTexture(const char* name, TextureAsset& texture);

		Material* getMaterial() const noexcept;
		inline Material* operator->() const noexcept { return getMaterial(); }
		const char* getPath() const noexcept;

		inline bool hasReference() const noexcept { return pInternal != nullptr; }

	private:
		void* pInternal = nullptr;

	};

}