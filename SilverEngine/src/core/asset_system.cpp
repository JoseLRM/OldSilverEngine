#include "core/asset_system.h"
#include "utils/allocators.h"

namespace sv {

    // Time to update one asset type, after this time will update the next type
    constexpr float UNUSED_CHECK_TIME = 1.f;

    struct AssetType_internal {

	std::string	  name;
	AssetCreateFn	  create;
	AssetLoadFileFn	  load_file;
	AssetFreeFn 	  free;
	f32		  unused_time;
	f64		  last_update = 0.0;
	List<const char*> extensions;

	SizedInstanceAllocator allocator;

	AssetType_internal(u32 size) : allocator(size, 30u) {}

    };

    struct Asset_internal {

	std::atomic<i32>	ref_count = 0;
	f32					unused_time = f32_max;
	const char*			filepath = nullptr;
	AssetType_internal* type = nullptr;

    };

    static List<AssetType_internal*> asset_types;

    static std::unordered_map<std::string, Asset_internal*> filepath_map;
    static std::unordered_map<std::string, AssetType_internal*> extension_map;

    static u32 check_type_index = 0u;
    static f32 check_time = 0.f;
    static List<Asset_internal*> free_assets_list;

    SV_INLINE static bool destroy_asset(Asset_internal* asset, AssetType_internal* type)
    {
	const char* filepath = asset->filepath;

	bool res = type->free(asset + 1u);
	type->allocator.free(asset);

	if (filepath) {
	    SV_LOG_INFO("%s freed: %s", type->name.c_str(), filepath);
	    filepath_map.erase(filepath);
	}
	else {
	    SV_LOG_INFO("%s freed", type->name.c_str());
	}

	return res;
    }

    void update_assets()
    {
	check_time += engine.deltatime;

	if (check_time >= UNUSED_CHECK_TIME) {

	    AssetType_internal* type = asset_types[check_type_index];
	    free_assets_list.reset();

	    f64 now = timer_now();
	    f32 elapsed_time = f32(now - type->last_update);

	    for (auto& pool : type->allocator) {
		for (void* _ptr : pool) {

		    Asset_internal* asset = reinterpret_cast<Asset_internal*>(_ptr);

		    i32 ref_count = asset->ref_count.load();

		    if (ref_count <= 0) {

			if (asset->unused_time == f32_max) {

			    asset->unused_time = type->unused_time;
			}

			asset->unused_time -= elapsed_time;

			if (asset->unused_time <= 0.f) {

			    free_assets_list.push_back(asset);
			}
		    }
		    else {
			asset->unused_time = f32_max;
		    }
		}
	    }

	    // Free assets
	    for (Asset_internal* asset : free_assets_list) {

		destroy_asset(asset, type);
		// TODO: this can fail...
	    }

	    type->last_update = now;
	    check_type_index = (check_type_index + 1u) % u32(asset_types.size());
	    check_time -= UNUSED_CHECK_TIME;
	}
    }

    void close_assets()
    {
	free_unused_assets();

	for (AssetType_internal* type : asset_types) {

	    type->allocator.clear();
	    delete type;
	}
	asset_types.clear();

	filepath_map.clear();
	extension_map.clear();

	free_assets_list.clear();
    }

    SV_INLINE static AssetType_internal* get_type_from_filepath(size_t size, const char* filepath)
    {
	const char* extension = nullptr;

	const char* it = filepath + size;
	const char* end = filepath - 1u;

	while (it != end) {

	    if (*it == '.') {
		++it;

		extension = it;
		break;
	    }

	    --it;
	}

	if (extension) {
			
	    auto it = extension_map.find(extension);

	    if (it == extension_map.end()) {

		SV_LOG_ERROR("Unknown extension '%s'", extension);
		return nullptr;
	    }
	    else {
		return it->second;
	    }
	}
	else {

	    SV_LOG_ERROR("The filepath '%s' has no extension", filepath);
	    return nullptr;
	}
    }

