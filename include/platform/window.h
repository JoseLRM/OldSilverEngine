#pragma once

#include "core.h"

namespace sv {

	enum WindowStyle {
		WindowStyle_None,
		WindowStyle_Fullscreen = SV_BIT(0),
		WindowStyle_NoTitle = SV_BIT(1),
		WindowStyle_NoBorder = SV_BIT(2),
		WindowStyle_Maximizable = SV_BIT(3),
		WindowStyle_Minimizable = SV_BIT(4),
		WindowStyle_Resizable = SV_BIT(5),

		WindowStyle_NoDecoration = WindowStyle_NoTitle | WindowStyle_NoBorder,
		WindowStyle_Default = WindowStyle_Maximizable | WindowStyle_Minimizable | WindowStyle_Resizable,
	};
	typedef u32 WindowStyleFlags;

	WindowHandle window_handle_get() noexcept;

	vec4u	window_bounds_get();
	vec2u	window_position_get();
	vec2u	window_size_get();
	float	window_aspect_get();
	void	window_bounds_set(const vec4u& bounds);
	void	window_position_set(const vec2u& position);
	void	window_size_set(const vec2u& size);
	void	window_aspect_set(float aspect);

	Result			window_title_set(const wchar* title);
	const wchar*	window_title_get();

	Result			window_icon_set(const wchar* filePath);
	const wchar*	window_icon_get_filepath();

	WindowStyleFlags	window_style_get();
	void				window_style_set(WindowStyleFlags style);
	
	vec2u window_desktop_size();

#ifdef SV_PLATFORM_WIN
	typedef u64(*WindowProc)(WindowHandle, u32, u64, i64);

	void window_userproc_set(WindowProc userProc);
#endif

}