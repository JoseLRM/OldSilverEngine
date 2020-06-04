#pragma once

#include "core.h"

struct SV_WINDOW_INITIALIZATION_DESC
{
	ui32 width, height, x, y;
	bool showConsole;
	const char* title;
};

namespace SV {

	namespace _internal {
		bool RegisterWindowClass();
	}

	typedef void* WindowHandle;

	class Engine;
	class Window : public EngineDevice {
		WindowHandle m_WindowHandle;

		float m_X, m_Y, m_Width, m_Height;
		bool m_Resized = false;

	private:
		Window();
		~Window();

		static bool CreateWindowInstance(SV::Window* window, const SV_WINDOW_INITIALIZATION_DESC* desc);

		bool Initialize(const SV_WINDOW_INITIALIZATION_DESC* desc);
		bool UpdateInput();
		bool Close();

		i64 WindowProc(ui32 message, i64 wParam, i64 lParam);
		
	public:
		friend Engine;

		static i64 WindowProc(Window& window, ui32 message, i64 wParam, i64 lParam);

		inline WindowHandle GetWindowHandle() const noexcept
		{
			return m_WindowHandle;
		}

	};

}