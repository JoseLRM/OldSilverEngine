#pragma once

#include "core.h"

#include "windows/platform_windows.h"

struct SV_WINDOW_INITIALIZATION_DESC
{
	ui32 width, height, x, y;
	const wchar* title;
	bool fullscreen;
};

namespace _sv {

	bool window_initialize(const SV_WINDOW_INITIALIZATION_DESC& desc);
	bool window_close();
	void window_update();

	void window_set_position(ui32 x, ui32 y);
	void window_set_size(ui32 width, ui32 height);
	void window_notify_resized();

}

namespace sv {

	WindowHandle window_get_handle() noexcept;
	ui32 window_get_x() noexcept;
	ui32 window_get_y() noexcept;
	ui32 window_get_width() noexcept;
	ui32 window_get_height() noexcept;
	float window_get_aspect() noexcept;
	bool window_is_resized() noexcept;

	bool window_fullscreen_get();
	void window_fullscreen_set(bool fullscreen);

}