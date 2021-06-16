#pragma once

#include "defines.h"
#include "utils/serialize.h"
#include "platform/os.h"

#define SV_DEFINE_ASSET_PTR(name, ptr_type) struct name {				\
		SV_INLINE ptr_type get() const noexcept { ptr_type* ptr = reinterpret_cast<ptr_type*>(sv::get_asset_content(asset_ptr)); return ptr ? *ptr : nullptr; } \
		SV_INLINE const char* get_filepath() const noexcept { return sv::get_asset_filepath(asset_ptr); } \
		SV_INLINE const char* get_name() const noexcept { return sv::get_asset_name(asset_ptr); } \
		SV_INLINE void set(void* ptr) const noexcept { ptr_type* p = reinterpret_cast<ptr_type*>(sv::get_asset_content(asset_ptr)); *p = (ptr_type)ptr; } \
		SV_INLINE operator sv::AssetPtr& () { return asset_ptr; }		\
		SV_INLINE operator const sv::AssetPtr& () const { return asset_ptr; } \
		sv::AssetPtr asset_ptr;											\
    }

#define SV_DEFINE_ASSET(name, type) struct name {						\
		SV_INLINE type* get() const noexcept { return reinterpret_cast<type*>(sv::get_asset_content(asset_ptr)); } \
		SV_INLINE const char* get_filepath() const noexcept { return sv::get_asset_filepath(asset_ptr); } \
		SV_INLINE const char* get_name() const noexcept { return sv::get_asset_name(asset_ptr); } \
		SV_INLINE type* operator->() const noexcept { return get(); }	\
		SV_INLINE operator sv::AssetPtr& () { return asset_ptr; }			\
		SV_INLINE operator const sv::AssetPtr& () const { return asset_ptr; } \
		sv::AssetPtr asset_ptr;											\
    }

namespace sv {

	constexpr u32 ASSET_EXTENSION_NAME_SIZE = 10u;
    constexpr u32 ASSET_TYPE_NAME_SIZE = 30u;
	constexpr u32 ASSET_TYPE_EXTENSION_MAX = 7u;
	constexpr u32 ASSET_NAME_SIZE = 20u;

	struct SV_API AssetPtr {

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

	void _initialize_assets();
    void _close_assets();
    void _update_assets();

    SV_API bool create_asset(AssetPtr& asset_ptr, const char* type, const char* name = NULL);

	enum AssetLoadingPriority : u32 {
		AssetLoadingPriority_RightNow,
		AssetLoadingPriority_KeepItLoading,
		AssetLoadingPriority_GetIfExists,
	};

    // Load the asset if exists and use the extension to determine how this file should be treated
    // If it is in use simply get the existing asset
    SV_API bool load_asset_from_file(AssetPtr& asset_ptr, const char* filepath, AssetLoadingPriority priority = AssetLoadingPriority_KeepItLoading);

	SV_API bool load_asset_from_name(AssetPtr& asset_ptr, const char* type, const char* name, bool create_if_not_exists = true);

	SV_API bool        set_asset_name(AssetPtr& asset_ptr, const char* name);
	SV_API const char* get_asset_name(const AssetPtr& asset_ptr);

    SV_API void unload_asset(AssetPtr& asset_ptr);

    SV_API void* get_asset_content(const AssetPtr& asset_ptr);
    SV_API const char* get_asset_filepath(const AssetPtr& asset_ptr);

    typedef bool(*AssetCreateFn)(void* asset);
    typedef bool(*AssetLoadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetReloadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetFreeFn)(void* asset);

    struct AssetTypeDesc {
	
		const char*	      name;
		u32		          asset_size;
		const char**      extensions;
		u32		          extension_count;
		AssetCreateFn     create;
		AssetLoadFileFn	  load_file;
		AssetReloadFileFn reload_file;
		AssetFreeFn	      free;
		f32		          unused_time;

    };

    SV_API bool register_asset_type(const AssetTypeDesc* desc);

    SV_API void update_asset_files();
    SV_API void free_unused_assets();

    SV_INLINE void serialize_asset(Serializer& s, const AssetPtr& asset_ptr)
    {
		constexpr u32 VERSION = 0u;

		serialize_u32(s, VERSION);
		
		if (asset_ptr.ptr == nullptr) serialize_bool(s, false);
		else {
			const char* filepath = get_asset_filepath(asset_ptr);

			if (filepath == NULL) {
				serialize_bool(s, false);
			}
			else {

				serialize_bool(s, true);
				serialize_string(s, filepath);

				const char* name = get_asset_name(asset_ptr);
				if (name == NULL) {
					serialize_bool(s, false);
				}
				else {
					serialize_string(s, name);
				}
			}
		}
    }

    SV_INLINE void deserialize_asset(Deserializer& d, AssetPtr& asset_ptr, AssetLoadingPriority priority = AssetLoadingPriority_KeepItLoading)
    {
		u32 version;
		deserialize_u32(d, version);

		if (version == 0u) {

			bool attached_to_file;
			deserialize_bool(d, attached_to_file);

			if (attached_to_file) {
				
				char filepath[FILEPATH_SIZE + 1u];

				size_t size = deserialize_string_size(d);

				if (size > FILEPATH_SIZE) {
					SV_LOG_ERROR("The asset filepath size of %ul exceeds the size limit of %ul", size, FILEPATH_SIZE);
				}

				deserialize_string(d, filepath, FILEPATH_SIZE + 1u);

				if (!load_asset_from_file(asset_ptr, filepath, priority)) {
					SV_LOG_ERROR("Can't load the asset '%s'", filepath);
				}

				bool has_name;
				deserialize_bool(d, has_name);

				if (has_name) {
					char name[ASSET_NAME_SIZE + 1u];

					size_t size = deserialize_string_size(d);

					if (size > ASSET_NAME_SIZE) {
						SV_LOG_ERROR("The asset name size of %ul exceeds the size limit of %ul", size, ASSET_NAME_SIZE);
					}

					deserialize_string(d, name, ASSET_NAME_SIZE + 1u);

					if (!set_asset_name(asset_ptr, name)) {
						SV_LOG_ERROR("Can't set the asset name '%s'", name);
					}
				}
			}
		}
    }

}
