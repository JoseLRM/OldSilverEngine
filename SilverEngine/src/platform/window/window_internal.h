#pragma once

#include "platform/window.h"

namespace sv {

	Result window_initialize(WindowStyle style, const vec4u& bounds, const wchar* title, const wchar* iconFilePath);
	Result window_close();
	void window_update();

}