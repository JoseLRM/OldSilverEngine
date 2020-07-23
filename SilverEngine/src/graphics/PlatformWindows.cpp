#include "core.h"

#ifdef SV_PLATFORM_WINDOWS

#include "PlatformWindows.h"
#include "Window.h"
#include "Engine.h"

#include "Win.h"
#undef CreateWindow
#undef CallWindowProc

namespace SV {

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
		SV::Window_wnd* window_ptr = (SV::Window_wnd*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
		if (window_ptr == nullptr) return DefWindowProc(hWnd, msg, wParam, lParam);
		SV::Window_wnd& window = *window_ptr;

		// Call User Functions
		window.CallWindowProc(msg, wParam, lParam);

		switch (msg) {
			case WM_DESTROY:
			case WM_CLOSE:
			case WM_QUIT:
				Engine::RequestClose();
				return 0;
			case WM_SYSKEYDOWN:
			case WM_KEYDOWN:
			{
				// input
				ui8 keyCode = (ui8)wParam;
				if (keyCode > 255) {
					SV::LogW("Unknown keycode: %u", keyCode);
				}
				else if (~lParam & (1 << 30)) Engine::_internal::AddKeyPressed(keyCode);

				break;
			}
			case WM_SYSKEYUP:
			case WM_KEYUP:
			{
				// input
				ui8 keyCode = (ui8)wParam;
				if (keyCode > 255) {
					SV::LogW("Unknown keycode: %u", keyCode);
					VK_SHIFT;
				}
				else Engine::_internal::AddKeyReleased(keyCode);

				break;
			}
			case WM_LBUTTONDOWN:
				Engine::_internal::AddMousePressed(0);
				break;
			case WM_RBUTTONDOWN:
				Engine::_internal::AddMousePressed(1);
				break;
			case WM_MBUTTONDOWN:
				Engine::_internal::AddMousePressed(2);
				break;
			case WM_LBUTTONUP:
				Engine::_internal::AddMouseReleased(0);
				break;
			case WM_RBUTTONUP:
				Engine::_internal::AddMouseReleased(1);
				break;
			case WM_MBUTTONUP:
				Engine::_internal::AddMouseReleased(2);
				break;
			case WM_MOUSEMOVE:
			{
				ui16 _x = LOWORD(lParam);
				ui16 _y = HIWORD(lParam);

				float w = Window::GetWidth();
				float h = Window::GetHeight();

				float x = (float(_x) / w) - 0.5f;
				float y = (1.f - (float(_y) / h)) - 0.5f;

				Engine::_internal::SetMousePos(x, y);
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
				Window::_internal::SetDimensionValues(LOWORD(lParam), HIWORD(lParam));

				//switch (wParam)
				//{
				//case SIZE_MINIMIZED:
				//	m_Minimized = true;
				//	break;
				//default:
				//	m_Minimized = false;
				//	break;
				//}

				Window::_internal::NotifyResized();

				break;
			}
			case WM_MOVE:
			{
				Window::_internal::SetPositionValues(LOWORD(lParam), HIWORD(lParam));
				
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
			SV::LogE("Can't Register Window Class");
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