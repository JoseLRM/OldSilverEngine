#pragma once

#include "window.h"

namespace sv {

	Result window_initialize(const sv::InitializationWindowDesc& desc);
	Result window_close();
	void window_update();

	void window_set_position(ui32 x, ui32 y);
	void window_set_size(ui32 width, ui32 height);
	void window_notify_resized();

	sv::uvec4 window_get_last_bounds();

}