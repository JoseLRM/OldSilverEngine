#pragma once

#include "platform/input.h"

namespace sv {

	bool input_update(); // Returns if the engine should close

	void input_key_pressed_add(ui8 id);
	void input_key_released_add(ui8 id);
	void input_mouse_pressed_add(ui8 id);
	void input_mouse_released_add(ui8 id);

	void input_mouse_position_set(float x, float y);
	void input_mouse_dragged_set(int dx, int dy);

}