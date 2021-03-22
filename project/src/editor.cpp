#include "SilverEngine/core.h"

#ifdef SV_DEV

#include "SilverEngine/renderer.h"
#include "SilverEngine/dev.h"

namespace sv {

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

		EditorStyle style;

		Entity selected_entity = SV_ENTITY_NULL;
		bool camera_focus = false;

		AssetBrowserInfo asset_browser;
		GizmosInfo gizmos;
	};

	GlobalEditorData editor;

	/////////////////////////////////////////////// CAMERA ///////////////////////////////////////

	static void control_camera()
	{
		if (!input.unused)
			return;

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
				input.unused = false;
			}

			// Camera rotation
			if (input.mouse_buttons[MouseButton_Center] == InputState_Pressed) {

				editor.camera_focus = true;
			}
			else if (input.mouse_buttons[MouseButton_Center] == InputState_Released) {
				editor.camera_focus = false;
			}

			if (editor.camera_focus && (input.mouse_dragged.x != 0.f || input.mouse_dragged.y != 0.f)) {

				v2_f32 drag = input.mouse_dragged * 3.f;

				// TODO: pitch limit
				XMVECTOR pitch = XMQuaternionRotationAxis(XMVectorSet(1.f, 0.f, 0.f, 0.f), -drag.y);
				XMVECTOR yaw = XMQuaternionRotationAxis(XMVectorSet(0.f, 1.f, 0.f, 0.f), drag.x);

				rotation = XMQuaternionMultiply(pitch, rotation);
				rotation = XMQuaternionMultiply(rotation, yaw);
				rotation = XMQuaternionNormalize(rotation);
				input.unused = false;
			}

			dev.camera.rotation = v4_f32(rotation);
		}
		else {

			InputState button_state = input.mouse_buttons[MouseButton_Center];

			if (button_state == InputState_Pressed) {
				editor.camera_focus = true;
			}
			else if (button_state == InputState_Released) {
				editor.camera_focus = false;
			}

			if (editor.camera_focus) {

				v2_f32 drag = input.mouse_dragged;

				dev.camera.position -= (drag * v2_f32{ dev.camera.width, dev.camera.height }).getVec3();
				input.unused = false;
			}
			else editor.camera_focus = false;

			if (input.mouse_wheel != 0.f) {

				f32 force = 0.05f;
				if (input.keys[Key_Shift] == InputState_Hold) force *= 3.f;

				f32 length = dev.camera.getProjectionLength();

				f32 new_length = length - input.mouse_wheel * length * force;
				dev.camera.setProjectionLength(new_length);

				input.unused = false;
			}
		}
	}

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

		Result res = show ? gui_show_window(dev.gui, args[0]) : gui_hide_window(dev.gui, args[0]);

		if (result_okay(res)) {
			SV_LOG("%s '%s'", show ? "Showing" : "Hiding", args[0]);
		}
		else SV_LOG_ERROR("Window '%s' not found", args[0]);

		return res;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Result initialize_editor()
	{
		svCheck(gui_create(hash_string("EDITOR GUI"), &dev.gui));

		// Register commands
		svCheck(register_command("wnd", command_show_window));

		return Result_Success;
	}

	Result close_editor()
	{
		svCheck(gui_destroy(dev.gui));
		return Result_Success;
	}

	static void show_component_info(f32& y, Entity entity, CompID comp_id, BaseComponent* comp)
	{
		bool remove;

		if (egui_begin_component(entity, comp_id, &remove)) {

			GuiLabelStyle text_style;
			text_style.text_alignment = TextAlignment_Left;

			if (SpriteComponent::ID == comp_id) {

				SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

				egui_comp_color("Color", 0u, &spr.color);
				egui_comp_texture("Texture", 1u, &spr.texture);
				//TODO: egui_comp_drag_v4f32("Coords", 1u, &spr.texcoord, 0.01f);

			}

			if (MeshComponent::ID == comp_id) {

				MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);
			}

			if (CameraComponent::ID == comp_id) {

				CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

				bool main = engine.scene->main_camera == entity;

				if (egui_comp_bool("MainCamera", 0u, &main)) {

					if (main) engine.scene->main_camera = entity;
					else engine.scene->main_camera = SV_ENTITY_NULL;
				}

				f32 dimension = std::min(cam.width, cam.height);

				f32 near_min;
				f32 near_max;
				f32 near_adv;
				f32 far_min;
				f32 far_max;
				f32 far_adv;

				if (cam.projection_type == ProjectionType_Perspective) {
					near_min = 0.001f;
					near_max = f32_max;
					near_adv = 0.01f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}
				else {
					near_min = f32_min;
					near_max = cam.far;
					near_adv = 0.3f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
				}
					
				egui_comp_drag_f32("Near", 1u, &cam.near, near_adv, near_min, near_max);
				egui_comp_drag_f32("Far", 1u, &cam.far, far_adv, far_min, far_max);
				if (egui_comp_drag_f32("Dimension", 1u, &dimension)) {
					cam.width = dimension;
					cam.height = dimension;
				}
			}

			egui_end_component();
		}

		if (remove) {

			remove_component_by_id(engine.scene, entity, comp_id);
		}
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

			for (ComponentView<MeshComponent>& v : meshes) {

				MeshComponent& m = *v.comp;
				Entity entity = v.entity;

				if (editor.selected_entity == entity)
					continue;

				if (m.mesh.get() == nullptr) continue;

				Transform trans = get_entity_transform(engine.scene, entity);

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
							selected = entity;
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

			for (ComponentView<SpriteComponent>& v : sprites) {

				Entity entity = v.entity;

				if (editor.selected_entity == entity)
					continue;

				Transform trans = get_entity_transform(engine.scene, entity);

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
					selected = entity;
				}
			}
		}


		editor.selected_entity = selected;
	}

	static void show_entity_popup(Entity entity, bool& destroy)
	{
		if (gui_begin_popup(dev.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 0x3254fa + u64(entity))) {

			f32 y = editor.style.vertical_padding;
			constexpr f32 H = 20.f;

			destroy = gui_button(dev.gui, "Destroy", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
			y += H + editor.style.vertical_padding;

			if (gui_button(dev.gui, "Duplicate", 1u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				duplicate_entity(engine.scene, entity);
			}
			y += H + editor.style.vertical_padding;

			if (gui_button(dev.gui, "Create Child", 2u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
				create_entity(engine.scene, entity);
			}

			gui_end_popup(dev.gui);
		}
	}

	static void show_entity(Entity entity, f32& y, f32 xoff)
	{
		GUI* g = dev.gui;

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
		GUI* g = dev.gui;

		f32 y = 5.f;


		if (egui_begin_window("Hierarchy")) {

			u32 entity_count = get_entity_count(engine.scene);

			foreach(entity_index, entity_count) {

				Entity entity = get_entity_by_index(engine.scene, entity_index);

				show_entity(entity, y, 0.f);
				entity_count = get_entity_count(engine.scene);

				u32 childs = get_entity_childs_count(engine.scene, entity);
				entity_index += childs;
			}

			if (gui_begin_popup(dev.gui, GuiPopupTrigger_Parent, MouseButton_Right, 0x5634c)) {

				f32 y = editor.style.vertical_padding;
				constexpr f32 H = 20.f;

				if (gui_button(dev.gui, "Create Entity", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {
					create_entity(engine.scene);
				}
				y += H + editor.style.vertical_padding;

				if (gui_button(dev.gui, "Create Sprite", 1u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H))) {

					Entity e = create_entity(engine.scene, 0, "Sprite");
					add_component<SpriteComponent>(engine.scene, e);
				}
				y += H + editor.style.vertical_padding;

				gui_end_popup(dev.gui);
			}

			// MENU

			gui_push_id(g, "MENU");

			if (gui_begin_menu_item(g, "Test", 0u)) {

				if (gui_button(g, "ExitXD", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f))) {
					engine.close_request = true;
				}

				gui_end_menu_item(g);
			}

			if (gui_begin_menu_item(g, "Holaa", 1u)) {



				gui_end_menu_item(g);
			}

			gui_pop_id(g);

			egui_end_window();
		}


		gui_push_id(g, "MENU");

		if (gui_begin_menu_item(g, "Test", 0u)) {

			if (gui_button(g, "Exit", 0u, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f))) {
				engine.close_request = true;
			}

			gui_end_menu_item(g);
		}

		if (gui_begin_menu_item(g, "Holaa", 1u)) {



			gui_end_menu_item(g);
		}

		gui_pop_id(g);
	}

	void display_entity_inspector()
	{
		GUI* g = dev.gui;
		Entity selected = editor.selected_entity;

		f32 y = 5.f;

		if (egui_begin_window("Inspector")) {

			if (selected != SV_ENTITY_NULL) {

				// Entity name
				{
					const char* entity_name = get_entity_name(engine.scene, selected);
					egui_header(entity_name, 0u);
				}

				// Entity transform
				egui_transform(selected);

				// Entity components
				{
					u32 comp_count = get_entity_component_count(engine.scene, selected);

					gui_push_id(g, "Entity Components");

					foreach(comp_index, comp_count) {

						auto [comp_id, comp] = get_component_by_index(engine.scene, selected, comp_index);

						show_component_info(y, selected, comp_id, comp);
						comp_count = get_entity_component_count(engine.scene, selected);
					}

					gui_pop_id(g);
				}

				if (gui_begin_popup(g, GuiPopupTrigger_Parent, MouseButton_Right, 0xabc2544 + selected)) {

					f32 y = 5.f;

					u32 count = get_component_register_count();
					foreach(i, count) {

						CompID comp_id = CompID(i);

						if (get_component_by_id(engine.scene, selected, comp_id))
							continue;

						if (gui_button(g, get_component_name(comp_id), comp_id, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + 20.f))) {

							add_component_by_id(engine.scene, selected, comp_id);
						}

						y += 20.f + editor.style.vertical_padding;
					}

					gui_end_popup(g);
				}
			}

			egui_end_window();
		}
	}

	static void display_asset_browser()
	{
		GUI* gui = dev.gui;

		bool update_browser = false;
		std::string next_filepath;

		if (egui_begin_window("Asset Browser")) {

			AssetBrowserInfo& info = editor.asset_browser;

			gui_begin_grid(gui, u32(info.elements.size()), 150.f, 5.f, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

			foreach (i, info.elements.size()) {

				const AssetElement& e = info.elements[i];

				if (e.name.size() && e.name.front() != '.') {

					gui_begin_grid_element(gui, 69u + i);

					if (gui_button(gui, nullptr, 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f),
						GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), editor.style.button_style)) {

						if (e.type == AssetElementType_Directory) {

							update_browser = true;
							next_filepath = info.filepath + e.name + '/';
						}
					}

					gui_text(gui, e.name.c_str(), 1u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(0.2f));

					gui_end_grid_element(gui);
				}
			}

			gui_end_grid(gui);

			// TEMP
			if (input.unused && input.keys[Key_B] == InputState_Pressed) {

				if (info.filepath.size()) {

					info.filepath.pop_back();

					while (info.filepath.size() && info.filepath.back() != '/') {

						info.filepath.pop_back();
					}
					
					next_filepath = std::move(info.filepath);
					update_browser = true;
				}
				
				input.unused = false;
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

			egui_end_window();
		}
	}

	void update_editor()
	{
		GUI* g = dev.gui;

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
				dev.console_active = !dev.console_active;
			}
			if (input.keys[Key_F1] == InputState_Pressed) {
				dev.debug_draw = !dev.debug_draw;
			}

			if (input.unused && input.keys[Key_Control]) {


			}
		}

		// Adjust camera
		dev.camera.adjust(window_aspect_get(engine.window));

		if (!entity_exist(engine.scene, editor.selected_entity)) {
			editor.selected_entity = SV_ENTITY_NULL;
		}

		if (egui_begin()) {

			if (!editor.camera_focus && engine.scene != nullptr) {
				display_entity_hierarchy();
				display_entity_inspector();
				display_asset_browser();
			}
			else {
				// TODO
			}

			egui_end();
		}

		if (dev.debug_draw && engine.scene) {

			control_camera();
		}

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
			//{
			//	EntityView<CameraComponent> cameras(engine.scene);
			//
			//	for (CameraComponent& cam : cameras) {
			//
			//		Transform trans = get_entity_transform(engine.scene, cam.entity);
			//
			//		draw_debug_quad(trans.getWorldMatrix(), Color::Red(), cmd);
			//	}
			//}

			// Draw lights
			//{
			//	EntityView<LightComponent> lights(engine.scene);
			//
			//	XMMATRIX matrix;
			//
			//	for (LightComponent& l : lights) {
			//
			//		matrix = XMMatrixRotationQuaternion(XMQuaternionInverse(dev.camera.rotation.get_dx()));
			//
			//		// Move to entity position
			//		v3_f32 position = get_entity_transform(engine.scene, l.entity).getWorldPosition();
			//		matrix *= XMMatrixTranslation(position.x, position.y, position.z);
			//
			//		draw_debug_quad(matrix, Color::White(40u), cmd);
			//	}
			//}

			// Draw gizmos
			draw_gizmos(engine.offscreen, cmd);

			end_debug_batch(true, false, dev.camera.view_projection_matrix, cmd);
		}

		// Draw gui
		gui_draw(dev.gui, cmd);
	}

}

#endif
