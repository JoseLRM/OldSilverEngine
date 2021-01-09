#pragma once

#include "core/window.h"

namespace sv {

	Result window_initialize(WindowStyle style, const v4_u32& bounds, const wchar* title, const wchar* iconFilePath);
	Result window_close();
	void window_update();

}