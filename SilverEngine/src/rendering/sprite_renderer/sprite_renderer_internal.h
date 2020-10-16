#pragma once

#include "rendering/sprite_renderer.h"

#define svThrow(x) SV_THROW("SPRITE_RENDERER_ERROR", x)
#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[SPRITE_RENDERER] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Blue | sv::LoggingStyle_Red, "[SPRITE_RENDERER_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[SPRITE_RENDERER_ERROR] "#x, __VA_ARGS__)

namespace sv {

	Result sprite_renderer_initialize();
	Result sprite_renderer_close();

}