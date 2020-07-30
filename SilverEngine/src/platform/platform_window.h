#pragma once

#include "core.h"

#ifdef SV_PLATFORM_WINDOWS

namespace sv {

	typedef void(*WindowProc)(ui32&, ui64&, i64&);

	class Window_wnd {
		std::vector<WindowProc> m_UserWindowProcs;

	public:
		void CallWindowProc(ui32& msg, ui64& wParam, i64& lParam);
		void AddWindowProc(WindowProc);

		void PeekMessages();

		WindowHandle CreateWindowWindows(const wchar* title, ui32 xPos, ui32 yPos, ui32 width, ui32 height);
		BOOL CloseWindowWindows(WindowHandle hWnd);
		
	};

}

#endif