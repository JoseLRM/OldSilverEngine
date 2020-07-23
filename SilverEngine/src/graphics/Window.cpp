#include "core.h"

#include "..//Engine.h"

#include "Window.h"

namespace SV {
	namespace Window {

		WindowHandle g_WindowHandle;

		float g_X, g_Y, g_Width, g_Height;
		bool g_Resized = false;

#ifdef SV_PLATFORM_WINDOWS
		Window_wnd platform;
#elif SV_PLATFORM_LINUX
		Window_lnx platform;
#endif

		namespace _internal {
			bool Initialize(const SV_WINDOW_INITIALIZATION_DESC& desc)
			{
				// Create Window
				g_X = desc.x;
				g_Y = desc.y;
				g_Width = desc.width;
				g_Height = desc.height;

				g_WindowHandle = platform.CreateWindowWindows(desc.title, g_X, g_Y, g_Width, g_Height);

				if (g_WindowHandle == 0) {
					SV::LogE("Error creating Window class");
					return false;
				}

				return true;
			}
			bool Close()
			{
				return platform.CloseWindowWindows(g_WindowHandle);
			}

			void Update()
			{
				g_Resized = false;
				platform.PeekMessages();
				if (g_Resized) {
					Graphics::ResizeSwapChain();
				}
			}

			void SetDimensionValues(ui32 width, ui32 height)
			{
				g_Width = width;
				g_Height = height;
			}
			void SetPositionValues(ui32 x, ui32 y)
			{
				g_X = x;
				g_Y = y;
			}
			void NotifyResized()
			{
				g_Resized = true;
			}
		}

		WindowHandle GetWindowHandle() noexcept { return g_WindowHandle; }

		ui32 GetX() noexcept { return g_X; }
		ui32 GetY() noexcept { return g_Y; }
		ui32 GetWidth() noexcept { return g_Width; }
		ui32 GetHeight() noexcept { return g_Height; }

		float GetAspect() noexcept { return float(g_Width) / float(g_Height); }

		bool IsResized() noexcept { return g_Resized; }
	}
}