#include "SilverEngine/core.h"

#include "SilverEngine/renderer.h"

namespace sv {

	void camera_controller2D(Scene* scene, CameraComponent& camera, f32 max_projection_length)
	{
		Transform trans = get_entity_transform(scene, camera.entity);

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

	void camera_controller3D(Scene* scene, CameraComponent& camera, f32 velocity)
	{
		Transform trans = get_entity_transform(scene, camera.entity);

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

	enum GizmosTransformMode : u32 {
		GizmosTransformMode_None,
		GizmosTransformMode_Position
	};
	
	struct GizmosInfo {

		GizmosTransformMode mode = GizmosTransformMode_Position;

	};

	struct Editor {

		GUI* gui;
		EditorStyle style;

		Entity selected_entity = SV_ENTITY_NULL;
		bool camera_controller = false;

		GizmosInfo gizmos;
	};

	Editor editor;

	/////////////////////////////////////////////// GIZMOS ///////////////////////////////////////////////////////

	static void update_gizmos()
	{
		if (!input.unused) return;
		if (editor.selected_entity == SV_ENTITY_NULL) return;


	}

	static void draw_gizmos(GPUImage* offscreen, CommandList cmd)
	{
		if (editor.selected_entity == SV_ENTITY_NULL) return;

		GizmosInfo& info = editor.gizmos;

		Transform trans = get_entity_transform(engine.scene, editor.selected_entity);
		v3_f32 position = trans.getLocalPosition();

		constexpr f32 GIZMOS_SIZE = 1.f;

		switch (info.mode)
		{

		case GizmosTransformMode_Position:
		{
			draw_debug_line(position, position + v3_f32::right() * GIZMOS_SIZE, Color::Red(), cmd);
			draw_debug_line(position, position + v3_f32::up() * GIZMOS_SIZE, Color::Green(), cmd);
			draw_debug_line(position, position + v3_f32::forward() * GIZMOS_SIZE, Color::Blue(), cmd);
		}
		break;

		}
	}

	/////////////////////////////////////////////// COMMANDS ///////////////////////////////////////////////////////

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

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

		u64 gui_id = u64((u64(comp_id) << 32u) + comp->entity);

		gui_begin_container(gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), container_style);

		gui_text(gui, get_component_name(comp_id), GuiCoord::Pixel(35.f), GuiCoord::IPixel(10.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), text_style);
		
		bool show = gui_checkbox_id(gui, gui_id, GuiCoord::Pixel(0.f), GuiCoord::Aspect(), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), checkbox_style);

		gui_end_container(gui);

		if (show) {
			SV_LOG("All right");
		}

