#pragma once

#include "defines.h"

// Console

#if SV_DEV

namespace sv {
	
    typedef bool(*CommandFn)(const char** args, u32 argc);
    
    SV_API bool execute_command(const char* command);
    SV_API bool register_command(const char* name, CommandFn command_fn);

    SV_API void console_print(const char* str, ...);
    SV_API void console_notify(const char* title, const char* str, ...);
    SV_API void console_clear();

    void _console_initialize();
    void _console_close();
    void _console_update();
    void _console_draw();
    
}

#define SV_LOG_CLEAR() sv::console_clear()
#define SV_LOG_SEPARATOR() sv::console_print("----------------------------------------------------\n")
#define SV_LOG(x, ...) sv::console_print(x "\n", __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::console_notify("INFO", x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::console_notify("WARNING", x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::console_notify("ERROR", x, __VA_ARGS__)

#else

#define SV_LOG_CLEAR() do{}while(0)
#define SV_LOG_SEPARATOR() do{}while(0)
#define SV_LOG(x, ...) do{}while(0)
#define SV_LOG_INFO(x, ...) do{}while(0)
#define SV_LOG_WARNING(x, ...) sv::printf("[WARNING] " x "\n", __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::printf("[ERROR] " x "\n", __VA_ARGS__)

#endif

// Assertion

namespace sv {
    SV_API void throw_assertion(const char* content, u32 line, const char* file);
}

#if SV_SLOW
#define SV_ASSERT(x) do{ if (!(x)) { sv::throw_assertion(#x, __LINE__, __FILE__); } } while(0)
#else
#define SV_ASSERT(x)
#endif
