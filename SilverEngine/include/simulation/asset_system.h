#pragma once

#include "core.h"
#include "utils/io.h"

namespace sv {

	typedef void* AssetType;

	typedef Result(*AssetLoadFromFileFn)(const char* filePath, void* pObject);
	typedef Result(*AssetLoadFromIDFn)(size_t ID, void* pObject);
	typedef Result(*AssetCreateFn)(void* pObject);
	typedef Result(*AssetDestroyFn)(void* pObject);
	typedef Result(*AssetSerializeFn)(ArchiveO& archive, void* pObject);
	typedef bool(*AssetIsUnusedFn)(void* pObject);

	class AssetRef {

		// This is a copy of a internal struct to inline some getters
		struct Internal {
			std::atomic<int>	refCount = 0;
			float				unusedTime = float_max;
			const char*			filePath = nullptr;
			size_t				hashCode = 0u;
			void*				assetType = nullptr;
		};

	public:
		AssetRef() = default;
		~AssetRef();
		AssetRef(const AssetRef& other);
		AssetRef& operator=(const AssetRef& other);
		AssetRef(AssetRef&& other) noexcept;
		AssetRef& operator=(AssetRef&& other) noexcept;
		inline bool operator==(const AssetRef& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const AssetRef& other) const noexcept { return pInternal != other.pInternal; }

		/*
			Load a asset attached to a external file
		*/
		Result loadFromFile(const char* filePath);
		Result loadFromFile(size_t hashCode);

		/*
			Load a asset assigned with an ID
		*/
		Result loadFromID(AssetType assetType, const char* str);
		Result loadFromID(AssetType assetType, size_t ID);

		/*
			Create a asset attached to anything
		*/
		Result create(AssetType assetType);

		/*
			Try to load from a saved archive
		*/
		Result load(ArchiveI& archive);

		/*
			Save the asset to be loaded in other execution
		*/
		void save(ArchiveO& archive) const;

		/*
			Dereference to the global asset, if it is unused it will be removed
		*/
		void unload();

		Result serialize(const char* filePath = nullptr);

		inline void* get() const { return pInternal ? (reinterpret_cast<ui8*>(pInternal) + sizeof(Internal)) : nullptr; }
		const char* getAssetTypeStr() const;
		
		inline bool isAttachedToFile() const noexcept 
		{ 
			const char* filePath = reinterpret_cast<Internal*>(pInternal)->filePath;
			return filePath && filePath != (const char*)SIZE_MAX;
		}
		inline bool isAttachedToID() const noexcept 
		{
			return reinterpret_cast<Internal*>(pInternal)->filePath == nullptr;
		}

		inline const char* getFilePath() const noexcept
		{
			if (pInternal == nullptr) return nullptr;
			const char* filePath = reinterpret_cast<Internal*>(pInternal)->filePath;
			if (filePath) return (filePath == (const char*)SIZE_MAX) ? nullptr : filePath;
			else nullptr;
		}

		inline size_t getHashCode() const noexcept
		{ 
			return pInternal ? reinterpret_cast<Internal*>(pInternal)->hashCode : 0u;
		}
		inline size_t getID() const noexcept
		{ 
			return getHashCode();
		}

		inline bool hasReference() const noexcept { return pInternal != nullptr; }

	private:
		void* pInternal = nullptr;

	};

	class Asset {
	public:
		inline Result loadFromFile(const char* filePath) { return m_Ref.loadFromFile(filePath); }
		inline Result loadFromFile(size_t hashCode) { return m_Ref.loadFromFile(hashCode); }
		inline Result loadFromID(AssetType assetType, size_t ID) { return m_Ref.loadFromID(assetType, ID); }
		inline Result create(AssetType assetType) { return m_Ref.create(assetType); }
		inline Result load(ArchiveI& archive) { return m_Ref.load(archive); }
		inline void save(ArchiveO& archive) { m_Ref.save(archive); }
		inline void unload() { m_Ref.unload(); }

		Result serialize(const char* filePath = nullptr) { return m_Ref.serialize(filePath); }

		inline void* get() const { return m_Ref.get(); }
		inline const char* getAssetTypeStr() const { return m_Ref.getAssetTypeStr(); }

		inline bool isAttachedToFile() const noexcept { return m_Ref.isAttachedToFile(); }
		inline bool isAttachedToID() const noexcept { return m_Ref.isAttachedToID(); }

		inline const char* getFilePath() const noexcept { return m_Ref.getFilePath(); }
		inline size_t getHashCode() const noexcept { return m_Ref.getHashCode(); }
		inline size_t getID() const noexcept { return m_Ref.getID(); }

		inline bool hasReference() const noexcept { return m_Ref.hasReference(); }

		inline AssetRef& getRef() noexcept { return m_Ref; }
		inline const AssetRef& getRef() const noexcept { return m_Ref; }

	protected:
		AssetRef m_Ref;

	};

	struct AssetFile {
		
		AssetType assetType = nullptr;
		void* pInternalAsset = nullptr;
		std::filesystem::file_time_type lastModification;
		ui32 refreshID;

	};

	struct AssetRegisterTypeDesc {

		const char*			name;
		const char**		pExtensions;
		ui32				extensionsCount;
		AssetLoadFromFileFn	loadFileFn;
		AssetLoadFromIDFn	loadIDFn;
		AssetCreateFn		createFn;
		AssetDestroyFn		destroyFn;
		AssetLoadFromFileFn	reloadFileFn;
		AssetSerializeFn	serializeFn;
		AssetIsUnusedFn		isUnusedFn;
		size_t				assetSize;
		float				unusedLifeTime;

	};

	Result asset_refresh();

	void asset_free_unused();
	void asset_free_unused(AssetType assetType);

	Result asset_register_type(const AssetRegisterTypeDesc* desc, AssetType* pAssetType);

	const char* asset_filepath_get(size_t hashCode);
	AssetType asset_type_get(const char* name);
	const std::string& asset_folderpath_get();

	const std::unordered_map<std::string, AssetFile>& asset_map_get();

}