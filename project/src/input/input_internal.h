#pragma once

#include "SilverEngine/input.h"

namespace sv {

	void input_update();

	void input_key_pressed_add(u8 id);
	void input_key_released_add(u8 id);
	void input_text_add(char c);
	void input_text_command_add(TextCommand c);
	void input_mouse_pressed_add(u8 id);
	void input_mouse_released_add(u8 id);

	void input_mouse_position_set(f32 x, f32 y);
	void input_mouse_dragged_set(i32 dx, i32 dy);
	void input_mouse_wheel_set(f32 wheel);

	void input_focussed_window_set(Window* window);

}