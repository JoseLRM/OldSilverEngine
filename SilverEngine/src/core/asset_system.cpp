#include "core/asset_system.h"
#include "utils/allocators.h"
#include "utils/string.h"

namespace sv {

    // Time to update one asset type, after this time will update the next type
    constexpr float UNUSED_CHECK_TIME = 1.f;

	struct Asset_internal;

    struct AssetType_internal {

		char	          name[ASSET_TYPE_NAME_SIZE + 1u];
		AssetCreateFn	  create_fn;
		AssetLoadFileFn	  load_file_fn;
		AssetReloadFileFn reload_file_fn;
		AssetFreeFn 	  free_fn;
		f32		          unused_time;
		f64		          last_update = 0.0;
		u32               extension_count;
		char              extensions[ASSET_EXTENSION_NAME_SIZE + 1u][ASSET_TYPE_EXTENSION_MAX];
		
		ThickHashTable<Asset_internal*, 100u> name_table;
		
		SizedInstanceAllocator allocator;

		AssetType_internal(u32 size) : allocator(size, 30u) {}

    };

    struct Asset_internal {

		std::atomic<i32>	ref_count = 0;
		f32					unused_time = f32_max;
		// TODO: Move on
		char			    filepath[FILEPATH_SIZE + 1u] = "";
		Date                last_write_date;
		char			    name[ASSET_NAME_SIZE + 1u] = "";
		AssetType_internal* type = NULL;

    };

	struct AssetSystemData {
		
		List<AssetType_internal*>             asset_types;
		ThickHashTable<Asset_internal*, 2000> filepath_table;
		ThickHashTable<AssetType_internal*, 100>   extension_table;

		u32 check_type_index = 0u;
		f32 check_time = 0.f;
		List<Asset_internal*> free_assets_list;
		
	};

    static AssetSystemData* asset_system = NULL;

    SV_AUX bool destroy_asset(Asset_internal* asset, AssetType_internal* type)
    {
		bool res = type->free_fn(asset + 1u);
		type->allocator.free(asset);

		bool log = false;

		if (string_size(asset->name)) {
			
			if (!log) {
				SV_LOG_INFO("%s freed: %s", type->name, asset->name);
				log = true;
			}
			
			asset_system->filepath_table.erase(asset->filepath);
		}
		
		if (string_size(asset->filepath)) {

			if (!log) {
				SV_LOG_INFO("%s freed: %s", type->name, asset->filepath);
				log = true;
			}
			
			asset_system->filepath_table.erase(asset->filepath);
		}

		if (!log) {
			SV_LOG_INFO("%s freed", type->name);
			log = true;
		}

		return res;
    }

