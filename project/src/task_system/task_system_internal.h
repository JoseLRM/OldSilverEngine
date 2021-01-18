#pragma once

#include "SilverEngine/task_system.h"

namespace sv {

	Result task_initialize(u32 minThreadCount);
	Result task_close();

}