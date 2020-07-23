#include "core.h"

#include "Input.h"

namespace SV {
	namespace Engine {

		bool g_Keys[255];
		bool g_KeysPressed[255];
		bool g_KeysReleased[255];
			 
		bool g_Mouse[3];
		bool g_MousePressed[3];
		bool g_MouseReleased[3];
			 
		vec2 g_Pos;
		vec2 g_RPos;
		vec2 g_Dragged;

		bool g_CloseRequest = false;

		bool IsKey(ui8 id) {
			return g_Keys[id];
		}
		bool IsKeyPressed(ui8 id) {
			return g_KeysPressed[id];
		}
		bool IsKeyReleased(ui8 id) {
			return g_KeysReleased[id];
		}

		bool IsMouse(ui8 id) {
			return g_Mouse[id];
		}
		bool IsMousePressed(ui8 id) {
			return g_MousePressed[id];
		}
		bool IsMouseReleased(ui8 id) {
			return g_MouseReleased[id];
		}

		vec2 GetMousePos()
		{
			return g_Pos;
		}
		vec2 GetMouseRPos()
		{
			return g_RPos;
		}
		vec2 GetMouseDragged()
		{
			return g_Dragged;
		}

		void RequestClose() noexcept { g_CloseRequest = true; }

		namespace _internal {

			bool UpdateInput()
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

			void AddKeyPressed(ui8 id) {
				g_Keys[id] = true;
				g_KeysPressed[id] = true;
			}
			void AddKeyReleased(ui8 id) {
				g_Keys[id] = false;
				g_KeysReleased[id] = true;
			}
			void AddMousePressed(ui8 id) {
				g_Mouse[id] = true;
				g_MousePressed[id] = true;
			}
			void AddMouseReleased(ui8 id) {
				g_Mouse[id] = false;
				g_MouseReleased[id] = true;
			}
			void SetMousePos(float x, float y)
			{
				g_Pos = { x, y };
			}

			void SetMouseDragged(int dx, int dy)
			{
				g_Dragged.x = float(dx);
				g_Dragged.y = float(dy);
			}

		}
	}
}