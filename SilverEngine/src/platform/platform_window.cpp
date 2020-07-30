#include "core.h"

#ifdef SV_PLATFORM_WINDOWS

#include "platform_window.h"
#include "Window.h"
#include "Engine.h"

#include "windows_impl.h"
#undef CreateWindow
#undef CallWindowProc

namespace sv {

	inline HWND ToHWND(WindowHandle wnd)
	{
		return reinterpret_cast<HWND>(wnd);
	}
	inline WindowHandle ToSVWH(HWND wnd)
	{
		return reinterpret_cast<WindowHandle>(wnd);
	}

	LRESULT CALLBACK WindowProcFn(HWND hWnd, ui32 msg, WPARAM wParam, LPARAM lParam)
	{
		// Get Window (in userData)
		Window_wnd* window_ptr = (Window_wnd*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (window_ptr == nullptr) return DefWindowProc(hWnd, msg, wParam, lParam);
		Window_wnd& window = *window_ptr;

		// Call User Functions
		window.CallWindowProc(msg, wParam, lParam);

		switch (msg) {
			case WM_DESTROY:
			case WM_CLOSE:
			case WM_QUIT:
				sv::engine_request_close();
				return 0;
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				// input
				ui8 keyCode = (ui8)wParam;
				if (keyCode > 255) {
					sv::log_warning("Unknown keycode: %u", keyCode);
				}
				else if (~lParam & (1 << 30)) _sv::input_key_pressed_add(keyCode);

				break;
			}
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				// input
				ui8 keyCode = (ui8)wParam;
				if (keyCode > 255) {
					sv::log_warning("Unknown keycode: %u", keyCode);
					VK_SHIFT;
				}
				else _sv::input_key_released_add(keyCode);

				break;
			}
			case WM_LBUTTONDOWN:
				_sv::input_mouse_pressed_add(0);
				break;
			case WM_RBUTTONDOWN:
				_sv::input_mouse_pressed_add(1);
				break;
			case WM_MBUTTONDOWN:
				_sv::input_mouse_pressed_add(2);
				break;
			case WM_LBUTTONUP:
				_sv::input_mouse_released_add(0);
				break;
			case WM_RBUTTONUP:
				_sv::input_mouse_released_add(1);
				break;
			case WM_MBUTTONUP:
				_sv::input_mouse_released_add(2);
				break;
			case WM_MOUSEMOVE:
			{
				ui16 _x = LOWORD(lParam);
				ui16 _y = HIWORD(lParam);

				float w = window_get_width();
				float h = window_get_height();

				float x = (float(_x) / w) - 0.5f;
				float y = (1.f - (float(_y) / h)) - 0.5f;

				_sv::input_mouse_position_set(x, y);
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
				_sv::window_set_size(LOWORD(lParam), HIWORD(lParam));

				//switch (wParam)
				//{
				//case SIZE_MINIMIZED:
				//	m_Minimized = true;
				//	break;
				//default:
				//	m_Minimized = false;
				//	break;
				//}

				_sv::window_notify_resized();

				break;
			}
			case WM_MOVE:
			{
				_sv::window_set_position(LOWORD(lParam), HIWORD(lParam));
				
				//m_Minimized = false;
				//jsh::WindowMovedEvent e(screenX, screenY);
				//jshEvent::Dispatch(e);
				break;
			}
			case WM_SETFOCUS:
			{
				//jsh::WindowGainFocusEvent e;
				//jshEvent::Dispatch(e);
				break;
			}
			case WM_KILLFOCUS:
			{
				//jsh::WindowLostFocusEvent e;
				//jshEvent::Dispatch(e);
				break;
			}

		}

		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	void AdjustWindow(int x, int y, int& w, int& h, DWORD style)
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

	std::mutex g_RegisterWindowClassMutex;
	bool RegisterWindowClass()
	{
		static bool registred = false;
		std::lock_guard<std::mutex> lock(g_RegisterWindowClassMutex);

		if (registred) return true;

		WNDCLASSW wndClass;
		wndClass.cbClsExtra = 0;
		wndClass.cbWndExtra = 0;
		wndClass.hbrBackground = 0;
		wndClass.hCursor = LoadCursorW(0, IDC_ARROW);
		wndClass.hIcon = 0;
		wndClass.hInstance = 0;
		wndClass.lpfnWndProc = WindowProcFn;
		wndClass.lpszClassName = L"SilverWindow";
		wndClass.lpszMenuName = 0;
		wndClass.style = 0;

		RegisterClassW(&wndClass);

		registred = false;
		return true;
	}

	void Window_wnd::CallWindowProc(ui32& msg, ui64& wParam, i64& lParam)
	{
		for (ui32 i = 0; i < m_UserWindowProcs.size(); ++i) {
			m_UserWindowProcs[i](msg, wParam, lParam);
		}
	}

	void Window_wnd::AddWindowProc(WindowProc wp)
	{
		m_UserWindowProcs.push_back(wp);
	}
	void Window_wnd::PeekMessages()
	{
		MSG msg;
	
		while (PeekMessageW(&msg, 0, 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	WindowHandle Window_wnd::CreateWindowWindows(const wchar* title, ui32 x, ui32 y, ui32 width, ui32 height)
	{
		if (!RegisterWindowClass()) {
			sv::log_error("Can't Register Window Class");
			return 0;
		}

		DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX | WS_VISIBLE;

		int w = width, h = height;
		AdjustWindow(x, y, w, h, style);

		HWND hWnd = CreateWindowExW(0u, L"SilverWindow", title, style, x, y, w, h, 0, 0, 0, 0);

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);

		return hWnd;
	}

	BOOL Window_wnd::CloseWindowWindows(WindowHandle hWnd)
	{
		return CloseWindow(ToHWND(hWnd));
	}

}

#endif