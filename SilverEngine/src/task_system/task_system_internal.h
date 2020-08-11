#pragma once

#include "task_system.h"

namespace sv {

	bool task_initialize(ui32 minThreadCount);
	bool task_close();

}