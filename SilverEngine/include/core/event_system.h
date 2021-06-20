#pragma once

#include "defines.h"

namespace sv {

    constexpr u32 EVENT_NAME_SIZE = 50u;
    constexpr u32 EVENT_REGISTER_DATA_SIZE = 16u;
	constexpr u32 PLUGIN_NAME_SIZE = 30u;

    enum EventFlag : u32 {
    };
    
    bool _event_initialize();
    bool _event_close();
	void _event_update();

	bool register_plugin(const char* name, const char* filepath);
	void unregister_plugins();

    typedef void(*EventFn)(void* event_data, void* register_data);

    SV_API bool _event_register(const char* event_name, EventFn event, u32 flags, void* data, u32 data_size);
    SV_API bool _event_unregister(const char* event_name, EventFn event);
    SV_API void event_unregister_all(const char* event_name);
    
    SV_API void event_dispatch(const char* event_name, void* data);

    template<typename T>
    SV_INLINE bool event_register(const char* event_name, T event, u32 flags, void* data, u32 data_size)
    {
		return _event_register(event_name, (EventFn) event, flags, data, data_size);
    }

    template<typename T>
    SV_INLINE bool event_unregister(const char* event_name, T event)
    {
		return _event_unregister(event_name, (EventFn) event);
    }

    template<typename T>
    SV_INLINE bool event_register(const char* event_name, T event, u32 flags = 0u)
    {
		return event_register(event_name, event, flags, nullptr, 0u);
    }
    
}
