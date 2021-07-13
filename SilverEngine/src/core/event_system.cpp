#include "core/event_system.h"

namespace sv {

    struct EventRegister {
		EventFn function;
		u32 flags;
		u8 data[EVENT_REGISTER_DATA_SIZE];
    };
	
	struct PluginFunction {
		EventFn function;
		Library library;
	};

    struct EventType {
		char name[EVENT_NAME_SIZE + 1u];
		List<EventRegister> registers;
		List<PluginFunction> plugin_functions;
		Mutex mutex;
    };

	struct PluginRegister {

		char    name[PLUGIN_NAME_SIZE + 1u];
		char    filepath[FILEPATH_SIZE + 1u];
		Library library;

#if SV_EDITOR
		Date last_write;
#endif
		
	};

    struct EventSystemState {
		
		Mutex global_mutex;
		ThickHashTable<EventType, 3000u> table;

		List<PluginRegister> plugins;

#if SV_EDITOR
		f32 update_plugins_time;
#endif
		
    };

    EventSystemState* event_system = nullptr;

    bool _event_initialize()
    {
		event_system = SV_ALLOCATE_STRUCT(EventSystemState, "EventSystem");
		
		SV_CHECK(mutex_create(event_system->global_mutex));
		event_system->update_plugins_time = 0.f;
	    
		return true;
    }
    
    bool _event_close()
    {
		if (event_system) {

			mutex_destroy(event_system->global_mutex);

			for (EventType& t : event_system->table) {
				mutex_destroy(t.mutex);
			}

			SV_FREE_STRUCT(event_system);
			event_system = nullptr;
		}
	
		return true;
    }

	SV_AUX void update_plugin_functions_type(EventType& type, PluginRegister* reg, Library old_lib)
	{			
		if (old_lib) {

			foreach(i, type.plugin_functions.size()) {

				if (type.plugin_functions[i].library == old_lib) {
					type.plugin_functions.erase(i);
					break;
				}
			}
		}

		if (reg && reg->library) {
				
			PluginFunction p;
			p.function = (EventFn)library_address(reg->library, type.name);
			p.library = reg->library;

			if (p.function != NULL)
				type.plugin_functions.push_back(p);
		}
	}

	SV_AUX void update_plugin_functions(PluginRegister* reg, Library old_lib)
	{
		mutex_lock(event_system->global_mutex);

		for (EventType& type : event_system->table) {

			mutex_lock(type.mutex);
			update_plugin_functions_type(type, reg, old_lib);
			mutex_unlock(type.mutex);
		}
		
		mutex_unlock(event_system->global_mutex);
	}

	void _event_update()
	{
		
#if SV_EDITOR
		event_system->update_plugins_time += engine.deltatime;

		if (event_system->update_plugins_time > 1.f) {
			event_system->update_plugins_time -= 1.f;

			for (PluginRegister& reg : event_system->plugins) {

				Date last_write;
				if (file_date(reg.filepath, NULL, &last_write, NULL)) {

					if (last_write != reg.last_write) {

						char filepath[FILEPATH_SIZE + 1u] = "$system/bin/";
						string_append(filepath, filepath_name(reg.filepath), FILEPATH_SIZE + 1u);
					
						if (reg.library) {
							library_free(reg.library);
						}

						if (file_copy(reg.filepath, filepath)) {

							Library last_library = reg.library;
							reg.library = library_load(filepath);

							if (reg.library == NULL) {
								SV_LOG_ERROR("Can't load the dynamic library '%s'", reg.filepath);
							}
						
							reg.last_write = last_write;
							update_plugin_functions(&reg, last_library);

							ReloadPluginEvent e;
							string_copy(e.name, reg.name, PLUGIN_NAME_SIZE + 1u);
							e.library = reg.library;

							event_dispatch("reload_plugin", &e);
						}
					}
				}
			}
		}
#endif
		
	}

	bool register_plugin(const char* name, const char* filepath)
	{
		List<PluginRegister>& plugins = event_system->plugins;

		name = string_validate(name);

		if (name[0] == '\0') {
			SV_LOG_ERROR("Empty plugin name");
			return false;
		}
		
		for (const PluginRegister& reg : plugins) {
			if (string_equals(reg.name, name)) {
				SV_LOG_ERROR("Plugin name '%s' duplicated", name);
				return false;
			}
		}

		const char* original_filepath = filepath;
		
#if SV_EDITOR
		char aux_filepath[FILEPATH_SIZE + 1u] = "$system/bin/";
		string_append(aux_filepath, filepath_name(filepath), FILEPATH_SIZE + 1u);
		
		if (!file_copy(filepath, aux_filepath)) {
			SV_LOG_ERROR("Can't copy the dynamic library '%s'", filepath);
		}
		
		filepath = aux_filepath;
#endif
		
		Library lib = library_load(filepath);

		if (lib == 0) {
			SV_LOG_ERROR("Can't load the dynamic library '%s'", filepath);
		}

		PluginRegister& reg = plugins.emplace_back();
		string_copy(reg.name, name, PLUGIN_NAME_SIZE + 1u);
		string_copy(reg.filepath, original_filepath, FILEPATH_SIZE + 1u);
		reg.library = lib;

		SV_LOG_INFO("Plugin registred '%s'", name);

#if SV_EDITOR		
		file_date(filepath, NULL, &reg.last_write, NULL);
#endif

		update_plugin_functions(&reg, NULL);
		
		return true;
	}

