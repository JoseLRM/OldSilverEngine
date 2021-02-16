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

	Editor_Entity* create_editor_entity(ECS* ecs, GUI* gui, GuiContainer* parent)
	{
	    Editor_Entity* e = new Editor_Entity();
	    
	    e->container = ecs_container_create(gui, parent);
	    e->gui = gui;
	    e->ecs = ecs;

	    f32 y = 5.f;

	    // Create entity transform gui
	    foreach(i, 3u) {

		GuiButton** buttons;

		switch (i) {
		case 0:
		    buttons = e->button_coord;
		    break;
		case 1:
		    buttons = e->button_rotation;
		    break;
		default:
		    buttons = e->button_scale;
		    break;
		}
		
		buttons[0u] = gui_button_create(e->gui, e->container);
		buttons[1u] = gui_button_create(e->gui, e->container);
		buttons[2u] = gui_button_create(e->gui, e->container);

		constexpr f32 HEIGHT = 5.f;
		
		buttons[0u]->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		buttons[0u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		buttons[0u]->w = { 1.f, GuiConstraint_Aspect };
		buttons[0u]->h = { HEIGHT, GuiConstraint_Pixel };
		buttons[0u]->text = "X";
		buttons[0u]->color = { 229u, 25u, 25u, 255u };
		y += HEIGHT + 3.f;

		buttons[1u]->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		buttons[1u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		buttons[1u]->w = { 1.f, GuiConstraint_Aspect };
		buttons[1u]->h = { HEIGHT, GuiConstraint_Pixel };
		buttons[1u]->text = "Y";
		buttons[1u]->color = { 51u, 204u, 51u, 255u };
		y += HEIGHT + 3.f;

		buttons[2u]->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		buttons[2u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		buttons[2u]->w = { 1.f, GuiConstraint_Aspect };
		buttons[2u]->h = { HEIGHT, GuiConstraint_Pixel };
		buttons[2u]->text = "Z";
		buttons[2u]->color = { 13u, 25u, 229u, 255u };
		y += HEIGHT + 3.f;
	    }
	    
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

	    // Recive input
	    GuiButton* button = gui_button_released(e->gui);

	    if (button) {

		for (const GuiButton* b : e->button_entity) {

		    if (b == button) {
			e->selected_entity = Entity(button->user_id);
			SV_LOG_INFO("Entity selected %u", e->selected_entity);
			break;
		    }
		}
	    }
	}
	
	void update_editor_entity(Editor_Entity* e)
	{
	    if (e->entity != SV_ENTITY_NULL || !ecs_entity_exist(e->ecs, e->entity)) {

		e->container->enabled = true;

		
	    }
	    else {

		e->entity = SV_ENTITY_NULL;
		e->container->enabled = false;
	    }
	}
    
	void destroy_editor_ecs(Editor_ECS* e)
	{
	    gui_widget_destroy(e->gui, e->container);
	    delete e;
	}
    
	void destroy_editor_entity(Editor_Entity* e)
	{
	    gui_widget_destroy(e->gui, e->container);
	    delete e;
	}

}
