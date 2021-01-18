#include "SilverEngine/core.h"

#include "SilverEngine/platform/impl.h"
#include "window_internal.h"
#include "input/input_internal.h"
#include "graphics/graphics_internal.h"

#ifndef SV_PLATFORM_WIN
#	error Platform not supported
#endif

namespace sv {

	// GLOBAL FUNCTIONS

#ifdef SV_PLATFORM_WIN
	LRESULT CALLBACK WindowProcFn(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam);
#endif

	Result window_initialize()
	{
#ifdef SV_PLATFORM_WIN
		
		// Define Window Class
		{
			WNDCLASSW c{};
			c.hCursor = LoadCursorW(0, IDC_ARROW);
			c.lpfnWndProc = WindowProcFn;
			c.lpszClassName = L"SilverWindow";

			if (!RegisterClassW(&c)) {
				SV_LOG_ERROR("Can't register window class");
				return Result_PlatformError;
			}
		}

		
#endif

		return Result_Success;
	}

	void window_update()
	{
		MSG msg;

		while (PeekMessageW(&msg, 0, 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	Result window_close()
	{
		UnregisterClassW(L"SilverWindow", 0);
		return Result_Success;
	}

#ifdef SV_PLATFORM_WIN

	LRESULT CALLBACK WindowProcFn(HWND hWnd, u32 msg, WPARAM wParam, LPARAM lParam)
	{
		Window_internal* wndPtr = nullptr;
		{
			wndPtr = (Window_internal*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
		}
		if (wndPtr == nullptr) return DefWindowProcW(hWnd, msg, wParam, lParam);

		Window_internal& wnd = *wndPtr;

		switch (msg) {
		case WM_DESTROY:
		case WM_CLOSE:
		case WM_QUIT:
			wnd.close_request = true;
			return 0;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			// input
			u8 keyCode = (u8)wParam;
			if (keyCode > 255) {
				SV_LOG_WARNING("Unknown keycode: %u", keyCode);
			}
			else if (~lParam & (1 << 30)) input_key_pressed_add(keyCode);

			break;
		}
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			// input
			u8 keyCode = (u8)wParam;
			if (keyCode > 255) {
				SV_LOG_WARNING("Unknown keycode: %u", keyCode);
			}
			else input_key_released_add(keyCode);

			break;
		}
		case WM_LBUTTONDOWN:
			input_mouse_pressed_add(0);
			break;
		case WM_RBUTTONDOWN:
			input_mouse_pressed_add(1);
			break;
		case WM_MBUTTONDOWN:
			input_mouse_pressed_add(2);
			break;
		case WM_LBUTTONUP:
			input_mouse_released_add(0);
			break;
		case WM_RBUTTONUP:
			input_mouse_released_add(1);
			break;
		case WM_MBUTTONUP:
			input_mouse_released_add(2);
			break;
		case WM_MOUSEMOVE:
		{
			u16 _x = LOWORD(lParam);
			u16 _y = HIWORD(lParam);

			f32 w = f32(wnd.bounds.z);
			f32 h = f32(wnd.bounds.w);

			f32 x = (f32(_x) / w) - 0.5f;
			f32 y = -(f32(_y) / h) + 0.5f;

			input_mouse_position_set(x, y);
			break;
		}
		case WM_CHAR:
		{
			switch (wParam)
			{
			case 0x08:
				input_text_command_add(TextCommand_DeleteLeft);
				break;

			case 0x0A:

				// Process a linefeed. 

				break;

			case 0x1B:
				input_text_command_add(TextCommand_Escape);
				break;

			case 0x09:

				// TODO: Tabulations input
				input_text_add(' ');
				input_text_add(' ');
				input_text_add(' ');
				input_text_add(' ');

				break;

			case 0x0D:
				input_text_command_add(TextCommand_Enter);
				break;

			default:
				input_text_add(wParam);
				break;
			}

			break;
		}

		// RAW MOUSE
		//case WM_INPUT:
		//{
		//	UINT size;
		//	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == -1) break;
		//
		//	rawMouseBuffer.resize(size);
		//	if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawMouseBuffer.data(), &size, sizeof(RAWINPUTHEADER)) == -1) break;
		//
		//	RAWINPUT& rawInput = reinterpret_cast<RAWINPUT&>(*rawMouseBuffer.data());
		//	if (rawInput.header.dwType == RIM_TYPEMOUSE) {
		//		if (rawInput.data.mouse.lLastX != 0 || rawInput.data.mouse.lLastY != 0) {
		//			jshInput::MouseDragged(rawInput.data.mouse.lLastX, rawInput.data.mouse.lLastY);
		//		}
		//	}
		//}
		//break;
		case WM_SIZE:
		{
			wnd.bounds.z = LOWORD(lParam);
			wnd.bounds.w = HIWORD(lParam);

			switch (wParam)
			{
			case SIZE_MAXIMIZED:
				wnd.state = WindowState_Maximized;
				break;
			case SIZE_MINIMIZED:
				wnd.state = WindowState_Minimized;
				break;
			default:
				wnd.state = WindowState_Windowed;
				break;
			}

			wnd.resized = true;

			break;
		}
		case WM_MOVE:
		{
			wnd.bounds.x = LOWORD(lParam);
			wnd.bounds.y = HIWORD(lParam);

			//m_Minimized = false;
			//jsh::WindowMovedEvent e(screenX, screenY);
			//jshEvent::Dispatch(e);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			float wheel = float(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
			input_mouse_wheel_set(wheel);
			break;
		}
		case WM_SETFOCUS:
		{
			input_focussed_window_set((Window*)&wnd);
			break;
		}
		case WM_KILLFOCUS:
		{
			input_focussed_window_set(nullptr);
			break;
		}

		}

		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	DWORD parse_style(WindowState state, WindowFlags flags)
	{
		DWORD style = WS_VISIBLE;

		switch (state)
		{
		case sv::WindowState_Maximized:
			style |= WS_MAXIMIZE;

		case sv::WindowState_Windowed:
			
			if (~flags & WindowFlag_NoTitle) style |= WS_CAPTION | WS_SYSMENU | WS_OVERLAPPED;
			if (~flags & WindowFlag_NoBorder) style |= WS_BORDER;
			if (flags & WindowFlag_Maximizable) style |= WS_MINIMIZEBOX;
			if (flags & WindowFlag_Minimizable) style |= WS_MAXIMIZEBOX;
			if (flags & WindowFlag_Resizable) style |= WS_SIZEBOX;
			break;

		case sv::WindowState_Minimized:
			style = 0u;
			break;

		case sv::WindowState_Fullscreen:
			style |= WS_POPUP;
			break;

		default:
			style = 0;
			break;

		}

		return style;
	}

	void window_adjust(u32 x, u32 y, u32& w, u32& h, DWORD style)
	{
		RECT rect;
		rect.left = x;
		rect.right = x + w;
		rect.top = y;
		rect.bottom = y + h;

		AdjustWindowRect(&rect, style, false);
		w = rect.right - rect.left;
		h = rect.bottom - rect.top;
	}

	v4_u32 adjusted_bounds(const v4_u32& bounds, WindowState state, WindowFlags flags, DWORD style)
	{
		v4_u32 b;
		if (state == WindowState_Fullscreen) {
			v2_u32 res = desktop_size();
			b = { 0u, 0u, res.x, res.y };
		}
		else {
			b = { bounds.x, bounds.y, bounds.z, bounds.w };
			window_adjust(b.x, b.y, b.z, b.w, style);
		}
		return b;
	}

#endif

	// CLASS METHODS

	Result window_create(const WindowDesc* desc, Window** pWindow)
	{
#ifdef SV_PLATFORM_WIN
		// Set style
		DWORD style = parse_style(desc->state, desc->flags);

		u32 w = desc->bounds.z, h = desc->bounds.w;
		window_adjust(desc->bounds.x, desc->bounds.y, w, h, style);

		Window_internal& wnd = *new Window_internal();
		wnd.bounds = desc->bounds;
		wnd.title = desc->title ? desc->title : L"SilverEngine";

		wnd.handle = CreateWindowExW(0u, L"SilverWindow", wnd.title.c_str(), style, i32(desc->bounds.x), i32(desc->bounds.y), i32(w), i32(h), 0, 0, 0, 0);

		if (wnd.handle == 0) {
			SV_LOG_ERROR("Error creating Window class");
			return Result_PlatformError;
		}

		*pWindow = reinterpret_cast<Window*>(&wnd);

		SetWindowLongPtrW((HWND)wnd.handle, GWLP_USERDATA, (LONG_PTR)&wnd);
		if (GetFocus() == wnd.handle) input_focussed_window_set(*pWindow);

		// Set Icon
		if (desc->iconFilePath != nullptr) {
			svCheck(window_icon_set(*pWindow, desc->iconFilePath));
		}

		window_flags_set(*pWindow, desc->flags);
		window_state_set(*pWindow, desc->state);

		// Create SwapChain
		svCheck(graphics_swapchain_create(*pWindow, &wnd.swap_chain));

		SV_LOG_INFO("Window created");
#endif

		return Result_Success;
	}

	Result window_destroy(Window* window)
	{
		if (window == nullptr) return Result_Success;

		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		svCheck(graphics_destroy(reinterpret_cast<Primitive*>(wnd.swap_chain)));
		Result res = DestroyWindow((HWND(wnd.handle))) ? Result_Success : Result_PlatformError;
		delete& wnd;
		return res;
	}

	bool window_update(Window* window)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);

		if (wnd.style_modified || wnd.flags_modified) {
			graphics_gpu_wait();

			WindowState state = wnd.state;
			WindowFlags flags = wnd.flags;

			if (wnd.style_modified) state = wnd.new_state;
			if (wnd.flags_modified) flags = wnd.new_flags;

#ifdef SV_PLATFORM_WIN
			// Set Windows style
			DWORD style = parse_style(state, flags);
			SetWindowLongPtrW((HWND)wnd.handle, GWL_STYLE, (LONG_PTR)style);

			v4_u32 bounds = adjusted_bounds(wnd.bounds, state, flags, style);
			SetWindowPos((HWND)wnd.handle, 0u, bounds.x, bounds.y, bounds.z, bounds.w, 0);
#endif

			wnd.state = state;
			wnd.flags = flags;
			wnd.flags_modified = false;
			wnd.style_modified = false;
		}

		if (wnd.close_request) {

			window_destroy(window);
			return false;
		}

		if (wnd.resized && window_has_valid_surface(window)) {
			graphics_swapchain_resize(wnd.swap_chain);
		}

		wnd.resized = false;

		return true;
	}

	Result window_icon_set(Window* window, const wchar* filePath)
	{
#ifdef SV_RES_PATH
		std::wstring filePathStr = SV_RES_PATH_W;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);

#ifdef SV_PLATFORM_WIN

		auto icon = LoadImageW(0, filePath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED | LR_LOADTRANSPARENT);
		if (icon == NULL) return Result_PlatformError;

		HWND hwnd = (HWND)wnd.handle;

		SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)icon);
		SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)icon);
