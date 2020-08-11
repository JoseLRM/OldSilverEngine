#pragma once

#include "core.h"

#ifdef SV_PLATFORM_WINDOWS
#include "platform/windows/platform_windows.h"
#else
#error "Platform not supported"
#endif

namespace sv {

	struct InitializationWindowDesc
	{
		ui32 width, height, x, y;
		const wchar* title;
		bool fullscreen;
	};

	WindowHandle window_get_handle() noexcept;
	ui32 window_get_x() noexcept;
	ui32 window_get_y() noexcept;
	ui32 window_get_width() noexcept;
	ui32 window_get_height() noexcept;
	float window_get_aspect() noexcept;
	Window_wnd& window_get_platform() noexcept;
	bool window_is_resized() noexcept;

	bool window_fullscreen_get();
	void window_fullscreen_set(bool fullscreen);

}