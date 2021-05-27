#pragma once

namespace sv {

	enum MouseButton : u32 {
		MouseButton_Left,
		MouseButton_Right,
		MouseButton_Center,

		MouseButton_MaxEnum,
		MouseButton_None,
	};

    enum Key : u32 {
		Key_Tab,
		Key_Shift,
		Key_Control,
		Key_Capital,
		Key_Escape,
		Key_Alt,
		Key_Space,
		Key_Left,
		Key_Up,
		Key_Right,
		Key_Down,
		Key_Enter,
		Key_Insert,
		Key_Delete,
		Key_Supr,
		Key_A,
		Key_B,
		Key_C,
		Key_D,
		Key_E,
		Key_F,
		Key_G,
		Key_H,
		Key_I,
		Key_J,
		Key_K,
		Key_L,
		Key_M,
		Key_N,
		Key_O,
		Key_P,
		Key_Q,
		Key_R,
		Key_S,
		Key_T,
		Key_U,
		Key_V,
		Key_W,
		Key_X,
		Key_Y,
		Key_Z,
		Key_Num0,
		Key_Num1,
		Key_Num2,
		Key_Num3,
		Key_Num4,
		Key_Num5,
		Key_Num6,
		Key_Num7,
		Key_Num8,
		Key_Num9,
		Key_F1,
		Key_F2,
		Key_F3,
		Key_F4,
		Key_F5,
		Key_F6,
		Key_F7,
		Key_F8,
		Key_F9,
		Key_F10,
		Key_F11,
		Key_F12,
		Key_F13,
		Key_F14,
		Key_F15,
		Key_F16,
		Key_F17,
		Key_F18,
		Key_F19,
		Key_F20,
		Key_F21,
		Key_F22,
		Key_F23,
		Key_F24,

		Key_MaxEnum,
		Key_None,
	};

    enum InputState : u8 {
		InputState_None,
		InputState_Pressed,
		InputState_Hold,
		InputState_Released,
	};

    enum TextCommand : u32 {
		TextCommand_Null,
		TextCommand_DeleteLeft,
		TextCommand_DeleteRight,
		TextCommand_Enter,
		TextCommand_Escape,
	};

	constexpr u32 INPUT_TEXT_SIZE = 20u;

    struct GlobalInputData {

		InputState keys[Key_MaxEnum];
		InputState mouse_buttons[MouseButton_MaxEnum];

		char text[INPUT_TEXT_SIZE + 1u];
		List<TextCommand> text_commands;

		v2_f32	mouse_position;
		v2_f32	mouse_last_pos;
		v2_f32	mouse_dragged;
		f32	mouse_wheel;

		bool unused;

    };

    extern GlobalInputData SV_API input;
	
}