#endif

		return Result_Success;
	}

	void window_title_set(Window* window, const wchar* title)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		SetWindowTextW((HWND)wnd.handle, title);
	}

	void window_state_set(Window* window, WindowState state)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		wnd.new_state = state;
		wnd.style_modified = true;
	}

	void window_flags_set(Window* window, WindowFlags flags)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		wnd.new_flags = flags;
		wnd.flags_modified = true;
	}

	void window_position_set(Window* window, const v2_u32& position)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		window_bounds_set(window, v4_u32{ position.x, position.y, wnd.bounds.z, wnd.bounds.w });
	}

	void window_size_set(Window* window, const v2_u32& size)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		window_bounds_set(window, v4_u32{ wnd.bounds.x, wnd.bounds.y, size.x, size.y });
	}

	void window_bounds_set(Window* window, const v4_u32& bounds)
	{
		Window_internal& wnd = *reinterpret_cast<Window_internal*>(window);
		SetWindowPos((HWND)wnd.handle, 0, bounds.x, bounds.y, bounds.z, bounds.w, 0);
	}

	bool window_has_valid_surface(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.state != WindowState_Invalid && wnd.state != WindowState_Minimized && wnd.bounds.z != 0u && wnd.bounds.w != 0u;
	}

	WindowHandle window_handle(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.handle;
	}

	WindowState window_state_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.state;
	}
	
	WindowFlags window_flags_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.flags;
	}

	u32 window_x_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.bounds.x;
	}

	u32 window_y_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.bounds.y;
	}

	v2_u32 window_position_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return { wnd.bounds.x, wnd.bounds.y };
	}

	u32 window_width_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.bounds.z;
	}

	u32 window_height_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.bounds.w;
	}

	v2_u32 window_size_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return { wnd.bounds.z, wnd.bounds.w };
	}

	const wchar* window_title_get(const Window* window)
	{
		const Window_internal& wnd = *reinterpret_cast<const Window_internal*>(window);
		return wnd.title.c_str();
	}

	v2_u32 desktop_size()
	{
#ifdef SV_PLATFORM_WIN
		HWND desktop = GetDesktopWindow();
		RECT rect;
		GetWindowRect(desktop, &rect);
		return { u32(rect.right), u32(rect.bottom) };
#endif
	}

}