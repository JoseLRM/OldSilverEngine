#include "SilverEngine/core.h"

#include "SilverEngine/renderer.h"

namespace sv {

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

		input.unused = false;
		trans.setPosition(position.getVec3());
	}

	void camera_controller3D(ECS* ecs, CameraComponent& camera, f32 velocity)
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

			f32 force = velocity;
			if (input.keys[Key_Shift] == InputState_Hold)
				force *= 3.f;

			position += v3_f32(direction) * input.mouse_wheel * force;
		}

		// Camera rotation
		if ((input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

			set_cursor_position(engine.window, 0.f, 0.f);

			v2_f32 drag = input.mouse_dragged * 2.f;

			// TODO: pitch limit
			XMVECTOR pitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), -drag.y);
			XMVECTOR yaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), drag.x);

			rotation = XMQuaternionMultiply(pitch, rotation);
			rotation = XMQuaternionMultiply(rotation, yaw);
			rotation = XMQuaternionNormalize(rotation);
		}

		input.unused = false;
		trans.setRotation(v4_f32(rotation));
		trans.setPosition(position);
	}

	struct EditorStyle {

		GuiWindowStyle window_style;
		GuiButtonStyle button_style;
		GuiSliderStyle slider_style;

		f32 vertical_padding = 5.f;
		f32 separator = 30.f;

	};
	
	struct Editor {

		GUI* gui;
		EditorStyle style;

		Entity selected_entity = SV_ENTITY_NULL;
		bool camera_controller = false;
	};

	Editor editor;

	Result command_show_window(const char** args, u32 argc)
	{
		bool show = true;

		if (argc == 0u) {
			SV_LOG_ERROR("This command needs some argument");
			return Result_InvalidUsage;
		}

		if (argc > 2u) {
			SV_LOG_ERROR("Too much arguments");
			return Result_InvalidUsage;
		}

		if (argc == 2u) {

			if (strcmp(args[1], "show") == 0 || strcmp(args[1], "1") == 0) show = true;
			else if (strcmp(args[1], "hide") == 0 || strcmp(args[1], "0") == 0) show = false;
			else {
				SV_LOG_ERROR("Invalid argument '%s'", args[1]);
				return Result_InvalidUsage;
			}
		}

		Result res = show ? gui_show_window(editor.gui, args[0]) : gui_hide_window(editor.gui, args[0]);

		if (result_okay(res)) {
			SV_LOG("%s '%s'", show ? "Showing" : "Hiding", args[0]);
		}
		else SV_LOG_ERROR("Window '%s' not found", args[0]);
		
		return res;
	}

	Result initialize_editor()
	{
		svCheck(gui_create(hash_string("EDITOR GUI"), &editor.gui));

		// Register commands
		svCheck(register_command("wnd", command_show_window));

		return Result_Success;
	}

	Result close_editor()
	{
		svCheck(gui_destroy(editor.gui));
		return Result_Success;
	}

	static void show_component_info(f32& y, CompID comp_id, BaseComponent* comp)
	{
		GUI* gui = editor.gui;

		GuiContainerStyle container_style;
		container_style.color = Color::Red(100u);

		GuiTextStyle text_style;
		text_style.text_color = Color::Black();
		text_style.text_alignment = TextAlignment_Left;

		GuiCheckBoxStyle checkbox_style;
		checkbox_style.active_box = GuiBox::Triangle(Color::Black(), true, 0.5f);
		checkbox_style.inactive_box = GuiBox::Triangle(Color::Black(), false, 0.5f);
		checkbox_style.color = Color::Black(0u);

		gui_begin_container(gui, GuiCoord::Center(), GuiCoord::Flow(y), GuiDim::Relative(0.9f), GuiDim::Pixel(30.f), container_style);

		gui_text(gui, ecs_component_name(comp_id), { 35.f, GuiConstraint_Pixel, GuiAlignment_Left }
		, GuiCoord::Center(), GuiDim::Pixel(300.f), GuiDim::Relative(1.f), text_style);

		bool show = gui_checkbox_id(gui, u64(comp_id << 32u + comp->entity), { 0.f, GuiConstraint_Relative, GuiAlignment_Left }
		, GuiCoord::Center(), GuiDim::Aspect(1.f), GuiDim::Relative(1.f), checkbox_style);

		gui_end_container(gui);

		if (show) {
			SV_LOG("All right");
		}

		y += 30.f + editor.style.vertical_padding;
	}

	// TEMP
	struct TempLine {
		v3_f32 p0;
		v3_f32 p1;
	};
	static std::vector<TempLine> lines;

	SV_INLINE static void select_entity()
	{
		v2_f32 mouse = input.mouse_position;

		ECS* ecs = engine.scene->ecs;
		CameraComponent* camera = get_main_camera(engine.scene);

		if (camera == nullptr) return;

		Transform camera_trans = ecs_entity_transform_get(ecs, camera->entity);
		v3_f32 camera_position = camera_trans.getWorldPosition();
		v4_f32 camera_rotation = camera_trans.getWorldRotation();

		Ray ray = screen_to_world_ray(mouse, camera_position, camera_rotation, camera);

		XMVECTOR ray_origin = ray.origin.getDX(1.f);
		XMVECTOR ray_direction = ray.direction.getDX(0.f);

		Entity selected = SV_ENTITY_NULL;
		f32 distance = f32_max;

		lines.push_back({ ray.origin, ray.origin + ray.direction * 40.f });

		EntityView<MeshComponent> meshes(ecs);

		for (MeshComponent& m : meshes) {

			Transform trans = ecs_entity_transform_get(ecs, m.entity);

			XMMATRIX itm = XMMatrixInverse(0, trans.getWorldMatrix());

			ray.origin = v3_f32(XMVector4Transform(ray_origin, itm));
			ray.direction = v3_f32(XMVector4Transform(ray_direction, itm));
			
			Mesh& mesh = *m.mesh.get();

			u32 triangles = u32(mesh.indices.size()) / 3u;

			for (u32 i = 0u; i < triangles; ++i) {

				u32 i0 = mesh.indices[i * 3u + 0u];
				u32 i1 = mesh.indices[i * 3u + 1u];
				u32 i2 = mesh.indices[i * 3u + 2u];

				v3_f32 p0 = mesh.positions[i0];
				v3_f32 p1 = mesh.positions[i1];
				v3_f32 p2 = mesh.positions[i2];

				v3_f32 intersection;

				if (intersect_ray_vs_traingle(ray, p0, p1, p2, intersection)) {

					f32 dis = intersection.length();
					if (dis < distance) {
						distance = dis;
						selected = m.entity;
					}
				}
			}
		}

		editor.selected_entity = selected;
	}

	void display_entity_hierarchy()
	{
		GUI* g = editor.gui;
		ECS* ecs = engine.scene->ecs;

		f32 y = 5.f;

		if (gui_begin_window(g, "Hierarchy", editor.style.window_style)) {

			u32 entity_count = ecs_entity_count(ecs);

			foreach(entity_index, entity_count) {

				Entity entity = ecs_entity_get(ecs, entity_index);

				const char* name = get_entity_name(ecs, entity);

				constexpr f32 BUTTON_HEIGHT = 25.f;
				
				if (gui_button(g, name, { 5.f, GuiConstraint_Pixel, GuiAlignment_Left },
					       GuiCoord::Flow(y), GuiDim::Relative(0.9f), GuiDim::Pixel(BUTTON_HEIGHT), editor.style.button_style)) {

					editor.selected_entity = entity;
				}

				y += BUTTON_HEIGHT + 2.f;
			}
			
			gui_end_window(g);
		}
	}

	void display_entity_inspector()
	{
		GUI* g = editor.gui;
		ECS* ecs = engine.scene->ecs;
		Entity selected = editor.selected_entity;

		constexpr f32 NAME_HEIGHT = 40.f;
		constexpr f32 TRANSFORM_HEIGHT = 20.f;
		
		f32 y = 5.f;

		if (gui_begin_window(g, "Inspector", editor.style.window_style)) {

			if (selected != SV_ENTITY_NULL) {

				// Entity name
				{
					const char* entity_name = get_entity_name(ecs, selected);

					gui_text(g, entity_name, GuiCoord::Center(), GuiCoord::Flow(y), GuiDim::Relative(0.9f), GuiDim::Pixel(NAME_HEIGHT));
					y += NAME_HEIGHT + editor.style.separator + editor.style.vertical_padding;
				}

				// Entity transform
				{
					// TODO: euler rotation
					Transform trans = ecs_entity_transform_get(ecs, selected);
					v3_f32 position = trans.getWorldPosition();
					v3_f32 scale = trans.getWorldScale();

					foreach(i, 2u) {

						v3_f32* values;

						switch (i) {
						case 0:
							values = &position;
							break;
						case 1:
							values = &scale;
							break;
						}

						// TODO: Hot color
						GuiButtonStyle x_style;
						x_style.color = { 229u, 25u, 25u, 255u };

						GuiButtonStyle y_style;
						y_style.color = { 51u, 204u, 51u, 255u };

						GuiButtonStyle z_style;
						z_style.color = { 13u, 25u, 229u, 255u };

						constexpr f32 EXTERN_PADDING = 0.03f;
						constexpr f32 INTERN_PADDING = 0.07f;

						constexpr f32 ELEMENT_WIDTH = (1.f - EXTERN_PADDING * 2.f - INTERN_PADDING * 2.f) / 3.f;

						// X
						if (gui_button(g, "X", { 0.5f - ELEMENT_WIDTH - INTERN_PADDING, GuiConstraint_Relative, GuiAlignment_Center
							}, GuiCoord::Flow(y), GuiDim::Relative(ELEMENT_WIDTH),
							GuiDim::Pixel(TRANSFORM_HEIGHT), x_style)) {

							// TEMP
							values->x = math_random_f32(u32(timer_now() * 100.f), -10.f, 10.f);
						}

						// Y
						if (gui_button(g, "Y", GuiCoord::Center(), GuiCoord::Flow(y),
							GuiDim::Relative(ELEMENT_WIDTH), GuiDim::Pixel(TRANSFORM_HEIGHT), y_style)) {

							// TEMP
							values->y = math_random_f32(u32(timer_now() * 100.f), -10.f, 10.f);
						}

						// Z
						if (gui_button(g, "Z", { 0.5f + ELEMENT_WIDTH + INTERN_PADDING, GuiConstraint_Relative, GuiAlignment_Center
							}, GuiCoord::Flow(y), GuiDim::Relative(ELEMENT_WIDTH),
							GuiDim::Pixel(TRANSFORM_HEIGHT), z_style)) {

							// TEMP
							values->z = math_random_f32(u32(timer_now() * 100.f), -10.f, 10.f);
						}

						y += TRANSFORM_HEIGHT + editor.style.vertical_padding;
					}

					y += editor.style.separator;

					trans.setPosition(position);
					trans.setScale(scale);
				}

				// Entity components
				{
					u32 comp_count = ecs_entity_component_count(ecs, selected);

					foreach(comp_index, comp_count) {

						auto [comp_id, comp] = ecs_component_get_by_index(ecs, selected, comp_index);

						show_component_info(y, comp_id, comp);
					}
				}
			}

			gui_end_window(g);
		}
	}
	
	void update_editor()
	{
		GUI* g = editor.gui;

		// KEY SHORTCUTS
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
			if (input.keys[Key_F3] == InputState_Pressed) {
				editor.camera_controller = false;
				engine.console_active = !engine.console_active;
			}
			if (input.keys[Key_F2] == InputState_Pressed) {
				editor.camera_controller = !editor.camera_controller;
				engine.console_active = false;
			}
		}

		if (editor.camera_controller)
			camera_controller3D(engine.scene->ecs, *get_main_camera(engine.scene), 0.3f);

		gui_begin(g, f32(window_width_get(engine.window)), f32(window_height_get(engine.window)));

		if (!editor.camera_controller) {

			display_entity_hierarchy();
			display_entity_inspector();

		}

		gui_end(g);

		if (input.unused) {

			// Entity selection
			if (input.mouse_buttons[MouseButton_Left] == InputState_Released)
				select_entity();
		}
	}

	void draw_editor()
	{
		CommandList cmd = graphics_commandlist_get();

		gui_draw(editor.gui, engine.scene->offscreen, cmd);

		begin_debug_batch(cmd);

		for (auto& l : lines) {
			draw_debug_line(l.p0, l.p1, Color::Red(), cmd);
		}

		CameraComponent* cam = get_main_camera(engine.scene);
		Transform trans = ecs_entity_transform_get(engine.scene->ecs, cam->entity);

		end_debug_batch(engine.scene->offscreen, engine.scene->depthstencil, camera_view_projection_matrix(trans.getWorldPosition(), trans.getWorldRotation(), *cam), cmd);
	}

}
