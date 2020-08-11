#include "core.h"

#include "window_internal.h"
#include "graphics/graphics_internal.h"

namespace sv {

	static WindowHandle g_WindowHandle;

	static ui32 g_X, g_Y, g_Width, g_Height;
	static bool g_Resized = false;
	static bool g_Fullscreen;
	static bool g_ChangeFullscreen = false;

	static ui32 g_LastX, g_LastY, g_LastWidth, g_LastHeight;

#ifdef SV_PLATFORM_WINDOWS
	static Window_wnd platform;
#elif SV_PLATFORM_LINUX
	static Window_lnx platform;
#endif

	bool window_initialize(const InitializationWindowDesc& desc)
	{
		// Create Window
		g_X = desc.x;
		g_Y = desc.y;
		g_Width = desc.width;
		g_Height = desc.height;
		g_Fullscreen = desc.fullscreen;

#ifdef SV_PLATFORM_WINDOWS
		g_WindowHandle = platform.CreateWindowWindows(desc.title, g_X, g_Y, g_Width, g_Height, desc.fullscreen);
#endif

		if (g_WindowHandle == 0) {
			sv::log_error("Error creating Window class");
			return false;
		}

		return true;
	}
	bool window_close()
	{
		return platform.CloseWindowWindows(g_WindowHandle);
	}

	void window_update()
	{
		g_Resized = false;

		if (g_ChangeFullscreen) {
			graphics_gpu_wait();
			g_Fullscreen = !g_Fullscreen;

			if (g_Fullscreen) {
				g_LastX = g_X;
				g_LastY = g_Y;
				g_LastWidth = g_Width;
				g_LastHeight = g_Height;
			}

			platform.SetFullscreen(g_Fullscreen);
			g_ChangeFullscreen = false;
		}

		platform.PeekMessages();
		if (g_Resized) {
			sv::graphics_swapchain_resize();
		}
	}

	void window_set_position(ui32 x, ui32 y)
	{
		g_X = x;
		g_Y = y;
	}
	void window_set_size(ui32 width, ui32 height)
	{
		g_Width = width;
		g_Height = height;
	}
	void window_notify_resized()
	{
		g_Resized = true;
	}

	sv::uvec4 window_get_last_bounds()
	{
		return { g_LastX, g_LastY, g_LastWidth, g_LastHeight };
	}

	WindowHandle window_get_handle() noexcept { return g_WindowHandle; }
	ui32 window_get_x() noexcept { return g_X; }
	ui32 window_get_y() noexcept { return g_Y; }
	ui32 window_get_width() noexcept { return g_Width; }
	ui32 window_get_height() noexcept { return g_Height; }
	float window_get_aspect() noexcept { return float(g_Width) / float(g_Height); }
	Window_wnd& window_get_platform() noexcept { return platform; }
	bool window_is_resized() noexcept { return g_Resized; }

	bool window_fullscreen_get()
	{
		return g_Fullscreen;
	}

	void window_fullscreen_set(bool fullscreen)
	{
		g_ChangeFullscreen = g_Fullscreen != fullscreen;
	}
	
}