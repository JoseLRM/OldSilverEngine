#include "SilverEngine/core.h"

#include "SilverEngine/editor.h"

namespace sv {

	void editor_camera_controller2D(v2_f32& position, CameraProjection& projection)
	{
		InputState button_state = input.mouse_buttons[MouseButton_Center];

		if (button_state != InputState_None) {

			position += input.mouse_dragged * v2_f32{ projection.width, projection.height };
		}

		if (input.mouse_wheel != 0.f) {

			f32 force = 0.05f;
			if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

			f32 length = projection.getProjectionLength();
			projection.setProjectionLength(length - input.mouse_wheel * length * force);
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