	void unregister_plugins()
	{
		mutex_lock(event_system->global_mutex);

		for (PluginRegister& reg : event_system->plugins) {

			Library old_lib = reg.library;
			library_free(reg.library);

			for (EventType& type : event_system->table) {

				mutex_lock(type.mutex);
				update_plugin_functions_type(type, NULL, old_lib);
				mutex_unlock(type.mutex);
			}
		}

		event_system->plugins.clear();
		
		mutex_unlock(event_system->global_mutex);
	}

	u32 get_plugin_count()
	{
		return (u32)event_system->plugins.size();
	}
	
	void get_plugin(char* name, Library* library, u32 index)
	{
		const PluginRegister& p = event_system->plugins[index];
		
		if (name) string_copy(name, p.name, PLUGIN_NAME_SIZE + 1u);
		if (library) *library = p.library;
	}

	bool get_plugin_by_name(const char* name, Library* library)
	{
		foreach(i, event_system->plugins.size()) {
			
			const PluginRegister& p = event_system->plugins[i];
			if (string_equals(p.name, name)) {
				*library = p.library;
				return true;
			}
		}

		return false;
	}

    SV_AUX EventType* find_type(const char* event_name, bool log_not_found)
    {
		size_t event_name_size = strlen(event_name);

		if (event_name_size > EVENT_NAME_SIZE) {
			SV_LOG_ERROR("The event name '%s' exceeds the max name size '%u'", event_name, event_name_size);
			return nullptr;
		}
	
		SV_LOCK_GUARD(event_system->global_mutex, lock);

		EventType* type = event_system->table.find(event_name);
	
		if (type == nullptr) {

			if (log_not_found)
				SV_LOG_ERROR("Event '%s' not found", event_name);
		}
	
		return type;
    }

    void event_unregister_all(const char* event_name)
    {
		SV_LOCK_GUARD(event_system->global_mutex, lock);
		event_system->table.erase(event_name);
    }

	SV_AUX EventType& find_and_create_type(const char* event_name)
	{
		EventType& type = event_system->table[event_name];

		if (!mutex_valid(type.mutex)) {
			mutex_create(type.mutex);
			string_copy(type.name, event_name, EVENT_NAME_SIZE + 1u);

			for (PluginRegister& plugin_reg : event_system->plugins) {
				update_plugin_functions_type(type, &plugin_reg, NULL);
			}
		}

		return type;
	}
    
    bool _event_register(const char* event_name, EventFn event, u32 flags, void* data, u32 data_size)
    {
		size_t event_name_size = strlen(event_name);

		if (event_name_size > EVENT_NAME_SIZE) {
			SV_LOG_ERROR("The event name '%s' exceeds the max name size '%u'", event_name, event_name_size);
			return false;
		}

		if (data && data_size > EVENT_REGISTER_DATA_SIZE) {
			SV_LOG_ERROR("The register data size exceeds the max register data size '%u'", data_size);
			return false;
		}

		mutex_lock(event_system->global_mutex);
		EventType& type = find_and_create_type(event_name);
		mutex_unlock(event_system->global_mutex);

		{
			SV_LOCK_GUARD(type.mutex, lock);

			for (const EventRegister& reg : type.registers) {

				if (reg.function == event) {
					SV_LOG_ERROR("Duplicated event register in '%s'", event_name);
					return false;
				}
			}

			EventRegister& reg = type.registers.emplace_back();
			reg.function = event;
			reg.flags = flags;
			SV_ZERO_MEMORY(reg.data, EVENT_REGISTER_DATA_SIZE);
			
			if (data) {
				memcpy(reg.data, data, data_size);
			}
		}

		return true;
    }
    
    bool _event_unregister(const char* event_name, EventFn event)
    {
		EventType* type = find_type(event_name, true);

		if (type == nullptr) return false;

		{
			SV_LOCK_GUARD(type->mutex, lock);

			foreach (i, type->registers.size()) {

				const EventRegister& reg = type->registers[i];

				if (reg.function == event) {
					type->registers.erase(i);
					return false;
				}
			}
		}

		return true;
    }
    
    void event_dispatch(const char* event_name, void* data)
    {
		mutex_lock(event_system->global_mutex);
		EventType& type = find_and_create_type(event_name);
		mutex_unlock(event_system->global_mutex);

		SV_LOCK_GUARD(type.mutex, lock);
		
		for (EventRegister& reg : type.registers) {

			reg.function(data, reg.data);
		}
		for (PluginFunction& reg : type.plugin_functions) {

			reg.function(data, NULL);
		}
    }

}
