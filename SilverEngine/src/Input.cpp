#include "core.h"

namespace SV {

	bool Input::IsKey(ui8 id) {
		return m_Keys[id];
	}
	bool Input::IsKeyPressed(ui8 id) {
		return m_KeysPressed[id];
	}
	bool Input::IsKeyReleased(ui8 id) {
		return m_KeysReleased[id];
	}

	bool Input::IsMouse(ui8 id) {
		return m_Mouse[id];
	}
	bool Input::IsMousePressed(ui8 id) {
		return m_MousePressed[id];
	}
	bool Input::IsMouseReleased(ui8 id) {
		return m_MouseReleased[id];
	}

	vec2 Input::MousePos()
	{
		return m_Pos;
	}
	vec2 Input::MouseRPos()
	{
		return m_RPos;
	}
	vec2 Input::MouseDragged()
	{
		return m_Dragged;
	}

	void Input::Update()
	{
		// KEYS
		// events
		for (ui8 i = 0; i < 255; ++i) {
			if (m_Keys[i] || m_KeysReleased[i]) {
				//KeyEvent e(i);
				//if (g_KeysPressed[i]) e = KeyEvent(i, JSH_EVENT_PRESSED);
				//else if (g_KeysReleased[i]) e = KeyEvent(i, JSH_EVENT_RELEASED);
				//
				//jshEvent::Dispatch(e);
			}
		}
		// reset pressed and released
		for (ui8 i = 0; i < 255; ++i) {
			m_KeysPressed[i] = false;
			m_KeysReleased[i] = false;
		}

		// MOUSE BUTTONS
		// events
		for (ui8 i = 0; i < 3; ++i) {
			if (m_Mouse[i] || m_MouseReleased[i]) {
				//MouseEvent e(JSH_EVENT_BUTTON);
				//jshEvent::Dispatch(e);
				//
				//MouseButtonEvent be(i);
				//if (g_MousePressed[i]) be = MouseButtonEvent(i, JSH_EVENT_PRESSED);
				//else if (g_MouseReleased[i]) be = MouseButtonEvent(i, JSH_EVENT_RELEASED);
				//jshEvent::Dispatch(be);
			}
		}

		// reset pressed and released
		for (ui8 i = 0; i < 3; ++i) {
			m_MousePressed[i] = false;
			m_MouseReleased[i] = false;
		}

		m_RPos = m_Pos;
		m_Dragged.x = 0.f;
		m_Dragged.y = 0.f;
	}

	void Input::KeyDown(ui8 id) {
		m_Keys[id] = true;
		m_KeysPressed[id] = true;

		//KeyPressedEvent keyPressedEvent(id);
		//jshEvent::Dispatch(keyPressedEvent);
	}
	void Input::KeyUp(ui8 id) {
		m_Keys[id] = false;
		m_KeysReleased[id] = true;

		//KeyReleasedEvent keyReleasedEvent(id);
		//jshEvent::Dispatch(keyReleasedEvent);
	}
	void Input::MouseDown(ui8 id) {
		m_Mouse[id] = true;
		m_MousePressed[id] = true;

		//MouseButtonPressedEvent mousePressedEvent(id);
		//jshEvent::Dispatch(mousePressedEvent);
	}
	void Input::MouseUp(ui8 id) {
		m_Mouse[id] = false;
		m_MouseReleased[id] = true;

		//MouseButtonReleasedEvent mouseReleasedEvent(id);
		//jshEvent::Dispatch(mouseReleasedEvent);
	}
	void Input::MousePos(float x, float y)
	{
		m_Pos = { x, y };
	}

	void Input::MouseDragged(int dx, int dy)
	{
		m_Dragged.x = float(dx);
		m_Dragged.y = float(dy);

		//MouseDraggedEvent e(g_Pos.x, g_Pos.y, g_Dragged.x, g_Dragged.y);
		//jshEvent::Dispatch(e);
		//
		//MouseEvent e2(JSH_EVENT_DRAGGED);
		//jshEvent::Dispatch(e2);
	}

}