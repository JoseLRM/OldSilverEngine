#include "SilverEngine/core.h"

#ifdef SV_DEV

#include "SilverEngine/renderer.h"
#include "SilverEngine/dev.h"

namespace sv {

	static void control_camera()
	{
		if (dev.camera.projection_type == ProjectionType_Perspective) {

			XMVECTOR rotation = dev.camera.rotation.get_dx();

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

				f32 force = dev.camera.velocity;
				if (input.keys[Key_Shift] == InputState_Hold)
					force *= 3.f;

				dev.camera.position += v3_f32(direction) * input.mouse_wheel * force;
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
			dev.camera.rotation = v4_f32(rotation);
		}
		else {

			InputState button_state = input.mouse_buttons[MouseButton_Center];

			if (button_state != InputState_None) {

				v2_f32 drag = input.mouse_dragged;

				dev.camera.position -= (drag * v2_f32{ dev.camera.width, dev.camera.height }).getVec3();
			}

			if (input.mouse_wheel != 0.f) {

				f32 force = 0.05f;
				if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

				f32 length = dev.camera.getProjectionLength();

				f32 new_length = length - input.mouse_wheel * length * force;
				dev.camera.setProjectionLength(new_length);
			}
		}

		input.unused = false;
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

	enum AssetElementType : u32 {
		AssetElementType_Unknown,
		AssetElementType_Directory,
	};

	struct AssetElement {
		std::string name;
		AssetElementType type;
	};

	struct AssetBrowserInfo {
		std::string filepath;
		std::vector<AssetElement> elements;
		Time last_update = 0.0;
	};

	struct GlobalEditorData {

		GUI* gui;
		EditorStyle style;

		Entity selected_entity = SV_ENTITY_NULL;
		bool camera_controller = false;

		AssetBrowserInfo asset_browser;
		GizmosInfo gizmos;
	};

	GlobalEditorData editor;

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

		gui_push_id(gui, u64("ShowComponent") ^ u64((u64(comp_id) << 32u) + comp->entity));

		bool remove = false;

		gui_begin_container(gui, 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 40.f), container_style);

		gui_text(gui, get_component_name(comp_id), 1u, GuiCoord::Pixel(35.f), GuiCoord::IPixel(10.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), text_style);

		if (gui_begin_popup(gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 2u)) {

			remove = gui_button(gui, "Remove", 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f));

