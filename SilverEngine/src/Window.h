#pragma once

#include "core.h"

struct SV_WINDOW_INITIALIZATION_DESC
{
	ui32 width, height, x, y;
	const char* title;
};

namespace SV {

	typedef void* WindowHandle;

	class Engine;
	class Window {
		WindowHandle m_WindowHandle;

	private:
		Window();
		~Window();

		static bool CreateWindowInstance(SV::Window* window, const SV_WINDOW_INITIALIZATION_DESC* desc);

		bool Initialize(const SV_WINDOW_INITIALIZATION_DESC* desc);
		bool UpdateInput();
		bool Close();

	public:
		friend Engine;

		inline WindowHandle GetWindowHandle() const noexcept
		{
			return m_WindowHandle;
		}

	};

}