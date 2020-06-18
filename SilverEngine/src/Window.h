#pragma once

#include "core.h"

namespace SV {
	typedef void* WindowHandle;
	typedef std::function<ui16(WindowHandle, ui32, ui64, ui64)> WindowProcFn;
}

struct SV_WINDOW_INITIALIZATION_DESC
{
	ui32 width, height, x, y;
	const wchar* title;
	SV::WindowHandle parent;
	SV::WindowProcFn userWindowProc;
};

namespace SV {

	namespace _internal {
		bool RegisterWindowClass();
	}

	class Engine;
	class Window : public SV::EngineDevice {
		static std::mutex s_WindowCreationMutex;

		WindowHandle m_WindowHandle;
		WindowProcFn m_UserWindowProc;

		float m_X, m_Y, m_Width, m_Height;
		bool m_Resized = false;

	private:
		Window();
		~Window();

		static bool CreateWindowInstance(SV::Window* window, const SV_WINDOW_INITIALIZATION_DESC& desc);
		static bool DestroyWindowInstance(SV::Window* window);

		bool Initialize(const SV_WINDOW_INITIALIZATION_DESC& desc);
		bool UpdateInput();
		bool Close();

		i64 WindowProc(ui32 message, i64 wParam, i64 lParam);
		
	public:
		friend Engine;

		static i64 WindowProc(Window& window, ui32 message, i64 wParam, i64 lParam);

		inline WindowHandle GetWindowHandle() const noexcept { return m_WindowHandle; }

		inline ui32 GetWidth() const noexcept { return m_Width; }
		inline ui32 GetHeight() const noexcept { return m_Height; }
		inline float GetAspect() const noexcept { return float(m_Width) / float(m_Height); }

		inline bool IsResized() const noexcept { return m_Resized; }
	};

	void ShowConsole();
	void HideConsole();

}