			gui_end_popup(gui);
		}

		bool show = gui_checkbox(gui, 3u, GuiCoord::Pixel(0.f), GuiCoord::Aspect(), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), checkbox_style);

		gui_end_container(gui);

		y += 40.f + editor.style.vertical_padding;

		if (show) {

			GuiTextStyle text_style;
			text_style.text_alignment = TextAlignment_Left;

			gui_push_id(gui, SpriteComponent::ID);

			if (SpriteComponent::ID == comp_id) {

				SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

				gui_text(gui, "Color", 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), text_style);
				gui_button(gui, "Something", 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f));
				y += 30.f + editor.style.vertical_padding;

				gui_text(gui, "Texture", 2, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), text_style);
				gui_button(gui, "Something", 3, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f));

				if (gui_begin_popup(gui, GuiPopupTrigger_LastWidget, MouseButton_Left, 4)) {

					if (gui_button(gui, "New", 0, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f))) {

						const char* filters[] = {
						"all", "*",
						"png", "*.png",
						"jpg", "*.jpg",
						"jpeg", "*.jpeg",
						"gif", "*.gif",
						};

						// TODO: Custom file dialog
						std::string filepath = file_dialog_open(sizeof(filters) / sizeof(const char*) / 2u, filters, "");

						if (filepath.size()) {

							load_asset_from_file(spr.texture, filepath.c_str());
						}
					}

					if (gui_button(gui, "Remove", 1, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(30.f), GuiCoord::IPixel(50.f))) {

						unload_asset(spr.texture);
					}

					gui_end_popup(gui);
				}

				y += 30.f + editor.style.vertical_padding;

			}

			if (MeshComponent::ID == comp_id) {

				MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);

				gui_text(gui, "Mesh", 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), text_style);
				gui_button(gui, "Something", 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f));

				if (gui_begin_popup(gui, GuiPopupTrigger_LastWidget, MouseButton_Left, 2u)) {

					if (gui_button(gui, "New", 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f))) {

						const char* filters[] = {
						"mesh", "*.mesh",
						"all", "*",
						};

						// TODO: Custom file dialog
						std::string filepath = file_dialog_open(sizeof(filters) / sizeof(const char*) / 2u, filters, "");

						if (filepath.size()) {

							load_asset_from_file(m.mesh, filepath.c_str());
						}
					}

					if (gui_button(gui, "Remove", 1u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(30.f), GuiCoord::IPixel(50.f))) {

						unload_asset(m.mesh);
					}

					gui_end_popup(gui);
				}

				y += 30.f + editor.style.vertical_padding;

				gui_text(gui, "Material", 3, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), text_style);
				gui_button(gui, "Something", 4, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f));

				if (gui_begin_popup(gui, GuiPopupTrigger_LastWidget, MouseButton_Left, 5)) {

					if (gui_button(gui, "New", 0, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f))) {

						const char* filters[] = {
						"mat", "*.mat",
						"all", "*",
						};

						// TODO: Custom file dialog
						std::string filepath = file_dialog_open(sizeof(filters) / sizeof(const char*) / 2u, filters, "");

						if (filepath.size()) {

							load_asset_from_file(m.material, filepath.c_str());
						}
					}

					if (gui_button(gui, "Remove", 1, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(30.f), GuiCoord::IPixel(50.f))) {

						unload_asset(m.material);
					}

					gui_end_popup(gui);
				}

				y += 30.f + editor.style.vertical_padding;

			}

			if (CameraComponent::ID == comp_id) {

				CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

				bool main = engine.scene->main_camera == cam.entity;

				if (gui_checkbox(gui, &main, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f))) {

					if (main) engine.scene->main_camera = cam.entity;
					else engine.scene->main_camera = SV_ENTITY_NULL;
				}

				gui_text(gui, "Main Camera", 1u, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 30.f), text_style);
				
				y += 30.f + editor.style.vertical_padding;

			}

			gui_pop_id(gui);
		}

		if (remove) {

			remove_component_by_id(engine.scene, comp->entity, comp_id);
		}

		gui_pop_id(gui);

		y += 30.f + editor.style.vertical_padding;
	}
	
	SV_INLINE static void select_entity()
	{
		v2_f32 mouse = input.mouse_position;

		Ray ray = screen_to_world_ray(mouse, dev.camera.position, dev.camera.rotation, &dev.camera);

		XMVECTOR ray_origin = ray.origin.getDX(1.f);
		XMVECTOR ray_direction = ray.direction.getDX(0.f);

		Entity selected = SV_ENTITY_NULL;
		f32 distance = f32_max;

		// Select meshes
		{
			EntityView<MeshComponent> meshes(engine.scene);

			for (MeshComponent& m : meshes) {

				if (m.mesh.get() == nullptr) continue;

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
		}

		// Select sprites
		{
			EntityView<SpriteComponent> sprites(engine.scene);

			ray.origin = v3_f32(ray_origin);
			ray.direction = v3_f32(ray_direction);

			XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
			XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
			XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
			XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

			XMVECTOR v0;
			XMVECTOR v1;
			XMVECTOR v2;
			XMVECTOR v3;

			for (SpriteComponent& s : sprites) {

				Transform trans = get_entity_transform(engine.scene, s.entity);

				XMMATRIX tm = trans.getWorldMatrix();

				v0 = XMVector3Transform(p0, tm);
				v1 = XMVector3Transform(p1, tm);
				v2 = XMVector3Transform(p2, tm);
				v3 = XMVector3Transform(p3, tm);

				f32 dis = f32_max;

				v3_f32 intersection;

				// TODO: Ray vs Quad intersection

				if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {

					dis = intersection.length();
				}
				else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {

					dis = std::min(intersection.length(), dis);
				}

				if (dis < distance) {
					distance = dis;
					selected = s.entity;
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

			destroy = gui_button(editor.gui, "Destroy", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
			y += H + editor.style.vertical_padding;

			if (gui_button(editor.gui, "Duplicate", 1u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				duplicate_entity(engine.scene, entity);
			}
			y += H + editor.style.vertical_padding;

			if (gui_button(editor.gui, "Create Child", 2u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				create_entity(engine.scene, entity);
			}

			gui_end_popup(editor.gui);
		}
	}

	static void show_entity(Entity entity, f32& y, f32 xoff)
	{
		GUI* g = editor.gui;

		gui_push_id(g, entity);

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

			if (gui_button(g, name, 0u, GuiCoord::Pixel(10.f + xoff), GuiCoord::IPixel(10.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + BUTTON_HEIGHT), style)) {

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

			gui_begin_container(g, 1u, GuiCoord::Pixel(10.f + xoff), GuiCoord::IPixel(10.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + BUTTON_HEIGHT), style);

			GuiButtonStyle button_style;
			button_style.color = Color::White(0u);
			button_style.hot_color = Color::White(0u);

			if (gui_button(g, name, 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f))) {

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

		gui_pop_id(g);
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

				if (gui_button(editor.gui, "Create Entity", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
					create_entity(engine.scene);
				}
				y += H + editor.style.vertical_padding;

				if (gui_button(editor.gui, "Create Sprite", 1u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {

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

					gui_text(g, entity_name, 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + NAME_HEIGHT));
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

						gui_push_id(g, i + 0x23549abf);

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
						{
							gui_begin_container(g, 1u,
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 1.5f - INTERN_PADDING),
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f - INTERN_PADDING),
								GuiCoord::IPixel(y),
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT),
								container_style
							);

							if (gui_button(g, "X", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), x_style)) {

								if (i == 1u)
									values->x = 1.f;
								else
									values->x = 0.f;
							}

							gui_drag_f32(g, &values->x, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						// Y
						{
							gui_begin_container(g, 2u,
								GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f),
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f),
								GuiCoord::IPixel(y),
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT),
								container_style
							);

							if (gui_button(g, "Y", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), y_style)) {

								if (i == 1u)
									values->y = 1.f;
								else
									values->y = 0.f;
							}

							gui_drag_f32(g, &values->y, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						// Z
						{
							gui_begin_container(g, 3u,
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f + INTERN_PADDING),
								GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 1.5f + INTERN_PADDING),
								GuiCoord::IPixel(y),
								GuiCoord::IPixel(y + TRANSFORM_HEIGHT),
								container_style
							);

							if (gui_button(g, "Z", 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), z_style)) {

								if (i == 1u)
									values->z = 1.f;
								else
									values->z = 0.f;
							}

							gui_drag_f32(g, &values->z, 0.1f, 1u, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

							gui_end_container(g);
						}

						gui_pop_id(g);

						y += TRANSFORM_HEIGHT + editor.style.vertical_padding;
					}

					y += editor.style.separator;

					trans.setPosition(position);
					trans.setScale(scale);
				}

				// Entity components
				{
					u32 comp_count = get_entity_component_count(engine.scene, selected);

					gui_push_id(g, "Entity Components");

					foreach(comp_index, comp_count) {

						auto [comp_id, comp] = get_component_by_index(engine.scene, selected, comp_index);

						show_component_info(y, comp_id, comp);
						comp_count = get_entity_component_count(engine.scene, selected);
					}

					gui_pop_id(g);
				}
			}

			if (gui_begin_popup(g, GuiPopupTrigger_Parent, MouseButton_Right, 0xabc2544)) {

				f32 y = 5.f;

				u32 count = get_component_register_count();
				foreach(i, count) {

					CompID comp_id = CompID(i);

					if (get_component_by_id(engine.scene, selected, comp_id))
						continue;

					if (gui_button(g, get_component_name(comp_id), 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 20.f))) {

						add_component_by_id(engine.scene, selected, comp_id);
					}

					y += 20.f + editor.style.vertical_padding;
				}

				gui_end_popup(g);
			}

			gui_end_window(g);
		}
	}

	static void display_asset_browser()
	{
		GUI* gui = editor.gui;

		f32 y = editor.style.vertical_padding;
		constexpr f32 HEIGHT = 30.f;

		bool update_browser = false;
		std::string next_filepath;

		if (gui_begin_window(gui, "Asset Browser", editor.style.window_style)) {

			AssetBrowserInfo& info = editor.asset_browser;

			for (const AssetElement& e : info.elements) {

				if (e.name.size() && e.name.front() != '.') {

					if (gui_button(gui, e.name.c_str(), 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f),
						GuiCoord::IPixel(y), GuiCoord::IPixel(y + HEIGHT), editor.style.button_style)) {

						update_browser = true;

						if (e.type == AssetElementType_Directory) {

							next_filepath = info.filepath + e.name + '/';
						}
					}

					y += editor.style.vertical_padding + HEIGHT;
				}
			}

			// Update per time
			if (!update_browser) {
				Time now = timer_now();

				if (now - info.last_update > 1.0) {
					update_browser = true;
					next_filepath = std::move(info.filepath);
				}
			}

			// Update browser elements
			if (update_browser) {

#if defined(SV_PATH) && defined(SV_RES_PATH)
				while (!std::filesystem::exists(SV_RES_PATH + next_filepath) && next_filepath.size()) {
#else
				while (!std::filesystem::exists(next_filepath) && next_filepath.size()) {
#endif

					next_filepath.pop_back();
					while (next_filepath.size() && next_filepath.back() != '/') {
						next_filepath.pop_back();
					}
				}

				// Clear browser data
				info.elements.clear();

#if defined(SV_PATH) && defined(SV_RES_PATH)
				std::string real_path = SV_RES_PATH + next_filepath;
				for (const auto& entry : std::filesystem::directory_iterator(real_path)) {
#else
				for (const auto& entry : std::filesystem::directory_iterator(next_filepath)) {
#endif

					AssetElement element;

					// Select element type
					if (entry.is_directory()) {
						element.type = AssetElementType_Directory;
					}
					else {
						element.type = AssetElementType_Unknown;
					}

					// Set name
					element.name = parse_string(entry.path().filename().c_str());
					// TODO: Don't know why this string is created with two '\0'
					element.name.pop_back();

					info.elements.emplace_back(std::move(element));
				}

				info.filepath = std::move(next_filepath);
				info.last_update = timer_now();
			}

			gui_end_window(gui);
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
			if (input.keys[Key_F4] == InputState_Pressed) {

				dev.camera.projection_type = (dev.camera.projection_type == ProjectionType_Orthographic) ? ProjectionType_Perspective : ProjectionType_Orthographic;

				if (dev.camera.projection_type == ProjectionType_Orthographic) {

					dev.camera.width = 30.f;
					dev.camera.height = 30.f;
					dev.camera.near = -1000.f;
					dev.camera.far = 1000.f;
					dev.camera.rotation = { 0.f, 0.f, 0.f, 1.f };
				}
				else {

					dev.camera.width = 0.1f;
					dev.camera.height = 0.1f;
					dev.camera.near = 0.03f;
					dev.camera.far = 100000.f;
				}
			}
			if (input.keys[Key_F3] == InputState_Pressed) {
				editor.camera_controller = false;
				dev.console_active = !dev.console_active;
			}
			if (input.keys[Key_F2] == InputState_Pressed) {
				editor.camera_controller = !editor.camera_controller;
				dev.console_active = false;
				dev.debug_draw = true;
			}
			if (input.keys[Key_F1] == InputState_Pressed) {

				editor.camera_controller = false;
				dev.debug_draw = !dev.debug_draw;
			}

			if (input.unused && input.keys[Key_Control]) {

				
			}
		}

		// Adjust camera
		dev.camera.adjust(window_aspect_get(engine.window));

		if (editor.camera_controller) {

			if (engine.scene == nullptr)
				editor.camera_controller = false;
			else {
				control_camera();
			}
		}

		if (!entity_exist(engine.scene, editor.selected_entity)) {
			editor.selected_entity = SV_ENTITY_NULL;
		}

		gui_begin(g, f32(window_width_get(engine.window)), f32(window_height_get(engine.window)));

		if (!editor.camera_controller) {

			if (engine.scene != nullptr) {
				display_entity_hierarchy();
				display_entity_inspector();
				display_asset_browser();
			}
			else {
				// TODO
			}
		}

		gui_end(g);

		if (input.unused && engine.scene) {

			// Entity selection
			if (input.mouse_buttons[MouseButton_Left] == InputState_Released)
				select_entity();
		}
	}

	void draw_editor(CommandList cmd)
	{
		if (engine.scene && dev.debug_draw) {

			begin_debug_batch(cmd);

			// Draw selected entity
			if (editor.selected_entity != SV_ENTITY_NULL) {

				Transform trans = get_entity_transform(engine.scene, editor.selected_entity);
				MeshComponent* mesh_comp = get_component<MeshComponent>(engine.scene, editor.selected_entity);
				SpriteComponent* sprite_comp = get_component<SpriteComponent>(engine.scene, editor.selected_entity);

				if (mesh_comp && mesh_comp->mesh.get()) {

					u8 alpha = 5u + u8(f32(sin(timer_now().toSeconds_f64() * 3.5f) + 1.0) * 50.f * 0.5f);
					draw_debug_mesh_wireframe(mesh_comp->mesh.get(), trans.getWorldMatrix(), Color::Red(alpha), cmd);
				}
				if (sprite_comp) {

					XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
					XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
					XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
					XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

					XMMATRIX tm = trans.getWorldMatrix();

					p0 = XMVector3Transform(p0, tm);
					p1 = XMVector3Transform(p1, tm);
					p2 = XMVector3Transform(p2, tm);
					p3 = XMVector3Transform(p3, tm);

					u8 alpha = 50u + u8(f32(sin(timer_now().toSeconds_f64() * 3.5f) + 1.0) * 200.f * 0.5f);
					Color selection_color = Color::Red(alpha);

					draw_debug_line(v3_f32(p0), v3_f32(p1), selection_color, cmd);
					draw_debug_line(v3_f32(p1), v3_f32(p3), selection_color, cmd);
					draw_debug_line(v3_f32(p3), v3_f32(p2), selection_color, cmd);
					draw_debug_line(v3_f32(p2), v3_f32(p0), selection_color, cmd);
				}
			}

			// Draw 2D grid
			if (dev.camera.projection_type == ProjectionType_Orthographic) {
				draw_debug_orthographic_grip(dev.camera.position.getVec2(), {}, { dev.camera.width, dev.camera.height }, 1.f, Color::White(), cmd);
			}

			// Draw cameras
			{
				EntityView<CameraComponent> cameras(engine.scene);

				for (CameraComponent& cam : cameras) {

					Transform trans = get_entity_transform(engine.scene, cam.entity);

					draw_debug_quad(trans.getWorldMatrix(), Color::Red(), cmd);
				}
			}

			// Draw lights
			{
				EntityView<LightComponent> lights(engine.scene);

				XMMATRIX matrix;
				
				for (LightComponent& l : lights) {

					matrix = XMMatrixRotationQuaternion(XMQuaternionInverse(dev.camera.rotation.get_dx()));
					
					// Move to entity position
					v3_f32 position = get_entity_transform(engine.scene, l.entity).getWorldPosition();
					matrix *= XMMatrixTranslation(position.x, position.y, position.z);
										
					draw_debug_quad(matrix, Color::White(40u), cmd);
				}
			}

			// Draw gizmos
			draw_gizmos(engine.offscreen, cmd);

			end_debug_batch(true, false, camera_view_projection_matrix(dev.camera.position, dev.camera.rotation, dev.camera), cmd);
		}

		// Draw gui
		gui_draw(editor.gui, cmd);
	}

}

#endif
