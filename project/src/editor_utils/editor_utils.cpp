#include "SilverEngine/core.h"

#include "SilverEngine/editor_utils.h"
#include "SilverEngine/renderer.h"

namespace sv {

	void key_shortcuts()
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

	void camera_controller2D(ECS* ecs, CameraComponent& camera, f32 max_projection_length)
	{
		Transform trans = ecs_entity_transform_get(ecs, camera.entity);

		v2_f32 position = trans.getWorldPosition().getVec2();
		
		InputState button_state = input.mouse_buttons[MouseButton_Center];

		if (button_state != InputState_None) {

			v2_f32 drag = input.mouse_position - input.mouse_last_pos;

			position -= drag * v2_f32{ camera.width, camera.height };
		}

		if (input.mouse_wheel != 0.f) {

			f32 force = 0.05f;
			if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

			f32 length = camera.getProjectionLength();

			f32 new_length = std::min(length - input.mouse_wheel * length * force, max_projection_length);
			camera.setProjectionLength(new_length);
		}
		
		trans.setPosition(position.getVec3());
	}

	void camera_controller3D(ECS* ecs, CameraComponent& camera)
	{
		Transform trans = ecs_entity_transform_get(ecs, camera.entity);

		v3_f32 position = trans.getLocalPosition();
		XMVECTOR rotation = trans.getLocalRotation().get_dx();

		XMVECTOR direction;
		XMMATRIX rotation_matrix;

		// Rotation matrix
		rotation_matrix = XMMatrixRotationQuaternion(rotation);

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
		
			rotation = XMQuaternionMultiply(rotation, XMQuaternionRotationRollPitchYaw(drag.y, drag.x, 0.f));
		}

		trans.setRotation(v4_f32(rotation));
		trans.setPosition(position);
	}

    	Editor_ECS* create_editor_ecs(ECS* ecs, GUI* gui, GuiContainer* parent)
	{
	    Editor_ECS* e = new Editor_ECS();
	    
	    e->container = ecs_container_create(gui, parent);
	    e->gui = gui;
	    e->ecs = ecs;
	    
	    return e;
	}
	void update_editor_ecs(Editor_ECS* e)
	{
	    // Update entity buttons
	    {
		u32 count = ecs_entity_count(e->ecs);

		// Update count
		if (e->button_entity.size() != count) {
		    e->button_entity.resize(count, nullptr);

		    foreach(i, count) {

			GuiButton*& button = e->button_entity[i];
			
			if (button == nullptr) {
			    button = ecs_button_create(e->gui, e->container);
			}

			constexpr f32 PIXEL_HEIGHT = 20.f;
			f32 y_value = 5.f + (f32(i) * (PIXEL_HEIGHT + 3.f));

			button->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
			button->y = { y_value, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
			button->w = { 0.9f, GuiConstraint_Relative };
			button->h = { PIXEL_HEIGHT, GuiConstraint_Pixel };

			button->user_id = u64(ecs_entity_get(e->ecs, i));
			button->color = Color::Gray(100u);
		    }
		}
		
		// Update names and parents
		foreach(i, count) {

		    GuiButton* button = e->button_entity[i];
		    Entity button_entity = Entity(button->user_id);
		    Entity entity = ecs_entity_get(e->ecs, i);

		    if (button_entity != entity) {

			button->user_id = u64(entity);
			button_entity = entity;

			// TODO: update parents			
		    }

		    const char* entity_name = "Entity";
		    
		    NameComponent* name = ecs_component_get(e->ecs, entity);
		    if (name && name->name.size()) {
			entity_name = name->name.c_str();
		    }

		    if (strcmp(entity_name, button->text.c_str()) != 0) {
			button->text = entity_name;
		    }
		}
	    }
	    
	}

}
