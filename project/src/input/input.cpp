#include "SilverEngine/core.h"

#include "input_internal.h"

namespace sv {

	static bool g_Keys[255];
	static bool g_KeysPressed[255];
	static bool g_KeysReleased[255];

	static std::string g_Text;
	static std::vector<TextCommand> g_TextCommands;

	static bool g_Mouse[3];
	static bool g_MousePressed[3];
	static bool g_MouseReleased[3];

	static v2_f32 g_Pos;
	static v2_f32 g_RPos;
	static v2_f32 g_Dragged;
	static f32 g_Wheel;

	static Window* g_FocusedWindow = nullptr;

	void input_update()
	{
		g_Text.clear();
		g_TextCommands.resize(1u);
		g_TextCommands.back() = TextCommand_Null;

		// KEYS
		// reset pressed and released
		for (u8 i = 0; i < 255; ++i) {
			g_KeysPressed[i] = false;
			g_KeysReleased[i] = false;
		}

		// MOUSE BUTTONS
		// reset pressed and released
		for (u8 i = 0; i < 3; ++i) {
			g_MousePressed[i] = false;
			g_MouseReleased[i] = false;
		}

		g_RPos = g_Pos;
		g_Dragged.x = 0.f;
		g_Dragged.y = 0.f;
		g_Wheel = 0.f;
	}

	void input_key_pressed_add(u8 id)
	{
		g_Keys[id] = true;
		g_KeysPressed[id] = true;
	}
	void input_key_released_add(u8 id)
	{
		g_Keys[id] = false;
		g_KeysReleased[id] = true;
	}
	void input_text_add(char c)
	{
		g_Text += c;
	}
	void input_text_command_add(TextCommand c)
	{
		SV_ASSERT(g_TextCommands.size());
		g_TextCommands.back() = c;
		g_TextCommands.emplace_back(TextCommand_Null);
	}
	void input_mouse_pressed_add(u8 id)
	{
		g_Mouse[id] = true;
		g_MousePressed[id] = true;
	}
	void input_mouse_released_add(u8 id)
	{
		g_Mouse[id] = false;
		g_MouseReleased[id] = true;
	}

	void input_mouse_position_set(f32 x, f32 y)
	{
		g_Pos = { x, y };
	}
	void input_mouse_dragged_set(i32 dx, i32 dy)
	{
		g_Dragged.x = f32(dx);
		g_Dragged.y = f32(dy);
	}
	void input_mouse_wheel_set(f32 wheel)
	{
		g_Wheel = wheel;
	}

	void input_focussed_window_set(Window* window)
	{
		g_FocusedWindow = window;
	}

	bool input_key(u8 id) {
		return g_Keys[id];
	}
	bool input_key_pressed(u8 id) {
		return g_KeysPressed[id];
	}
	bool input_key_released(u8 id) {
		return g_KeysReleased[id];
	}

	const char* input_text()
	{
		return g_Text.c_str();
	}

	const TextCommand* input_text_commands()
	{
		return g_TextCommands.data();
	}

	bool input_mouse(u8 id) {
		return g_Mouse[id];
	}
	bool input_mouse_pressed(u8 id) {
		return g_MousePressed[id];
	}
	bool input_mouse_released(u8 id) {
		return g_MouseReleased[id];
	}

	v2_f32 input_mouse_position()
	{
		return g_Pos;
	}
	v2_f32 input_mouse_last_position()
	{
		return g_RPos;
	}
	v2_f32 input_mouse_dragged()
	{
		//return g_Dragged;
		//TEMP:
		return g_Pos - g_RPos;
	}

	float input_mouse_wheel()
	{
		return g_Wheel;
	}

	Window* input_focussed_window()
	{
		return g_FocusedWindow;
	}

}