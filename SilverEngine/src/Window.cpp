#include "core.h"

#include "Window.h"
#include "Win.h"

inline HWND ToHWND(SV::WindowHandle wnd)
{
	return reinterpret_cast<HWND>(wnd);
}
inline SV::WindowHandle ToSVWH(HWND wnd)
{
	return reinterpret_cast<SV::WindowHandle>(wnd);
}

namespace SV {

	//////////////////////////////////////GLOBALS//////////////////////////////////////
	std::map<WindowHandle, Window*> g_WindowsMap;

	LRESULT WindowProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// Find SV::Window instance
		SV::Window* windowPtr = nullptr;
		{
			WindowHandle handle = ToSVWH(wnd);
			auto it = g_WindowsMap.find(handle);
			if (it != g_WindowsMap.end()) windowPtr = it->second;
		}
		if (windowPtr) return Window::WindowProc(*windowPtr, message, wParam, lParam);
		else return DefWindowProcA(wnd, message, wParam, lParam);
	}

	i64 Window::WindowProc(Window& window, ui32 message, i64 wParam, i64 lParam)
	{
		return window.WindowProc(message, wParam, lParam);
	}

	i64 Window::WindowProc(ui32 message, i64 wParam, i64 lParam)
	{
		SV::Input& input = GetEngine().GetInput();

		switch (message) {
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			// input
			ui8 keyCode = (ui8)wParam;
			if (keyCode > 255) {
				SV::LogW("Unknown keycode: %u", keyCode);
			}
			else if (~lParam & (1 << 30)) input.KeyDown(keyCode);

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
			else input.KeyUp(keyCode);

			break;
		}
		case WM_LBUTTONDOWN:
			input.MouseDown(0);
			break;
		case WM_RBUTTONDOWN:
			input.MouseDown(1);
			break;
		case WM_MBUTTONDOWN:
			input.MouseDown(2);
			break;
		case WM_LBUTTONUP:
			input.MouseUp(0);
			break;
		case WM_RBUTTONUP:
			input.MouseUp(1);
			break;
		case WM_MBUTTONUP:
			input.MouseUp(2);
			break;
		case WM_MOUSEMOVE:
		{
			ui16 _x = LOWORD(lParam);
			ui16 _y = HIWORD(lParam);

			float x = (float(_x) / m_Width) - 0.5f;
			float y = (float(_y) / m_Height) - 0.5f;

			input.MousePos(x, y);
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
			m_Width = LOWORD(lParam);
			m_Height = HIWORD(lParam);
			m_Resized = true;

			break;
		}
		case WM_MOVE:
		{
			m_X = LOWORD(lParam);
			m_Y = HIWORD(lParam);
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

		return DefWindowProcA(ToHWND(m_WindowHandle), message, wParam, lParam);
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
	namespace _internal {
		bool RegisterWindowClass()
		{
			WNDCLASSA wndClass;
			wndClass.cbClsExtra = 0;
			wndClass.cbWndExtra = 0;
			wndClass.hbrBackground = 0;
			wndClass.hCursor = 0;
			wndClass.hIcon = 0;
			wndClass.hInstance = 0;
			wndClass.lpfnWndProc = WindowProc;
			wndClass.lpszClassName = "SilverWindow";
			wndClass.lpszMenuName = "SilverWindow";
			wndClass.style = 0;

			RegisterClassA(&wndClass);
			return true;
		}
	}

	bool Window::CreateWindowInstance(SV::Window* window, const SV_WINDOW_INITIALIZATION_DESC* desc)
	{
		DWORD style = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX;

		int w = desc->width, h = desc->height;
		AdjustWindow(desc->x, desc->y, w, h, style);

		window->m_WindowHandle = CreateWindowExA(0u, "SilverWindow", desc->title, style, desc->x, desc->y, w, h, 0, 0, 0, 0);

		if (window->m_WindowHandle == 0) {
			SV::LogE("Error creating Window class");
			return false;
		}

		g_WindowsMap[window->m_WindowHandle] = window;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Window::Window() : m_WindowHandle(nullptr)
	{}
	Window::~Window()
	{}

	bool Window::Initialize(const SV_WINDOW_INITIALIZATION_DESC* desc)
	{
		CreateWindowInstance(this, desc);

		// Show Window
		ShowWindow(ToHWND(m_WindowHandle), SW_SHOWDEFAULT);

		// Console
		if (desc->showConsole) ShowWindow(GetConsoleWindow(), SW_SHOWDEFAULT);
		else ShowWindow(GetConsoleWindow(), SW_HIDE);

		return true;
	}
	bool Window::Close()
	{
		if (m_WindowHandle) {
			CloseWindow(ToHWND(m_WindowHandle));
			m_WindowHandle = 0;
		}
		return true;
	}

	bool Window::UpdateInput()
	{
		GetEngine().GetInput().Update();

		MSG msg;
		while (PeekMessageA(&msg, ToHWND(m_WindowHandle), 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT) return false;
		}

		return true;
	}

}