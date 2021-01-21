#pragma once

#include "core.h"

namespace sv {

	typedef void* WindowHandle;
	SV_DEFINE_HANDLE(Window);

	enum WindowState : u32 {
		
		WindowState_Invalid,
		WindowState_Windowed,
		WindowState_Minimized,
		WindowState_Maximized,
		WindowState_Fullscreen,

	};

	enum WindowFlag : u32 {

		WindowFlag_None			= 0u,
		WindowFlag_NoTitle		= SV_BIT(0),
		WindowFlag_NoBorder		= SV_BIT(1),
		WindowFlag_Resizable	= SV_BIT(2),
		WindowFlag_Maximizable	= SV_BIT(3),
		WindowFlag_Minimizable	= SV_BIT(4),

		WindowFlag_Default = WindowFlag_Resizable | WindowFlag_Maximizable | WindowFlag_Minimizable

	};
	typedef u32 WindowFlags;

	struct WindowDesc {

		const wchar* title;
		WindowState state;
		WindowFlags flags;
		v4_u32 bounds;
		const wchar* iconFilePath;

	};

	Result window_create(const WindowDesc* desc, Window** pWindow);
	Result window_destroy(Window* window);

	bool window_update(Window* window); // Return false if is closed

	Result	window_icon_set(Window* window, const wchar* iconFilePath);
	void	window_title_set(Window* window, const wchar* title);
	void	window_state_set(Window* window, WindowState state);
	void	window_flags_set(Window* window, WindowFlags flags);
	void	window_position_set(Window* window, const v2_u32& position);
	void	window_size_set(Window* window, const v2_u32& size);
	void	window_bounds_set(Window* window, const v4_u32& bounds);

	bool window_has_valid_surface(const Window* window); // Return true if has a valid surface to draw

	WindowHandle	window_handle(const Window* window);
	WindowState		window_state_get(const Window* window);
	WindowFlags		window_flags_get(const Window* window);
	
	u32				window_x_get(const Window* window);
	u32				window_y_get(const Window* window);
	v2_u32			window_position_get(const Window* window);
	u32				window_width_get(const Window* window);
	u32				window_height_get(const Window* window);
	v2_u32			window_size_get(const Window* window);
	f32				window_aspect_get(const Window* window);

	const wchar*	window_title_get(const Window* window);

	v2_u32 desktop_size();

}