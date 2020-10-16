#pragma once

#include "platform/window.h"

#define svThrow(x) SV_THROW("WINDOW_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Green, "[WINDOW] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Green, "[WINDOW_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[WINDOW_ERROR] "#x, __VA_ARGS__)

namespace sv {

	Result window_initialize(WindowStyle style, const vec4u& bounds, const wchar* title, const wchar* iconFilePath);
	Result window_close();
	void window_update();

}