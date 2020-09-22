#pragma once

#include "task_system.h"

#define svLog(x, ...) sv::console_log(sv::LoggingStyle_Red | sv::LoggingStyle_Green, "[TASKSYSTEM] "#x, __VA_ARGS__)
#define svLogWarning(x, ...) sv::console_log(sv::LoggingStyle_Red | sv::LoggingStyle_Green, "[TASKSYSTEM_WARNING] "#x, __VA_ARGS__)
#define svLogError(x, ...) sv::console_log(sv::LoggingStyle_Red, "[TASKSYSTEM_ERROR] "#x, __VA_ARGS__)

namespace sv {

	Result task_initialize(ui32 minThreadCount);
	Result task_close();

}