#pragma once

#include "core.h"

namespace sv {

	struct AssetPtr {

		AssetPtr() = default;

		AssetPtr::AssetPtr(void* ptr) : ptr(ptr)
		{
			std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
			ref->fetch_add(1);
		}

		AssetPtr::~AssetPtr()
		{
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
				ptr = nullptr;
			}
		}

		AssetPtr::AssetPtr(const AssetPtr& other)
		{
			if (other.ptr) {
				ptr = other.ptr;
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(1);
			}
		}

		AssetPtr& AssetPtr::operator=(const AssetPtr& other)
		{
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
			}

			ptr = other.ptr;

			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(1);
			}

			return *this;
		}

		AssetPtr::AssetPtr(AssetPtr&& other) noexcept
		{
			ptr = other.ptr;
			other.ptr = nullptr;
		}

		AssetPtr& AssetPtr::operator=(AssetPtr&& other) noexcept
		{
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
			}

			ptr = other.ptr;
			other.ptr = nullptr;

			return *this;
		}

		inline bool operator==(const AssetPtr& other) const noexcept { return ptr == other.ptr; }
		inline bool operator!=(const AssetPtr& other) const noexcept { return ptr != other.ptr; }

		void* ptr = nullptr;

	};

	Result	create_asset(AssetPtr& asset_ptr, const char* asset_type_name);
	Result	load_asset_from_file(AssetPtr& asset_ptr, const char* filepath);
	void	unload_asset(AssetPtr& asset_ptr);

	void*		get_asset_content(const AssetPtr& asset_ptr);
	const char* get_asset_filepath(const AssetPtr& asset_ptr);

	typedef Result(*AssetCreateFn)(void* asset);
	typedef Result(*AssetLoadFileFn)(void* asset, const char* filepath);
	typedef Result(*AssetReloadFileFn)(void* asset, const char* filepath);
	typedef Result(*AssetFreeFn)(void* asset);

	struct AssetTypeDesc {
	
		std::string			name;
		u32					asset_size;
		const char**		extensions;
		u32					extension_count;
		AssetCreateFn		create;
		AssetLoadFileFn		load_file;
		AssetReloadFileFn	reload_file;
		AssetFreeFn			free;
		f32					unused_time;

	};

	Result register_asset_type(const AssetTypeDesc* desc);

	void update_asset_files();
	void free_unused_assets();

	SV_INLINE void load_asset(ArchiveI& archive, AssetPtr& asset_ptr)
	{
		u8 type;
		archive >> type;

		switch (type)
		{
		case 1u:
		{
			std::string filepath;
			archive >> filepath;

			if (result_fail(load_asset_from_file(asset_ptr, filepath.c_str()))) {
				SV_LOG_ERROR("Can't load the asset '%s'", filepath.c_str());
			}
		}break;
		}
	}

	SV_INLINE void save_asset(ArchiveO& archive, const AssetPtr& asset_ptr)
	{
		if (asset_ptr.ptr == nullptr) archive << u8(0u);
		else {
			const char* filepath = get_asset_filepath(asset_ptr);

			if (filepath == nullptr) {
				archive << (u8)(0u);
			}
			else {

				archive << (u8)(1u);
				archive << filepath;
			}
		}
	}

}