    void _update_assets()
    {
		asset_system->check_time += engine.deltatime;

		if (asset_system->check_time >= UNUSED_CHECK_TIME) {

			AssetType_internal* type = asset_system->asset_types[asset_system->check_type_index];
			asset_system->free_assets_list.reset();

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

							asset_system->free_assets_list.push_back(asset);
						}
					}
					else {
						asset->unused_time = f32_max;

#if SV_EDITOR
						if (asset->filepath[0] && type->reload_file_fn) {

							Date last_write;
							if (file_date(asset->filepath, NULL, &last_write, NULL)) {

								if (asset->last_write_date != last_write) {

									if (type->reload_file_fn(asset + 1u, asset->filepath)) {
										SV_LOG_INFO("%s asset reloaded: '%s'", type->name, asset->filepath);
									}
									else {
										SV_LOG_ERROR("Can't reload the %s asset: '%s'", type->name, asset->filepath);
									}

									asset->last_write_date = last_write;
								}
							}
						}
#endif
					}
				}
			}

			// Free assets
			for (Asset_internal* asset : asset_system->free_assets_list) {

				destroy_asset(asset, type);
				// TODO: this can fail...
			}

			type->last_update = now;
			asset_system->check_type_index = (asset_system->check_type_index + 1u) % u32(asset_system->asset_types.size());
			asset_system->check_time -= UNUSED_CHECK_TIME;
		}
    }

	void _initialize_assets()
	{
		asset_system = SV_ALLOCATE_STRUCT(AssetSystemData);
	}

    void _close_assets()
    {
		if (asset_system) {
			
			free_unused_assets();

			for (AssetType_internal* type : asset_system->asset_types) {

				type->allocator.clear();
				delete type;
			}
			asset_system->asset_types.clear();

			asset_system->filepath_table.clear();
			asset_system->extension_table.clear();

			asset_system->free_assets_list.clear();

			SV_FREE_STRUCT(asset_system);
			asset_system = NULL;
		}
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
			
			AssetType_internal** type = asset_system->extension_table.find(extension);

			if (type == NULL) {
				
				SV_LOG_ERROR("Unknown extension '%s'", extension);
			}
			
			return *type;
		}
		else {

			SV_LOG_ERROR("The filepath '%s' has no extension", filepath);
			return nullptr;
		}
    }

    SV_AUX AssetType_internal* get_type_from_typename(const char* name)
    {
		for (AssetType_internal* type : asset_system->asset_types) {
			if (string_equals(type->name, name)) {
				return type;
			}
		}
		return nullptr;
    }

	SV_AUX bool is_valid_asset_name(AssetType_internal* type, const char* name)
	{
		if (string_size(name) > ASSET_NAME_SIZE) {
			SV_LOG_ERROR("The asset name '%s' is too large", name);
			return false;
		}

		if (type->name_table.find(name)) {
			SV_LOG_ERROR("The asset name '%s' is currently used", name);
			return false;
		}

		return true;
	}

    bool create_asset(AssetPtr& asset_ptr, const char* asset_type_name, const char* name)
    {
		AssetType_internal* type = get_type_from_typename(asset_type_name);
		if (type == nullptr) {
			SV_LOG_ERROR("Asset type '%s' not found", asset_type_name);
			return false;
		}

		if (type->create_fn == nullptr) {
			SV_LOG_ERROR("The asset type '%s' can't create assets by default", asset_type_name);
			return false;
		}

		name = string_validate(name);

		if (!is_valid_asset_name(type, name)) {
			return false;
		}

		Asset_internal* asset = new(type->allocator.alloc()) Asset_internal();
		asset->type = type;
		string_copy(asset->name, name, ASSET_NAME_SIZE + 1u);

		if (!type->create_fn(asset + 1u)) {

			type->allocator.free(asset);
			return false;
		}

		if (string_size(name))
			type->name_table[name] = asset;

		asset_ptr = AssetPtr(asset);

		SV_LOG_INFO("%s created", type->name);

		return true;
    }

	SV_AUX bool load_asset_right_now(AssetPtr& asset_ptr, const char* filepath)
	{
		Asset_internal** asset_ = asset_system->filepath_table.find(filepath);

		if (asset_ == NULL) {

			size_t size = string_size(filepath);
			AssetType_internal* type = get_type_from_filepath(size, filepath);
			if (type == NULL) return false;

			Asset_internal* asset = new(type->allocator.alloc()) Asset_internal();
			asset->type = type;

			if (!type->load_file_fn(asset + 1u, filepath)) {

				SV_LOG_ERROR("Can't load the asset '%s'", filepath);

				type->allocator.free(asset);
				return false;
			}

			asset_system->filepath_table[filepath] = asset;
			string_copy(asset->filepath, filepath, FILEPATH_SIZE + 1u);
			asset_ptr = AssetPtr(asset);

			file_date(filepath, NULL, &asset->last_write_date, NULL);

			SV_LOG_INFO("%s loaded: %s", type->name, filepath);
		}
		else {
			asset_ptr = AssetPtr(*asset_);
		}

		return true;
	}

	SV_AUX bool load_asset_keep_it_loading(AssetPtr& asset_ptr, const char* filepath)
	{
		// TODO
		return load_asset_right_now(asset_ptr, filepath);
	}

	SV_AUX bool load_asset_get_if_exists(AssetPtr& asset_ptr, const char* filepath)
	{
		Asset_internal** asset_ = asset_system->filepath_table.find(filepath);

		if (asset_) {
			asset_ptr = AssetPtr(*asset_);
			return true;
		}

		return false;
	}

    bool load_asset_from_file(AssetPtr& asset_ptr, const char* filepath, AssetLoadingPriority priority)
    {
		if (filepath == nullptr) return false;

		switch (priority) {

		case AssetLoadingPriority_RightNow:
			return load_asset_right_now(asset_ptr, filepath);

		case AssetLoadingPriority_KeepItLoading:
			return load_asset_keep_it_loading(asset_ptr, filepath);

		case AssetLoadingPriority_GetIfExists:
			return load_asset_get_if_exists(asset_ptr, filepath);
			
		}

		return false;
    }

	bool load_asset_from_name(AssetPtr& asset_ptr, const char* asset_type_name, const char* name, bool create_if_not_exists)
	{
		AssetType_internal* type = get_type_from_typename(asset_type_name);
		if (type == NULL) {
			SV_LOG_ERROR("Asset type '%s' not found", asset_type_name);
			return false;
		}

		Asset_internal** asset_ = type->name_table.find(name);
		
		if (asset_) {

			asset_ptr = AssetPtr(*asset_);
			return true;
		}
		else if (create_if_not_exists) {

			return create_asset(asset_ptr, asset_type_name, name);
		}

		return false;
	}

    void unload_asset(AssetPtr& asset_ptr)
    {
		asset_ptr.~AssetPtr();
    }

	bool set_asset_name(AssetPtr& asset_ptr, const char* name)
	{		
		if (asset_ptr.ptr) {
			
			Asset_internal* asset = reinterpret_cast<Asset_internal*>(asset_ptr.ptr);

			name = string_validate(name);

			if (!is_valid_asset_name(asset->type, name)) {
				return false;
			}

			if (asset->name[0]) {

				asset->type->name_table.erase(asset->name);
			}

			asset->type->name_table[name] = asset;
			string_copy(asset->name, name, ASSET_NAME_SIZE + 1u);

			return true;
		}
		return false;
	}
	
	const char* get_asset_name(const AssetPtr& asset_ptr)
	{
		if (asset_ptr.ptr) {
			
			const char* name = reinterpret_cast<Asset_internal*>(asset_ptr.ptr)->name;
			if (name[0]) return name;
			return NULL;
		}
		return NULL;
	}

    void* get_asset_content(const AssetPtr& asset_ptr)
    {
		if (asset_ptr.ptr) return reinterpret_cast<Asset_internal*>(asset_ptr.ptr) + 1u;
		return nullptr;
    }

    const char* get_asset_filepath(const AssetPtr& asset_ptr)
    {
		if (asset_ptr.ptr) {
			
			const char* filepath = reinterpret_cast<Asset_internal*>(asset_ptr.ptr)->filepath;
			if (filepath[0]) return filepath;
			return NULL;
		}
		return NULL;
    }

    bool register_asset_type(const AssetTypeDesc* desc)
    {
		// TODO: Check if the extensions or the name is repeated or if the extension name is too large

		AssetType_internal*& type = asset_system->asset_types.emplace_back();
		type = (AssetType_internal*)SV_ALLOCATE_MEMORY(sizeof(AssetType_internal));
		new(type) AssetType_internal(desc->asset_size + sizeof(Asset_internal));

		string_copy(type->name, desc->name, ASSET_TYPE_NAME_SIZE + 1u);
		type->create_fn = desc->create_fn;
		type->load_file_fn = desc->load_file_fn;
		type->reload_file_fn = desc->reload_file_fn;
		type->free_fn = desc->free_fn;
		type->unused_time = desc->unused_time;

		type->extension_count = desc->extension_count;
		foreach(i, desc->extension_count) {

			const char* ext = desc->extensions[i];
			
			asset_system->extension_table[ext] = type;
			string_copy(type->extensions[i], ext, ASSET_EXTENSION_NAME_SIZE + 1u);
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

			for (AssetType_internal* type : asset_system->asset_types) {

				asset_system->free_assets_list.reset();

				for (auto& pool : type->allocator) {
					for (void* _ptr : pool) {

						Asset_internal* asset = reinterpret_cast<Asset_internal*>(_ptr);

						if (asset->ref_count.load() <= 0) {

							asset_system->free_assets_list.push_back(asset);
						}
					}
				}

				// Free assets
				for (Asset_internal* asset : asset_system->free_assets_list) {

					destroy_asset(asset, type);
					// TODO: this can fail...
				}
			}
		}
    }

}
