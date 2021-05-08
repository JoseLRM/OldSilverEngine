#include "core/event_system.h"

namespace sv {

    struct EventRegister {
	EventFn function;
	u32 flags;
	u8 data[REGISTER_DATA_SIZE];
    };

    struct EventType {
	char name[EVENTNAME_SIZE + 1u];
	List<EventRegister> registers;
	// TODO: use custom mutex
	std::mutex mutex;
    };

    struct EventSystemState {

	// TODO: use custom mutex
	std::mutex global_mutex;
	ThickHashTable<EventType, 3000u> table;
    };

    EventSystemState* event_system = nullptr;

    bool _event_initialize()
    {
	event_system = SV_ALLOCATE_STRUCT(EventSystemState);
	    
	return true;
    }
    
    bool _event_close()
    {
	if (event_system) {

	    SV_FREE_STRUCT(event_system);
	    event_system = nullptr;
	}
	
	return true;
    }
    
    void event_unregister_flags(u32 flags)
    {
	if (event_system) {

	    event_system->global_mutex.lock();

	    for (EventType& t : event_system->table) {

		t.mutex.lock();

		u32 i = 0u;
		while (i < t.registers.size()) {

		    EventRegister& reg = t.registers[i];
		    
		    if ((reg.flags & flags) == flags) {

			t.registers.erase(i);
		    }
		    else ++i;
		}

		t.mutex.unlock();
	    }
	    
	    event_system->global_mutex.unlock();
	}
    }

    SV_AUX EventType* find_type(const char* event_name, bool log_not_found)
    {
	size_t event_name_size = strlen(event_name);

	if (event_name_size > EVENTNAME_SIZE) {
	    SV_LOG_ERROR("The event name '%s' exceeds the max name size '%u'", event_name, event_name_size);
	    return nullptr;
	}
	
	std::lock_guard<std::mutex> lock(event_system->global_mutex);

	EventType* type = event_system->table.find(event_name);
	
	if (type == nullptr) {

	    if (log_not_found)
		SV_LOG_ERROR("Event '%s' not found", event_name);
	}
	
	return type;
    }
    
    bool _event_register(const char* event_name, EventFn event, u32 flags, void* data, u32 data_size)
    {
	size_t event_name_size = strlen(event_name);

	if (event_name_size > EVENTNAME_SIZE) {
	    SV_LOG_ERROR("The event name '%s' exceeds the max name size '%u'", event_name, event_name_size);
	    return false;
	}

	if (data && data_size > REGISTER_DATA_SIZE) {
		SV_LOG_ERROR("The register data size exceeds the max register data size '%u'", data_size);
		return false;
	}

	event_system->global_mutex.lock();
	EventType& type = event_system->table[event_name];
	event_system->global_mutex.unlock();

	{
	    std::lock_guard<std::mutex> lock(type.mutex);

	    for (const EventRegister& reg : type.registers) {

		if (reg.function == event) {
		    SV_LOG_ERROR("Duplicated event register in '%s'", event_name);
		    return false;
		}
	    }

	    EventRegister& reg = type.registers.emplace_back();
	    reg.function = event;
	    reg.flags = flags;
	    SV_ZERO_MEMORY(reg.data, REGISTER_DATA_SIZE);

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
	    std::lock_guard<std::mutex> lock(type->mutex);

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
	EventType* type = find_type(event_name, false);

	if (type == nullptr) return;

	std::lock_guard<std::mutex> lock(type->mutex);

	for (EventRegister& reg : type->registers) {

	    reg.function(data, reg.data);
	}
    }

}
