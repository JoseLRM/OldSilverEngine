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
	bool g_WindowRegistred = false;

	LRESULT WindowProc(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// Find SV::Window instance
		SV::Window* windowPtr = nullptr;
		{
			WindowHandle handle = ToSVWH(wnd);
			auto it = g_WindowsMap.find(handle);
			if (it != g_WindowsMap.end()) windowPtr = it->second;
		}

		if (windowPtr) {
			SV::Window& window = *windowPtr;

			switch (message) {
			case WM_CLOSE:
				PostQuitMessage(0);
				return 0;

			}

			window.GetWindowHandle();
		}

		return DefWindowProcA(wnd, message, wParam, lParam);
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

	bool RegisterWindowClass()
	{
		if (g_WindowRegistred) return true;

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
		g_WindowRegistred = true;
		return true;
	}

	bool Window::CreateWindowInstance(SV::Window* window, const SV_WINDOW_INITIALIZATION_DESC* desc)
	{
		RegisterWindowClass();

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
		ShowWindow(ToHWND(m_WindowHandle), SW_SHOW);

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
		MSG msg;
		while (PeekMessageA(&msg, ToHWND(m_WindowHandle), 0u, 0u, PM_REMOVE) > 0) {
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT) return false;
		}

		return true;
	}

}