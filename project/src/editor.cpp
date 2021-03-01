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

		trans.setRotation(v4_f32(rotation));
		trans.setPosition(position);
	}

	struct Editor {

		IGUI* igui;

		Entity selected_entity = SV_ENTITY_NULL;
		bool camera_controller = false;
	};

	Editor editor;

	Result initialize_editor()
	{
		editor.igui = igui_create();

		return Result_Success;
	}

	Result close_editor()
	{
		igui_destroy(editor.igui);
		return Result_Success;
	}

	static void display_window_manager()
	{
		IGUI* g = editor.igui;

		const char* WINDOWS[] = {
			"Entity Hierarchy", "Entity Inspector"
		};
		constexpr u32 WINDOW_COUNT = 2u;

		if (igui_begin_window(g, "Window Manager")) {

			foreach(i, WINDOW_COUNT) {

				if (!igui_is_open_window(g, WINDOWS[i]) &&  igui_button(g, 0x3248B + i, WINDOWS[i])) igui_open_window(g, WINDOWS[i]);
			}

			igui_end_window(g);
		}
	}

	static void display_entity_hierarchy()
	{
		ECS* ecs = engine.scene->ecs;
		IGUI* g = editor.igui;

		if (igui_begin_window(g, "Entity Hierarchy")) {

			u32 entity_count = ecs_entity_count(ecs);

			foreach(i, entity_count) {

				Entity entity = ecs_entity_get(ecs, i);

				const char* name = get_entity_name(ecs, entity);

				if (igui_button(g, u64(entity), name)) {
					editor.selected_entity = entity;
				}
			}

			igui_end_window(g);
		}
	}

	static void display_entity_inspector()
	{
		ECS* ecs = engine.scene->ecs;
		IGUI* g = editor.igui;

		if (igui_begin_window(g, "Entity Inspector")) {

			if (editor.selected_entity != SV_ENTITY_NULL) {

				const char* name = get_entity_name(ecs, editor.selected_entity);

				igui_text(g, 0x43F32, name);

				// TODO: Transform

				// Display components
				u32 component_count = ecs_entity_component_count(ecs, editor.selected_entity);

				foreach(i, component_count) {

					auto[comp_id, comp] = ecs_component_get_by_index(ecs, editor.selected_entity, i);
					
					igui_text(g, 0x743B + i, ecs_component_name(comp_id));
				}
			}

			igui_end_window(g);
		}
	}

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
		XMVECTOR ray_direction = ray.direction.getDX(1.f);
		
		Entity selected = SV_ENTITY_NULL;
		f32 distance = f32_max;
		
		EntityView<MeshComponent> meshes(ecs);
		
		for (MeshComponent& m : meshes) {
			
			Transform trans = ecs_entity_transform_get(ecs, m.entity);
			
			XMMATRIX itm = XMMatrixInverse(0, trans.getWorldMatrix());
			
			v3_f32 o = v3_f32(XMVector4Transform(ray_origin, itm));
			v3_f32 d = v3_f32(XMVector4Transform(ray_direction, itm));
			
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
				
				if (intersect_ray_vs_traingle(o, d, p0, p1, p2, intersection)) {
					
					f32 dis = intersection.length();
					if (dis < distance) {
						distance = dis;
						selected = m.entity;
					}
				}
			}
		}
		
		if (selected != SV_ENTITY_NULL) {

			editor.selected_entity = selected;
		}
	}


	void update_editor()
	{
		IGUI* g = editor.igui;

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

		igui_begin(g);
		
		if (!editor.camera_controller) {
			
			if (input.keys[Key_F1]) display_window_manager();
			display_entity_inspector();
			display_entity_hierarchy();

			// Entity selection
			if (input.mouse_button[MouseButton_Left] == InputState_Released)
				select_entity();
		}

		igui_end(g, f32(window_width_get(engine.window)), f32(window_height_get(engine.window)));
	}

	void draw_editor()
	{
		CommandList cmd = graphics_commandlist_get();

		igui_render(editor.igui, engine.scene->offscreen, cmd);
	}

}
