#pragma once

#include "core.h"

#include "PlatformWindows.h"

struct SV_WINDOW_INITIALIZATION_DESC
{
	ui32 width, height, x, y;
	const wchar* title;
};

namespace SV {

	namespace Window {
		
		namespace _internal {
			bool Initialize(const SV_WINDOW_INITIALIZATION_DESC& desc);
			bool Close();
			void Update();

			void SetPositionValues(ui32 x, ui32 y);
			void SetDimensionValues(ui32 width, ui32 height);
			void NotifyResized();
		}


		WindowHandle GetWindowHandle() noexcept;

		ui32 GetX() noexcept;
		ui32 GetY() noexcept;
		ui32 GetWidth() noexcept;
		ui32 GetHeight() noexcept;

		float GetAspect() noexcept;

		bool IsResized() noexcept;

	}
}