    SV_INLINE static AssetType_internal* get_type_from_typename(const char* name)
    {
	for (AssetType_internal* type : asset_types) {
	    if (strcmp(type->name.c_str(), name) == 0) {
		return type;
	    }
	}
	return nullptr;
    }

    bool create_asset(AssetPtr& asset_ptr, const char* asset_type_name)
    {
	AssetType_internal* type = get_type_from_typename(asset_type_name);
	if (type == nullptr) {
	    SV_LOG_ERROR("Asset type '%s' not found", asset_type_name);
	    return false;
	}

	if (type->create == nullptr) {
	    SV_LOG_ERROR("The asset type '%s' can't create assets by default", asset_type_name);
	    return false;
	}

	Asset_internal* asset = new(type->allocator.alloc()) Asset_internal();
	asset->type = type;

	if (!type->create(asset + 1u)) {

	    type->allocator.free(asset);
	    return false;
	}

	asset_ptr = AssetPtr(asset);

	SV_LOG_INFO("%s created", type->name.c_str());

	return true;
    }

    bool load_asset_from_file(AssetPtr& asset_ptr, const char* filepath)
    {
	if (filepath == nullptr) return false;

	auto it = filepath_map.find(filepath);

	if (it == filepath_map.end()) {

	    size_t size = strlen(filepath);
	    AssetType_internal* type = get_type_from_filepath(size, filepath);
	    if (type == nullptr) return false;

	    Asset_internal* asset = new(type->allocator.alloc()) Asset_internal();
	    asset->type = type;

	    if (!type->load_file(asset + 1u, filepath)) {

		SV_LOG_ERROR("Can't load the asset '%s'", filepath);

		type->allocator.free(asset);
		return false;
	    }

	    filepath_map[filepath] = asset;
	    asset->filepath = filepath_map.find(filepath)->first.c_str();
	    asset_ptr = AssetPtr(asset);

	    SV_LOG_INFO("%s loaded: %s", type->name.c_str(), filepath);
	}
	else {
	    asset_ptr = AssetPtr(it->second);
	}

	return true;
    }

    void unload_asset(AssetPtr& asset_ptr)
    {
	asset_ptr.~AssetPtr();
    }

    void* get_asset_content(const AssetPtr& asset_ptr)
    {
	if (asset_ptr.ptr) return reinterpret_cast<Asset_internal*>(asset_ptr.ptr) + 1u;
	return nullptr;
    }

    const char* get_asset_filepath(const AssetPtr& asset_ptr)
    {
	if (asset_ptr.ptr) return reinterpret_cast<Asset_internal*>(asset_ptr.ptr)->filepath;
	return nullptr;
    }

    bool register_asset_type(const AssetTypeDesc* desc)
    {
	// TODO: Check if the extensions or the name is repeated

	AssetType_internal*& type = asset_types.emplace_back();
	type = new AssetType_internal(desc->asset_size + sizeof(Asset_internal));

	type->name = desc->name;
	type->create = desc->create;
	type->load_file = desc->load_file;
	type->free = desc->free;
	type->unused_time = desc->unused_time;

	type->extensions.resize(desc->extension_count);
	foreach(i, desc->extension_count) {
	    extension_map[desc->extensions[i]] = type;
	    type->extensions[i] = extension_map.find(desc->extensions[i])->first.c_str();
	}

	return true;
    }

    void update_asset_files()
    {
	SV_LOG("TODO");
    }

    void free_unused_assets()
    {
	foreach(i, 4u) {

	    for (AssetType_internal* type : asset_types) {

		free_assets_list.reset();

		for (auto& pool : type->allocator) {
		    for (void* _ptr : pool) {

			Asset_internal* asset = reinterpret_cast<Asset_internal*>(_ptr);

			if (asset->ref_count.load() <= 0) {

			    free_assets_list.push_back(asset);
			}
		    }
		}

		// Free assets
		for (Asset_internal* asset : free_assets_list) {

		    destroy_asset(asset, type);
		    // TODO: this can fail...
		}
	    }
	}
    }

}
