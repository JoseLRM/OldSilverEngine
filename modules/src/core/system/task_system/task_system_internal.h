#pragma once

#include "core/system/task_system.h"

namespace sv {

	Result task_initialize(u32 minThreadCount);
	Result task_close();

}