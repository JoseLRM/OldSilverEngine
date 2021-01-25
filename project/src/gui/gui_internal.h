#pragma once

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/InstanceAllocator.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

// TEMP
#include "SilverEngine/debug_renderer.h"

namespace sv {

	struct GUI_internal {

		InstanceAllocator<GuiContainer, 10u>	containers;
		InstanceAllocator<GuiButton, 10u>		buttons;
		InstanceAllocator<GuiSlider, 10u>		sliders;

		InstanceAllocator<GuiWindow, 5u>		windows;

		std::vector<GuiWidget*>				root;
		v2_f32								resolution;
		GuiWidget*							widget_focused = nullptr;
		GuiWidget*							widget_clicked;
		GuiWidget*							widget_hovered;
		GuiWidget*							widget_last_hovered = nullptr;
		GuiLockedInput						locked;

		// used during update
		v2_f32		mouse_position;
		bool		mouse_clicked;
		v2_f32		dragged_begin_pos;
		u32			dragged_action_id = 0u;

		//TEMP
		DebugRenderer debug;

	};

}