		y += 30.f + editor.style.vertical_padding;
	}

	SV_INLINE static void select_entity()
	{
		v2_f32 mouse = input.mouse_position;

		CameraComponent* camera = get_main_camera(engine.scene);

		if (camera == nullptr) return;

		Transform camera_trans = get_entity_transform(engine.scene, camera->entity);
		v3_f32 camera_position = camera_trans.getWorldPosition();
		v4_f32 camera_rotation = camera_trans.getWorldRotation();

		Ray ray = screen_to_world_ray(mouse, camera_position, camera_rotation, camera);

		XMVECTOR ray_origin = ray.origin.getDX(1.f);
		XMVECTOR ray_direction = ray.direction.getDX(0.f);

		Entity selected = SV_ENTITY_NULL;
		f32 distance = f32_max;

		EntityView<MeshComponent> meshes(engine.scene);

		for (MeshComponent& m : meshes) {

			Transform trans = get_entity_transform(engine.scene, m.entity);

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

	static void show_entity_popup(Entity entity, bool& destroy) 
	{
		if (gui_begin_popup(editor.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 0x3254fa + u64(entity))) {

			f32 y = editor.style.vertical_padding;
			constexpr f32 H = 20.f;

			destroy = gui_button(editor.gui, "Destroy", GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
			y += H + editor.style.vertical_padding;
			
			if (gui_button(editor.gui, "Duplicate", GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				duplicate_entity(engine.scene, entity);
			}
			y += H + editor.style.vertical_padding;

			if (gui_button(editor.gui, "Create Child", GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				create_entity(engine.scene, entity);
			}

			gui_end_popup(editor.gui);
		}
	}

	static void show_entity(Entity entity, f32& y, f32 xoff)
	{
		GUI* g = editor.gui;

		const char* name = get_entity_name(engine.scene, entity);

		constexpr f32 BUTTON_HEIGHT = 25.f;

		u32 child_count = get_entity_childs_count(engine.scene, entity);

		bool destroy = false;

		if (child_count == 0u) {

			GuiButtonStyle style = editor.style.button_style;

			if (entity == editor.selected_entity) {
				style.color = Color::Red();
				style.hot_color = Color::Red();
			}

			if (gui_button(g, name, GuiCoord::Pixel(10.f + xoff), GuiCoord::IPixel(10.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + BUTTON_HEIGHT), style)) {

				editor.selected_entity = entity;
			}

			show_entity_popup(entity, destroy);

			y += BUTTON_HEIGHT + 2.f;
		}
		else {

			GuiContainerStyle style;
			style.color = editor.style.button_style.color;

			if (entity == editor.selected_entity) {
				style.color = Color::Red();
			}

			gui_begin_container(g, GuiCoord::Pixel(10.f + xoff), GuiCoord::IPixel(10.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + BUTTON_HEIGHT), style);

			GuiButtonStyle button_style;
			button_style.color = Color::White(0u);
			button_style.hot_color = Color::White(0u);

			if (gui_button(g, name, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f))) {

				editor.selected_entity = entity;
			}

			show_entity_popup(entity, destroy);

			gui_end_container(g);

			y += BUTTON_HEIGHT + 2.f;

			const Entity* childs;
			child_count = get_entity_childs_count(engine.scene, entity);
			get_entity_childs(engine.scene, entity, &childs);

			foreach(i, child_count) {

				show_entity(childs[i], y, xoff + 15.f);

				get_entity_childs(engine.scene, entity, &childs);
				child_count = get_entity_childs_count(engine.scene, entity);

				if (i < child_count)
					i += get_entity_childs_count(engine.scene, childs[i]);
			}
		}

		if (destroy) {
			destroy_entity(engine.scene, entity);
			if (editor.selected_entity == entity)
				editor.selected_entity = SV_ENTITY_NULL;
		}
	}

	void display_entity_hierarchy()
	{
		GUI* g = editor.gui;

		f32 y = 5.f;
		

		if (gui_begin_window(g, "Hierarchy", editor.style.window_style)) {

			u32 entity_count = get_entity_count(engine.scene);

			foreach(entity_index, entity_count) {

				Entity entity = get_entity_by_index(engine.scene, entity_index);

				show_entity(entity, y, 0.f);
				entity_count = get_entity_count(engine.scene);

				u32 childs = get_entity_childs_count(engine.scene, entity);
				entity_index += childs;
			}

			if (gui_begin_popup(editor.gui, GuiPopupTrigger_Parent, MouseButton_Right, 0x5634c)) {

				f32 y = editor.style.vertical_padding;
				constexpr f32 H = 20.f;

				if (gui_button(editor.gui, "Create Entity", GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
					create_entity(engine.scene);
				}
				y += H + editor.style.vertical_padding;

				if (gui_button(editor.gui, "Create Sprite", GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
					
					Entity e = create_entity(engine.scene, 0, "Sprite");
					add_component<SpriteComponent>(engine.scene, e);
				}
				y += H + editor.style.vertical_padding;

				gui_end_popup(editor.gui);
			}
			
			gui_end_window(g);
		}
	}

	void display_entity_inspector()
	{
		GUI* g = editor.gui;
		Entity selected = editor.selected_entity;

		constexpr f32 NAME_HEIGHT = 40.f;
		constexpr f32 TRANSFORM_HEIGHT = 30.f;
		
		f32 y = 5.f;

		if (gui_begin_window(g, "Inspector", editor.style.window_style)) {

			if (selected != SV_ENTITY_NULL) {

				// Entity name
				{
					const char* entity_name = get_entity_name(engine.scene, selected);

					gui_text(g, entity_name, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + NAME_HEIGHT));
					y += NAME_HEIGHT + editor.style.separator + editor.style.vertical_padding;
				}

				// Entity transform
				{
					// TODO: euler rotation
					Transform trans = get_entity_transform(engine.scene, selected);
					v3_f32 position = trans.getLocalPosition();
					v3_f32 scale = trans.getLocalScale();

					GuiContainerStyle container_style;
					container_style.color = Color::White(0u);

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

						u64 gui_id = 0x3af34 + selected + i * 3u;

						// X
						{
							gui_begin_container(g, 
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 1.5f - INTERN_PADDING), 
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f - INTERN_PADDING), 
								GuiCoord::IPixel(y), 
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT), 
								container_style
							);

							if (gui_button(g, "X", GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), x_style)) {

								if (i == 1u)
									values->x = 1.f;
								else
									values->x = 0.f;
							}

							gui_drag_f32(g, &values->x, 0.1f, gui_id + 0u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						// Y
						{
							gui_begin_container(g,
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f),
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f),
								GuiCoord::IPixel(y),
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT),
								container_style
							);

							if (gui_button(g, "Y", GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), y_style)) {

								if (i == 1u)
									values->y = 1.f;
								else
									values->y = 0.f;
							}

							gui_drag_f32(g, &values->y, 0.1f, gui_id + 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						// Z
						{
							gui_begin_container(g,
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f + INTERN_PADDING),
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 1.5f + INTERN_PADDING),
								GuiCoord::IPixel(y),
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT),
								container_style
							);

							if (gui_button(g, "Z", GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), z_style)) {

								if (i == 1u)
									values->z = 1.f;
								else
									values->z = 0.f;
							}

							gui_drag_f32(g, &values->z, 0.1f, gui_id + 2u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						y += TRANSFORM_HEIGHT + editor.style.vertical_padding;
					}

					y += editor.style.separator;

					trans.setPosition(position);
					trans.setScale(scale);
				}

				// Entity components
				{
					u32 comp_count = get_entity_component_count(engine.scene, selected);

					foreach(comp_index, comp_count) {

						auto [comp_id, comp] = get_component_by_index(engine.scene, selected, comp_index);

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
			camera_controller3D(engine.scene, *get_main_camera(engine.scene), 0.3f);

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

		begin_debug_batch(cmd);

		// Draw selected entity
		if (editor.selected_entity != SV_ENTITY_NULL) {

			Transform trans = get_entity_transform(engine.scene, editor.selected_entity);
			MeshComponent* mesh_comp = get_component<MeshComponent>(engine.scene, editor.selected_entity);

			if (mesh_comp && mesh_comp->mesh.get()) {

				u8 alpha = 5u + u8(f32(sin(timer_now().toSeconds_f64() * 3.5f) + 1.0) * 50.f * 0.5f);
				draw_debug_mesh_wireframe(mesh_comp->mesh.get(), trans.getWorldMatrix(), Color::Red(alpha), cmd);
			}
		}

		// Draw gizmos
		draw_gizmos(engine.scene->offscreen, cmd);

		CameraComponent* cam = get_main_camera(engine.scene);
		Transform trans = get_entity_transform(engine.scene, cam->entity);
		end_debug_batch(engine.scene->offscreen, 0u, camera_view_projection_matrix(trans.getWorldPosition(), trans.getWorldRotation(), *cam), cmd);

		// Draw gui
		gui_draw(editor.gui, engine.scene->offscreen, cmd);
	}

}
