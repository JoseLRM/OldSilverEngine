#pragma once

#include "core.h"

namespace sv {

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

#define SV_DEFINE_ASSET_PTR(name, ptr_type) struct name {		\
	SV_INLINE ptr_type get() const noexcept { ptr_type* ptr = reinterpret_cast<ptr_type*>(get_asset_content(asset_ptr)); return ptr ? *ptr : nullptr; } \
	SV_INLINE operator AssetPtr& () { return asset_ptr; }		\
	SV_INLINE operator const AssetPtr& () const { return asset_ptr; } \
	AssetPtr asset_ptr;						\
    }

#define SV_DEFINE_ASSET(name, type) struct name {			\
	SV_INLINE type* get() const noexcept { return reinterpret_cast<type*>(get_asset_content(asset_ptr)); } \
	SV_INLINE type* operator->() const noexcept { return get(); }	\
	SV_INLINE operator AssetPtr& () { return asset_ptr; }		\
	SV_INLINE operator const AssetPtr& () const { return asset_ptr; } \
	AssetPtr asset_ptr;						\
    }

    SV_API bool create_asset(AssetPtr& asset_ptr, const char* asset_type_name);
    SV_API bool load_asset_from_file(AssetPtr& asset_ptr, const char* filepath);
    SV_API void unload_asset(AssetPtr& asset_ptr);

    SV_API void* get_asset_content(const AssetPtr& asset_ptr);
    SV_API const char* get_asset_filepath(const AssetPtr& asset_ptr);

    typedef bool(*AssetCreateFn)(void* asset);
    typedef bool(*AssetLoadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetReloadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetFreeFn)(void* asset);

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

    SV_API bool register_asset_type(const AssetTypeDesc* desc);

    SV_API void update_asset_files();
    SV_API void free_unused_assets();

    SV_INLINE Archive& operator>>(Archive& archive, AssetPtr& asset_ptr)
    {
	u8 type;
	archive >> type;

	switch (type)
	{
	case 1u:
	{
	    std::string filepath;
	    archive >> filepath;

	    if (!load_asset_from_file(asset_ptr, filepath.c_str())) {
		SV_LOG_ERROR("Can't load the asset '%s'", filepath.c_str());
	    }
	}break;
	}

	return archive;
    }

    SV_INLINE Archive& operator<<(Archive& archive, const AssetPtr& asset_ptr)
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

	return archive;
    }

}
