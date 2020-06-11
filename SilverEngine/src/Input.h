#pragma once

#include "core.h"
#include "EngineDevice.h"

#define SV_MOUSE_LEFT			0
#define SV_MOUSE_RIGHT			1
#define SV_MOUSE_CENTER        2

#define SV_KEY_BACK           0x08
#define SV_KEY_TAB            0x09
#define SV_KEY_CLEAR          0x0C
#define SV_KEY_RETURN         0x0D
#define SV_KEY_SHIFT          0x10
#define SV_KEY_CONTROL        0x11
#define SV_KEY_MENU           0x12
#define SV_KEY_PAUSE          0x13
#define SV_KEY_CAPITAL        0x14
#define SV_KEY_KANA           0x15
#define SV_KEY_HANGEUL        0x15  
#define SV_KEY_HANGUL         0x15
#define SV_KEY_JUNJA          0x17
#define SV_KEY_FINAL          0x18
#define SV_KEY_HANJA          0x19
#define SV_KEY_KANJI          0x19
#define SV_KEY_ESCAPE         0x1B
#define SV_KEY_CONVERT        0x1C
#define SV_KEY_NONCONVERT     0x1D
#define SV_KEY_ACCEPT         0x1E
#define SV_KEY_MODECHANGE     0x1F
#define SV_KEY_SPACE          0x20
#define SV_KEY_PRIOR          0x21
#define SV_KEY_NEXT           0x22
#define SV_KEY_END            0x23
#define SV_KEY_HOME           0x24
#define SV_KEY_LEFT           0x25
#define SV_KEY_UP             0x26
#define SV_KEY_RIGHT          0x27
#define SV_KEY_DOWN           0x28
#define SV_KEY_SELECT         0x29
#define SV_KEY_PRINT          0x2A
#define SV_KEY_EXECUTE        0x2B
#define SV_KEY_SNAPSHOT       0x2C
#define SV_KEY_INSERT         0x2D
#define SV_KEY_DELETE         0x2E
#define SV_KEY_HELP           0x2F
#define SV_KEY_LWIN           0x5B
#define SV_KEY_RWIN           0x5C
#define SV_KEY_APPS           0x5D
#define SV_KEY_SLEEP          0x5F
#define SV_KEY_NUMPAD0        0x60
#define SV_KEY_NUMPAD1        0x61
#define SV_KEY_NUMPAD2        0x62
#define SV_KEY_NUMPAD3        0x63
#define SV_KEY_NUMPAD4        0x64
#define SV_KEY_NUMPAD5        0x65
#define SV_KEY_NUMPAD6        0x66
#define SV_KEY_NUMPAD7        0x67
#define SV_KEY_NUMPAD8        0x68
#define SV_KEY_NUMPAD9        0x69
#define SV_KEY_MULTIPLY       0x6A
#define SV_KEY_ADD            0x6B
#define SV_KEY_SEPARATOR      0x6C
#define SV_KEY_SUBTRACT       0x6D
#define SV_KEY_DECIMAL        0x6E
#define SV_KEY_DIVIDE         0x6F
#define SV_KEY_F1             0x70
#define SV_KEY_F2             0x71
#define SV_KEY_F3             0x72
#define SV_KEY_F4             0x73
#define SV_KEY_F5             0x74
#define SV_KEY_F6             0x75
#define SV_KEY_F7             0x76
#define SV_KEY_F8             0x77
#define SV_KEY_F9             0x78
#define SV_KEY_F10            0x79
#define SV_KEY_F11            0x7A
#define SV_KEY_F12            0x7B
#define SV_KEY_F13            0x7C
#define SV_KEY_F14            0x7D
#define SV_KEY_F15            0x7E
#define SV_KEY_F16            0x7F
#define SV_KEY_F17            0x80
#define SV_KEY_F18            0x81
#define SV_KEY_F19            0x82
#define SV_KEY_F20            0x83
#define SV_KEY_F21            0x84
#define SV_KEY_F22            0x85
#define SV_KEY_F23            0x86
#define SV_KEY_F24            0x87
#define SV_KEY_NUMLOCK        0x90
#define SV_KEY_SCROLL         0x91
#define SV_KEY_OEM_NEC_EQUAL  0x92
#define SV_KEY_OEM_FJ_JISHO   0x92
#define SV_KEY_OEM_FJ_MASSHOU 0x93
#define SV_KEY_OEM_FJ_TOUROKU 0x94
#define SV_KEY_OEM_FJ_LOYA    0x95
#define SV_KEY_OEM_FJ_ROYA    0x96
#define SV_KEY_LSHIFT         0xA0
#define SV_KEY_RSHIFT         0xA1
#define SV_KEY_LCONTROL       0xA2
#define SV_KEY_RCONTROL       0xA3
#define SV_KEY_LMENU          0xA4
#define SV_KEY_RMENU          0xA5


namespace SV {

	class Input : public SV::EngineDevice {
		bool m_Keys[255];
		bool m_KeysPressed[255];
		bool m_KeysReleased[255];
			 
		bool m_Mouse[3];
		bool m_MousePressed[3];
		bool m_MouseReleased[3];
			 
		vec2 m_Pos;
		vec2 m_RPos;
		vec2 m_Dragged;

	public:
		friend Engine;
		friend Window;

		bool IsKey(ui8 id);
		bool IsKeyPressed(ui8 id);
		bool IsKeyReleased(ui8 id);

		bool IsMouse(ui8 id);
		bool IsMousePressed(ui8 id);
		bool IsMouseReleased(ui8 id);

		vec2 MousePos();
		vec2 MouseRPos();
		vec2 MouseDragged();

	private:
		void Update();

		void KeyDown(ui8 id);
		void KeyUp(ui8 id);

		void MouseDown(ui8 id);
		void MouseUp(ui8 id);

		void MousePos(float x, float y);
		void MouseDragged(int dx, int dy);

	};
}