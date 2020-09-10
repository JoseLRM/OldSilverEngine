#pragma once

#include "window.h"

namespace sv {

	Result window_initialize(const sv::InitializationWindowDesc& desc);
	Result window_close();
	void window_update();

}