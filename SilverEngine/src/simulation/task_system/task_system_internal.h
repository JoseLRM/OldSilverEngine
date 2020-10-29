#pragma once

#include "simulation/task_system.h"

namespace sv {

	Result task_initialize(ui32 minThreadCount);
	Result task_close();

}