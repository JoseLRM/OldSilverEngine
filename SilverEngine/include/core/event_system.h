#pragma once

#include "defines.h"

namespace sv {

    constexpr u32 EVENTNAME_SIZE = 50u;
    constexpr u32 REGISTER_DATA_SIZE = 16u;

    bool _event_initialize();
    bool _event_close();

    typedef void(EventFn*)(void* register_data, void* event_data);

    SV_API bool event_register(const char* event_name, EventFn event, void* data, u32 data_size);
    SV_API bool event_unregister(const char* event_name, EventFn event);
    
    SV_API void event_dispatch(const char* event_name, void* data);

    SV_INLINE bool event_register(const char* event_name, EventFn event)
    {
	return event_register(event_name, event, nullptr, 0u);
    }
    
}
