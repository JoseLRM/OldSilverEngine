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

			v2_f32 drag = input.mouse_dragged;

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
		if ((input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

			set_cursor_position(engine.window, 0.f, 0.f);

			v2_f32 drag = input.mouse_dragged;

			// TODO: pitch rotation
			XMVECTOR yaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), drag.x);
			//XMVECTOR pitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), -drag.y);

			//rotation = XMQuaternionMultiply(XMQuaternionMultiply(rotation, yaw), pitch);
			rotation = XMQuaternionMultiply(rotation, yaw);
			rotation = XMQuaternionNormalize(rotation); // I need this??
		}

		trans.setRotation(v4_f32(rotation));
		trans.setPosition(position);
	}

	Editor_ECS* create_editor_ecs(ECS* ecs, GUI* gui, GuiContainer* parent)
	{
		Editor_ECS* e = new Editor_ECS();

		e->container = gui_container_create(gui, parent);
		e->container->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container->y = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container->w = { 1.f, GuiConstraint_Relative };
		e->container->h = { 1.f, GuiConstraint_Relative };

		e->container_hierarchy = gui_container_create(gui, e->container);
		e->container_hierarchy->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container_hierarchy->y = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container_hierarchy->w = { 1.f, GuiConstraint_Relative };
		e->container_hierarchy->h = { 1.f, GuiConstraint_Relative };
		e->container_hierarchy->color = Color::Gray(200u);

		e->container_entity = gui_container_create(gui, e->container);
		e->container_entity->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container_entity->y = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		e->container_entity->w = { 1.f, GuiConstraint_Relative };
		e->container_entity->h = { 1.f, GuiConstraint_Relative };
		e->container_entity->color = Color::Gray(200u);

		e->gui = gui;
		e->ecs = ecs;

		f32 y = 5.f;

		// Create entity transform gui
		foreach(i, 3u) {

			GuiDrag** drags;

			switch (i) {
			case 0:
				drags = e->drag_coord;
				break;
			case 1:
				drags = e->drag_rotation;
				break;
			default:
				drags = e->drag_scale;
				break;
			}

			drags[0u] = gui_drag_create(e->gui, e->container_entity);
			drags[1u] = gui_drag_create(e->gui, e->container_entity);
			drags[2u] = gui_drag_create(e->gui, e->container_entity);

			constexpr f32 HEIGHT = 20.f;

			// X

			drags[0u]->x = { 0.05f, GuiConstraint_Relative, GuiCoordAlignment_Left };
			drags[0u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
			drags[0u]->w = { 0.25f, GuiConstraint_Relative };
			drags[0u]->h = { HEIGHT, GuiConstraint_Pixel };
			drags[0u]->color = { 229u, 25u, 25u, 255u };

			// Y

			drags[1u]->x = { 0.5f, GuiConstraint_Relative, GuiCoordAlignment_Center };
			drags[1u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
			drags[1u]->w = { 0.25f, GuiConstraint_Relative };
			drags[1u]->h = { HEIGHT, GuiConstraint_Pixel };
			drags[1u]->color = { 51u, 204u, 51u, 255u };

			// Z

			drags[2u]->x = { 0.05f, GuiConstraint_Relative, GuiCoordAlignment_InverseRight };
			drags[2u]->y = { y, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
			drags[2u]->w = { 0.25f, GuiConstraint_Relative };
			drags[2u]->h = { HEIGHT, GuiConstraint_Pixel };
			drags[2u]->color = { 13u, 25u, 229u, 255u };

			y += HEIGHT + 3.f;
		}

		return e;
	}

	void update_editor_ecs(Editor_ECS* e)
	{
		if (e->selected_entity == SV_ENTITY_NULL || !ecs_entity_exist(e->ecs, e->selected_entity)) {

			e->container_hierarchy->enabled = true;
			e->container_entity->enabled = false;
			e->selected_entity = SV_ENTITY_NULL;

			// Update entity buttons
			{
				u32 count = ecs_entity_count(e->ecs);

				// Update count
				if (e->button_entity.size() != count) {

					if (e->button_entity.size() > count) {
						for (u32 i = count; i < e->button_entity.size(); ++i) {
							gui_widget_destroy(e->gui, e->button_entity[i]);
						}
					}

					e->button_entity.resize(count, nullptr);

					foreach(i, count) {

						GuiButton*& button = e->button_entity[i];

						if (button == nullptr) {
							button = gui_button_create(e->gui, e->container_hierarchy);
						}

						constexpr f32 PIXEL_HEIGHT = 20.f;
						f32 y_value = 5.f + (f32(i) * (PIXEL_HEIGHT + 3.f));

						button->x = { 0.01f, GuiConstraint_Relative, GuiCoordAlignment_Left };
						button->y = { y_value, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
						button->w = { 0.9f, GuiConstraint_Relative };
						button->h = { PIXEL_HEIGHT, GuiConstraint_Pixel };

						button->user_id = u64(ecs_entity_get(e->ecs, i));
						button->color = Color::Gray(100u);
						button->hover_color = Color::Gray(150u);
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

					NameComponent* name = ecs_component_get<NameComponent>(e->ecs, entity);
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
						break;
					}
				}
			}
		}
		else {

			e->container_hierarchy->enabled = false;
			e->container_entity->enabled = true;

			Transform trans = ecs_entity_transform_get(e->ecs, e->selected_entity);

			// Update entity transform
			GuiWidget* focused = gui_widget_focused(e->gui);

			if (focused) {

			    foreach(i, 3u) {

				if (focused == e->drag_coord[i]) {

				    GuiDrag& drag = *e->drag_coord[i];

				    switch (i)
				    {
				    case 0:
					trans.setPositionX(drag.value);
					break;

				    case 1:
					trans.setPositionY(drag.value);
					break;

				    default:
					trans.setPositionZ(drag.value);
					break;
				    }
				    
				    break;
				}
			    }
			}

			// Update drag values
			v3_f32 position = trans.getLocalPosition();
			v3_f32 rotation = trans.getLocalEulerRotation();
			v3_f32 scale = trans.getLocalScale();
			
			e->drag_coord[0u]->value = position.x;
			e->drag_coord[1u]->value = position.y;
			e->drag_coord[2u]->value = position.z;

			e->drag_rotation[0u]->value = rotation.x;
			e->drag_rotation[1u]->value = rotaiton.y;
			e->drag_rotation[2u]->value = rotation.z;

			e->drag_scale[0u]->value = scale.x;
			e->drag_scale[1u]->value = scale.y;
			e->drag_scale[2u]->value = scale.z;
			
			//TEMP
			if (input.keys[Key_B]) e->selected_entity = SV_ENTITY_NULL;
		}
	}

	void destroy_editor_ecs(Editor_ECS* e)
	{
		gui_widget_destroy(e->gui, e->container);
		delete e;
	}

}
