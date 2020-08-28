#include "core.h"

#include "input_internal.h"

namespace sv {

	static bool g_Keys[255];
	static bool g_KeysPressed[255];
	static bool g_KeysReleased[255];
	
	static bool g_Mouse[3];
	static bool g_MousePressed[3];
	static bool g_MouseReleased[3];
	
	static vec2 g_Pos;
	static vec2 g_RPos;
	static vec2 g_Dragged;
	
	static bool g_CloseRequest = false;

	bool input_update()
	{
		if (g_CloseRequest) return true;

		// KEYS
		// reset pressed and released
		for (ui8 i = 0; i < 255; ++i) {
			g_KeysPressed[i] = false;
			g_KeysReleased[i] = false;
		}

		// MOUSE BUTTONS
		// reset pressed and released
		for (ui8 i = 0; i < 3; ++i) {
			g_MousePressed[i] = false;
			g_MouseReleased[i] = false;
		}

		g_RPos = g_Pos;
		g_Dragged.x = 0.f;
		g_Dragged.y = 0.f;

		return false;
	}

	void input_key_pressed_add(ui8 id)
	{
		g_Keys[id] = true;
		g_KeysPressed[id] = true;
	}
	void input_key_released_add(ui8 id)
	{
		g_Keys[id] = false;
		g_KeysReleased[id] = true;
	}
	void input_mouse_pressed_add(ui8 id)
	{
		g_Mouse[id] = true;
		g_MousePressed[id] = true;
	}
	void input_mouse_released_add(ui8 id)
	{
		g_Mouse[id] = false;
		g_MouseReleased[id] = true;
	}

	void input_mouse_position_set(float x, float y)
	{
		g_Pos = { x, y };
	}
	void input_mouse_dragged_set(int dx, int dy)
	{
		g_Dragged.x = float(dx);
		g_Dragged.y = float(dy);
	}

	bool input_key(ui8 id) {
		return g_Keys[id];
	}
	bool input_key_pressed(ui8 id) {
		return g_KeysPressed[id];
	}
	bool input_key_released(ui8 id) {
		return g_KeysReleased[id];
	}

	bool input_mouse(ui8 id) {
		return g_Mouse[id];
	}
	bool input_mouse_pressed(ui8 id) {
		return g_MousePressed[id];
	}
	bool input_mouse_released(ui8 id) {
		return g_MouseReleased[id];
	}

	vec2 input_mouse_position_get()
	{
		return g_Pos;
	}
	vec2 input_mouse_position_get_last()
	{
		return g_RPos;
	}
	vec2 input_mouse_dragged_get()
	{
		//return g_Dragged;
		//TEMP:
		return g_Pos - g_RPos;
	}

	void engine_request_close() noexcept { g_CloseRequest = true; }

}