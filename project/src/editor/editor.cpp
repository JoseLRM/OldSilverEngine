#include "SilverEngine/core.h"

#include "SilverEngine/editor.h"

namespace sv {

	struct Editor_internal {

		std::vector<EditorPanel*> panels;
		GUI* gui;

	};

#define PARSE_EDITOR() sv::Editor_internal& editor = *reinterpret_cast<sv::Editor_internal*>(editor_)

	Editor* editor_create(GUI* gui)
	{
		SV_ASSERT(gui);

		Editor_internal& editor = *new Editor_internal();

		editor.gui = gui;

		return (Editor*)&editor;
	}

	void editor_destroy(Editor* editor_)
	{
		PARSE_EDITOR();

		for (EditorPanel* panel : editor.panels) {
			delete panel;
		}
		editor.panels.clear();

		delete& editor;
	}

	void editor_update(Editor* editor)
	{
	}

	ConsolePanel* editor_console_create(Editor* editor_)
	{
		PARSE_EDITOR();

		return nullptr;
	}

	RuntimePanel* editor_runtime_create(Editor* editor_)
	{
		PARSE_EDITOR();

		RuntimePanel& panel = *new RuntimePanel();
		editor.panels.emplace_back(&panel);

		panel.window = gui_window_create(editor.gui);

		return &panel;
	}

	std::pair<EditorPanel**, u32> editor_panels(Editor* editor_)
	{
		PARSE_EDITOR();
		return std::pair<EditorPanel**, u32>(editor.panels.data(), u32(editor.panels.size()));
	}

	void editor_key_shortcuts()
	{
		if (input.keys[Key_F11] == InputState_Pressed) {
			engine.close_request = true;
		}
		if (input.keys[Key_F10] == InputState_Pressed) {
			if (window_state_get(engine.window) == WindowState_Fullscreen) {
				window_state_set(engine.window, WindowState_Windowed);
			}
			else window_state_set(engine.window, WindowState_Fullscreen);
		}
	}

	void editor_camera_controller2D(v2_f32& position, CameraProjection& projection, f32 max_projection_length)
	{
		InputState button_state = input.mouse_buttons[MouseButton_Center];

		if (button_state != InputState_None) {

			v2_f32 drag = input.mouse_position - input.mouse_last_pos;

			position -= drag * v2_f32{ projection.width, projection.height };
		}

		if (input.mouse_wheel != 0.f) {

			f32 force = 0.05f;
			if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

			f32 length = projection_length_get(projection);

			f32 new_length = std::min(length - input.mouse_wheel * length * force, max_projection_length);
			projection_length_set(projection, new_length);
		}
	}

	void editor_camera_controller3D(v3_f32& position, v2_f32& rotation, CameraProjection& projection)
	{
		XMVECTOR direction;
		XMMATRIX rotation_matrix;

		// Rotation matrix
		rotation_matrix = XMMatrixRotationX(rotation.x) * XMMatrixRotationY(rotation.y);

		// Camera direction
		direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		direction = XMVector3Transform(direction, rotation_matrix);

		f32 norm = XMVectorGetX(XMVector3Length(direction));

		// Zoom
		if (input.mouse_wheel != 0.f) {

			f32 force = 0.1f;
			if (input.keys[Key_Shift] == InputState_Hold)
				force *= 3.f;

			position += v3_f32(direction) * input.mouse_wheel * force;
		}

		// Camera rotation
		if (input.mouse_buttons[MouseButton_Center] == InputState_Hold && (input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

			v2_f32 drag = input.mouse_dragged * 0.01f;
			rotation += v2_f32(-drag.y, drag.x);
			rotation.x = std::min(PI / 2.f - 0.001f, rotation.x);
			rotation.x = std::max(-PI / 2.f + 0.001f, rotation.x);
		}
	}

}