#pragma once

#include "defines.h"

namespace sv {

    constexpr u32 EVENTNAME_SIZE = 50u;
    constexpr u32 REGISTER_DATA_SIZE = 16u;

    enum EventFlag : u32 {
	EventFlag_None = 0u,
	EventFlag_User = SV_BIT(0u),
    };
    
    bool _event_initialize();
    bool _event_close();

    typedef void(*EventFn)(void* event_data, void* register_data);

    SV_API bool _event_register(const char* event_name, EventFn event, u32 flags, void* data, u32 data_size);
    SV_API bool _event_unregister(const char* event_name, EventFn event);
    SV_API void event_unregister_flags(u32 flags);
    
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
    SV_INLINE bool event_register(const char* event_name, T event, u32 flags)
    {
	return event_register(event_name, event, flags, nullptr, 0u);
    }

    template<typename T>
    SV_INLINE bool event_user_register(const char* event_name, T event, u32 flags = EventFlag_None)
    {
	return event_register(event_name, event, flags | EventFlag_User);
    }
    
}
