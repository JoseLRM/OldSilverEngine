#pragma once

#include "SilverEngine/window.h"
#include "graphics/graphics_internal.h"

namespace sv {

	Result window_initialize();
	void window_update();
	Result window_close();

	struct Window_internal {

		WindowState			state = WindowState_Invalid;
		WindowFlags			flags = WindowFlag_None;
		std::wstring		title;
		v4_u32				bounds;
		WindowHandle		handle = nullptr;
		bool				close_request = false;
		bool				resized = false;
		bool				style_modified;
		bool				flags_modified;
		WindowState			new_state;
		WindowFlags			new_flags;
		SwapChain_internal* swap_chain = nullptr;

	};

}