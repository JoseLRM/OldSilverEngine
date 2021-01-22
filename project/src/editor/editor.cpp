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
			projection.updateMatrix();
		}
	}

}