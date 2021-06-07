#if SV_EDITOR

#include "core/renderer.h"
#include "debug/console.h"
#include "core/event_system.h"

namespace sv {

    enum GizmosTransformMode : u32 {
		GizmosTransformMode_None,
		GizmosTransformMode_Position
    };

    constexpr f32 GIZMOS_SIZE = 0.1f;

    enum GizmosObject : u32 {
		GizmosObject_None,
		GizmosObject_AxisX,
		GizmosObject_AxisY,
		GizmosObject_AxisZ,
    };

    struct GizmosInfo {

		GizmosTransformMode mode = GizmosTransformMode_Position;

		GizmosObject object = GizmosObject_None;
		bool focus = false;
	
		v3_f32 start_offset;
		f32 axis_size;
		f32 selection_size;
    };

    enum AssetElementType : u32 {
		AssetElementType_Unknown,
		AssetElementType_Texture,
		AssetElementType_Mesh,
		AssetElementType_Material,
		AssetElementType_SpriteSheet,
		AssetElementType_Directory,
    };

    struct AssetElement {
		char name[FILENAME_SIZE + 1];
		AssetElementType type;
    };

    struct AssetBrowserInfo {
		char filepath[FILEPATH_SIZE + 1] = {};
		List<AssetElement> elements;
		f64 last_update = 0.0;
    };

    struct EditorEntity {
		Entity handle;
    };

    struct HierarchyFolder {
		char path[FILEPATH_SIZE + 1];
		List<EditorEntity> entities;
		List<HierarchyFolder> folders;
		bool show;
    };
    
    struct HierarchyData {
		HierarchyFolder root;
    };

	enum SpriteSheetEditorState : u32 {
		SpriteSheetEditorState_Main,
		SpriteSheetEditorState_SpriteList,
		SpriteSheetEditorState_ModifySprite,
		SpriteSheetEditorState_NewSprite,
		SpriteSheetEditorState_AnimationList,
		SpriteSheetEditorState_NewAnimation,
		SpriteSheetEditorState_ModifyAnimation,
		SpriteSheetEditorState_AddSprite,
	};

    struct SpriteSheetEditorData {
		SpriteSheetAsset current_sprite_sheet;
		SpriteSheetEditorState state = SpriteSheetEditorState_Main;
		SpriteSheetEditorState next_state = SpriteSheetEditorState_Main;
		Sprite temp_sprite;
		SpriteAnimation temp_anim;
		u32 modifying_id = 0u;
		f32 simulation_time = 0.f;
    };

    struct GlobalEditorData {

		List<Entity> selected_entities;
		bool camera_focus = false;

		AssetBrowserInfo asset_browser;
		GizmosInfo gizmos;

		HierarchyData hierarchy;
	
		TextureAsset image;
		static constexpr v4_f32 TEXCOORD_FOLDER = { 0.f, 0.f, 0.1f, 0.1f };
		static constexpr v4_f32 TEXCOORD_LIGHT_PROBE = { 0.7f, 0.7f, 1.f, 1.f };

		SpriteSheetEditorData sprite_sheet_editor_data;

		char next_scene_name[SCENENAME_SIZE + 1u] = "";
    };

    GlobalEditorData editor;

	SV_AUX bool is_entity_selected(Entity entity)
	{
		for (Entity e : editor.selected_entities)
			if (e == entity) return true;
		return false;
	}

    SV_INTERNAL void show_reset_popup()
    {
		if (dev.engine_state == EngineState_ProjectManagement || dev.engine_state == EngineState_None) {
			return;
		}
	
		if (dev.engine_state != EngineState_ProjectManagement && show_dialog_yesno("Code reloaded!!", "Do you want to reset the game?")) {

			_engine_reset_game();
			dev.next_engine_state = EngineState_Edit;
		}
    }

    /////////////////////////////////////////////// KEY SHORTCUTS //////////////////////////////////////

    SV_AUX void update_key_shortcuts()
    {
		// Fullscreen
		if (input.keys[Key_F10] == InputState_Pressed) {
			
			os_window_set_fullscreen(os_window_state() != WindowState_Fullscreen);
		}

		// Debug rendering
		if (input.keys[Key_F1] == InputState_Pressed) {
			dev.debug_draw = !dev.debug_draw;
		}

		// Console show - hide
		if (input.keys[Key_F3] == InputState_Pressed) {

			if (console_is_open())
				console_close();
			else console_open();
		}

		// Change debug camera projection
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
	
		// Compile game code
		if (input.keys[Key_F5] == InputState_Pressed)
			_os_compile_gamecode();
	
		if (dev.engine_state != EngineState_Play) {

			// Close engine or play game
			if (input.keys[Key_F11] == InputState_Pressed) {

				if (input.keys[Key_Control] && input.keys[Key_Alt])
					engine.close_request = true;
				else
					dev.next_engine_state = EngineState_Play;
			}
	    
			if (input.unused && input.keys[Key_Control]) {

				// Undo
				if (input.keys[Key_Z] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.undo_action(nullptr);
					stack.unlock();
				}

				// Do
				if (input.keys[Key_Y] == InputState_Pressed) {

					DoUndoStack& stack = dev.do_undo_stack;
					stack.lock();
					stack.do_action(nullptr);
					stack.unlock();
				}

				// Duplicate
				if (input.keys[Key_D] == InputState_Pressed) {

					for (Entity e : editor.selected_entities) {
						duplicate_entity(e);
					}
				}
			}
		}
		else {

			// Editor mode
			if (input.keys[Key_F11] == InputState_Pressed) {
				dev.next_engine_state = EngineState_Edit;
			}
		}
    }

    /////////////////////////////////////////////// DO UNDO ACTIONS ////////////////////////////////////
    
    typedef void(*ConstructEntityActionFn)(Entity entity);
    
    struct EntityCreate_Action {
		Entity entity;
		Entity parent;
		ConstructEntityActionFn construct_entity;
    };
    
    SV_INTERNAL void do_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;

		Entity parent = data.parent;
		if (!entity_exist(parent))
			parent = SV_ENTITY_NULL;

		const char* name = (const char*)(pdata) + sizeof(data);
		if (*name == '\0')
			name = nullptr;
	
		data.entity = create_entity(parent, name);

		if (preturn) {
			memcpy(preturn, &data.entity, sizeof(Entity));
		}

		if (data.construct_entity) {
			data.construct_entity(data.entity);
		}
    }
    SV_INTERNAL void undo_entity_create(void* pdata, void* preturn) {

		EntityCreate_Action& data = *(EntityCreate_Action*)pdata;
		if (entity_exist(data.entity))
			destroy_entity(data.entity);
    }
    
    SV_INTERNAL void construct_entity_sprite(Entity entity) {
		add_component<SpriteComponent>(entity);
    }
    SV_INTERNAL void construct_entity_2D_camera(Entity entity) {
		add_component<CameraComponent>(entity);
    }
    
    SV_INTERNAL Entity editor_create_entity(Entity parent = SV_ENTITY_NULL, const char* name = nullptr, ConstructEntityActionFn construct_entity = nullptr)
    {
		DoUndoStack& stack = dev.do_undo_stack;
		stack.lock();

		stack.push_action(do_entity_create, undo_entity_create);
	
		EntityCreate_Action data;
		data.entity = SV_ENTITY_NULL;
		data.parent = parent;
		data.construct_entity = construct_entity;
	
		stack.push_data(&data, sizeof(data));

		size_t name_size = name ? strlen(name) : 0u;
		if (name_size)
			stack.push_data(name, name_size + 1u);
		else {
			char c = '\0';
			stack.push_data(&c, 1u);
		}

		Entity res;
		stack.do_action(&res);
	
		stack.unlock();

		return res;
    }

    /////////////////////////////////////////////// CAMERA ///////////////////////////////////////

    static void control_camera()
    {
		if (dev.postprocessing) {

			CameraComponent* cam = get_main_camera();

			if (cam) {
				dev.camera.bloom = cam->bloom;
			}
			else dev.postprocessing = false;
		}
		else {
			dev.camera.bloom = {};
		}
	
		if (!input.unused)
			return;

		if (dev.camera.projection_type == ProjectionType_Perspective) {

			XMVECTOR rotation = vec4_to_dx(dev.camera.rotation);

			XMVECTOR direction;
			XMMATRIX rotation_matrix;

			// Rotation matrix
			rotation_matrix = XMMatrixRotationQuaternion(rotation);

			// Camera direction
			direction = XMVectorSet(0.f, 0.f, 1.f, 0.f);
			direction = XMVector3Transform(direction, rotation_matrix);

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

				dev.camera.position -= vec2_to_vec3((drag * v2_f32{ dev.camera.width, dev.camera.height }));
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

    SV_AUX v2_f32 world_point_to_screen(const XMMATRIX& vpm, const v3_f32& position)
    {
		XMVECTOR p = vec3_to_dx(position, 1.f);

		p = XMVector4Transform(p, vpm);
	
		f32 d = XMVectorGetW(p);
		p = XMVectorDivide(p, XMVectorSet(d, d, d, 1.f));
	
		return v2_f32(p);
    }

    SV_AUX v2_f32 project_line_onto_line(v2_f32 origin, v2_f32 d0, v2_f32 d1)
    {
		return origin + (vec2_dot(d1, d0) / vec2_dot(d1, d1)) * d1;
    }
    SV_AUX v3_f32 project_line_onto_line(v3_f32 origin, v3_f32 d0, v3_f32 d1)
    {
		return origin + (vec3_dot(d1, d0) / vec3_dot(d1, d1)) * d1;
    }

    SV_AUX void intersect_line_vs_plane(v3_f32 line_point, v3_f32 line_direction, v3_f32 plane_point, v3_f32 plane_normal, v3_f32& intersection, f32& distance)
    {
		distance = vec3_dot(plane_point - line_point, plane_normal) / vec3_dot(line_direction, plane_normal);
		intersection = line_point + distance * line_direction;
    }

    SV_AUX void closest_points_between_two_lines(v3_f32 l0_pos, v3_f32 l0_dir, v3_f32 l1_pos, v3_f32 l1_dir, v3_f32& l0_res, v3_f32& l1_res)
    {
		v3_f32 u = l0_dir;
		v3_f32 v = l1_dir;
		v3_f32 w = l0_pos - l1_pos;
		f32 a = vec3_dot(u, u);         // always >= 0
		f32 b = vec3_dot(u, v);
		f32 c = vec3_dot(v, v);         // always >= 0
		f32 d = vec3_dot(u, w);
		f32 e = vec3_dot(v, w);
		f32 D = a*c - b*b;        // always >= 0
		f32 sc, tc;

		constexpr f32 SMALL_NUM = 0.00000001f;
	
		// compute the line parameters of the two closest points
		if (D < SMALL_NUM) {          // the lines are almost parallel
			sc = 0.0;
			tc = (b>c ? d/b : e/c);    // use the largest denominator
		}
		else {
			sc = (b*e - c*d) / D;
			tc = (a*e - b*d) / D;
		}

		l0_res = l0_pos + sc * u;
		l1_res = l1_pos + tc * v;
    }

    SV_AUX f32 relative_scalar(f32 value, v3_f32 position)
    {
		if (dev.camera.projection_type == ProjectionType_Perspective) {
		
			XMVECTOR pos = vec3_to_dx(position, 1.f);
			pos = XMVector4Transform(pos, dev.camera.view_matrix);
	    
			f32 distance = XMVectorGetZ(pos);

			f32 w_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.width);
			f32 w_near_prop = value / w_near_distance;

			w_near_distance = math_sqrt(distance * distance + dev.camera.width);

			f32 h_near_distance = math_sqrt(dev.camera.near * dev.camera.near + dev.camera.height);
			f32 h_near_prop = value / h_near_distance;

			h_near_distance = math_sqrt(distance * distance + dev.camera.height);

			return (w_near_distance * w_near_prop + h_near_distance * h_near_prop) * 0.5f;
		}
		else {

			return value * dev.camera.getProjectionLength();
		}
    }

	SV_AUX v3_f32 compute_selected_entities_position()
	{
		v3_f32 position;
		
		f32 mult = 1.f / f32(editor.selected_entities.size());
		for (Entity e : editor.selected_entities) {

			position += get_entity_world_position(e) * mult;
		}

		return position;
	}

    SV_INTERNAL void update_gizmos()
    {
		GizmosInfo& info = editor.gizmos;

		if (!input.unused || editor.selected_entities.empty()) {
			info.focus = false;
			return;
		}

		v3_f32 position = compute_selected_entities_position();
		
		// Compute axis size and selection size
		{
			info.axis_size = relative_scalar(GIZMOS_SIZE, position);
			info.selection_size = relative_scalar(0.008f, position);
		}

		if (!info.focus) {

			info.object = GizmosObject_None;
	    
			v2_f32 mouse_position = input.mouse_position * 2.f;

			switch (info.mode)
			{

			case GizmosTransformMode_Position:
			{
				GizmosObject obj[] = {
					GizmosObject_AxisX,
					GizmosObject_AxisY,
					GizmosObject_AxisZ
				};
		
				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);
		    
					v3_f32 axis[3u];
					axis[0] = v3_f32(info.axis_size, 0.f, 0.f);
					axis[1] = v3_f32(0.f, info.axis_size, 0.f);
					axis[2] = v3_f32(0.f, 0.f, info.axis_size);

					// TODO: Sort axis update by the distance to the camera
					foreach(i, 3u) {

						v3_f32 p0, p1;
						closest_points_between_two_lines(ray.origin, ray.direction, position, axis[i], p0, p1);

						f32 dist0 = vec3_length(p0 - p1);
			
						f32 dist1 = -1.f;

						switch(i) {
			    
						case 0:
							dist1 = p1.x - position.x;
							break;
			    
						case 1:
							dist1 = p1.y - position.y;
							break;
			    
						case 2:
							dist1 = p1.z - position.z;
							break;
						}

						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];
							info.start_offset = p1 - position;
						}
					}
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);
					v2_f32 to_mouse = mouse - vec3_to_vec2(position);
			
					foreach (i, 2u) {

						v2_f32 axis_direction = ((i == 0) ? v2_f32::right() : v2_f32::up()) * info.axis_size;

						v2_f32 projection = project_line_onto_line(vec3_to_vec2(position), to_mouse, axis_direction);
						f32 dist0 = vec2_length(mouse - projection);
						f32 dist1 = ((i == 0) ? (projection.x - position.x) : (projection.y - position.y));
			
						if (dist0 < info.selection_size && dist1 > 0.f && dist1 <= info.axis_size) {
							info.object = obj[i];

							if (i == 0) info.start_offset = { dist1, 0.f, 0.f };
							else info.start_offset = { 0.f, dist1, 0.f };
						}
					}
				}
			}
			break;

			}

			if (info.object != GizmosObject_None && input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		
				info.focus = true;
				input.unused = false;
			}
		}

		else {

			input.unused = false;

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

				info.focus = false;
			}
			else {

				v3_f32 init_pos = position;

				if (dev.camera.projection_type == ProjectionType_Perspective) {

					Ray ray = screen_to_world_ray(input.mouse_position, dev.camera.position, dev.camera.rotation, &dev.camera);

					v3_f32 axis;
		    
					switch (info.object) {

					case GizmosObject_AxisX:
						axis = v3_f32::right();
						break;

					case GizmosObject_AxisY:
						axis = v3_f32::up();
						break;

					case GizmosObject_AxisZ:
						axis = v3_f32::forward();
						break;
		    
					}

					v3_f32 p0, p1;
					closest_points_between_two_lines(ray.origin, ray.direction, position, axis, p0, p1);
					position = p1 - info.start_offset;
				}
				else {

					v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + vec3_to_vec2(dev.camera.position);
		
					if (info.object == GizmosObject_AxisX)
						position.x = mouse.x - info.start_offset.x;
					else if (info.object == GizmosObject_AxisY)
						position.y = mouse.y - info.start_offset.y;
				}

				// Set new position
				v3_f32 move = position - init_pos;
				
				for (Entity e : editor.selected_entities) {

					v3_f32& pos = *get_entity_position_ptr(e);

					Entity parent = get_entity_parent(e);

					if (entity_exist(parent)) {

						XMMATRIX wm = get_entity_world_matrix(parent);

						wm = XMMatrixInverse(NULL, wm);
						
						pos += (v3_f32)XMVector4Transform(vec3_to_dx(move), wm);
					}
					else pos += move;
				}
			}
		}
    }

    SV_INTERNAL void draw_gizmos(GPUImage* offscreen, CommandList cmd)
    {
		if (editor.selected_entities.empty()) return;

		GizmosInfo& info = editor.gizmos;

		v3_f32 position = compute_selected_entities_position();

		f32 axis_size = info.axis_size;

		switch (info.mode)
		{

		case GizmosTransformMode_Position:
		{
			Color color = ((info.object == GizmosObject_AxisX) ? (info.focus ? Color::Silver() : Color{255u, 50u, 50u, 255u}) : Color::Red());
			imrend_draw_line(position, position + v3_f32::right() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
			imrend_draw_line(position, position + v3_f32::up() * axis_size, color, cmd);

			color = ((info.object == GizmosObject_AxisZ) ? (info.focus ? Color::Silver() : Color{50u, 50u, 255u, 255u}) : Color::Blue());
			imrend_draw_line(position, position + v3_f32::forward() * axis_size, color, cmd);
		}
		break;

		}
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void on_entity_create(EntityCreateEvent* event)
    {
		if (get_entity_parent(event->entity) != SV_ENTITY_NULL) return;
		EditorEntity& e = editor.hierarchy.root.entities.emplace_back();
		e.handle = event->entity;
    }

    SV_INTERNAL void remove_entity_from_hierarchy_folder(HierarchyFolder& folder, Entity entity)
    {
		foreach(i, folder.entities.size()) {

			if (folder.entities[i].handle == entity) {
				folder.entities.erase(i);
				return;
			}
		}

		for (HierarchyFolder& f : folder.folders) {
			remove_entity_from_hierarchy_folder(f, entity);
		}
    }
    
    void on_entity_destroy(EntityDestroyEvent* event)
    {
		if (get_entity_parent(event->entity) != SV_ENTITY_NULL) return;
	
		remove_entity_from_hierarchy_folder(editor.hierarchy.root, event->entity);
    }
    
    bool _editor_initialize()
    {
		SV_CHECK(_gui_initialize());

		load_asset_from_file(editor.image, "$system/images/editor.png");

		event_register("on_entity_create", on_entity_create, 0u);
		event_register("on_entity_destroy", on_entity_destroy, 0u);
		event_register("user_callbacks_initialize", show_reset_popup, 0u);

		dev.engine_state = EngineState_ProjectManagement;
		_gui_load("PROJECT");
		engine.update_scene = false;

		return true;
    }

    bool _editor_close()
    {
		SV_CHECK(_gui_close());
		return true;
    }

    SV_AUX void gui_material(Material& mat)
    {
		gui_push_id("Material");
	
		u64 id = 0u;
	
		gui_text("Pipeline Settings", id++);
		gui_separator(5.f);
	
		gui_checkbox("Transparent", mat.transparent, id++);

		gui_text("Values", id++);
		gui_separator(5.f);

		gui_drag_color("Ambient Color", mat.ambient_color, id++);
		gui_drag_color("Diffuse Color", mat.diffuse_color, id++);
		gui_drag_color("Specular Color", mat.specular_color, id++);
		gui_drag_color("Emissive Color", mat.emissive_color, id++);
		gui_drag_f32("Shininess", mat.shininess, 0.01f, 0.f, 300.f, id++);
	
		gui_pop_id();
    }
    
    SV_INTERNAL void show_component_info(Entity entity, CompID comp_id, BaseComponent* comp)
    {
		bool remove;

		if (egui_begin_component(entity, comp_id, &remove)) {

			if (SpriteComponent::ID == comp_id) {

				SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

				gui_sprite("Sprite", spr.sprite_sheet, spr.sprite_id, 0u);
				gui_drag_color("Color", spr.color, 1u);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 3u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 4u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}

			if (AnimatedSpriteComponent::ID == comp_id) {

				AnimatedSpriteComponent& spr = *reinterpret_cast<AnimatedSpriteComponent*>(comp);

				SpriteSheet* sheet = spr.sprite_sheet.get();
				u32 max_index = 0u;

				if (sheet && sheet->sprite_animations.exists(spr.animation_id))
					max_index = sheet->sprite_animations[spr.animation_id].frames;

				gui_sprite_animation("Animation", spr.sprite_sheet, spr.animation_id, 0u);
				gui_drag_color("Color", spr.color, 1u);
				gui_drag_u32("Index", spr.index, 1u, 0u, max_index, 2u);
				gui_drag_f32("Time Mult", spr.time_mult, 0.01f, 0.f, f32_max, 6u);

				bool xflip = spr.flags & SpriteComponentFlag_XFlip;
				bool yflip = spr.flags & SpriteComponentFlag_YFlip;

				if (gui_checkbox("XFlip", xflip, 9u)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
				if (gui_checkbox("YFlip", yflip, 10u)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
			}
	    
			if (MeshComponent::ID == comp_id) {

				MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);

				egui_comp_mesh("Mesh", 0u, &m.mesh);
				//egui_comp_material("Material", 1u, &m.material);
				if (m.material.get())
					gui_material(*m.material.get());
			}

			if (CameraComponent::ID == comp_id) {

				CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

				SceneData& d = *get_scene_data();
				bool main = d.main_camera == entity;

				if (gui_checkbox("MainCamera", main, 0u)) {

					if (main) d.main_camera = entity;
					else d.main_camera = SV_ENTITY_NULL;
				}

				f32 dimension = SV_MIN(cam.width, cam.height);

				f32 near_min;
				f32 near_max;
				f32 near_adv;
				f32 far_min;
				f32 far_max;
				f32 far_adv;
				const char* preview;

				if (cam.projection_type == ProjectionType_Perspective) {
					near_min = 0.001f;
					near_max = f32_max;
					near_adv = 0.01f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
					preview = "Orthographic";
				}
				else {
					near_min = f32_min;
					near_max = cam.far;
					near_adv = 0.3f;
					far_min = cam.near;
					far_max = f32_max;
					far_adv = 0.3f;
					preview = "Perspective";
				}

				if (gui_begin_combobox(preview, 0u)) {

					if (gui_button("Orthographic")) {
						cam.projection_type = ProjectionType_Orthographic;
					}
					if (gui_button("Perspective")) {
						cam.projection_type = ProjectionType_Perspective;
					}

					gui_end_combobox();
				}

				gui_drag_f32("Near", cam.near, near_adv, near_min, near_max);
				gui_drag_f32("Far", cam.far, far_adv, far_min, far_max);
				if (gui_drag_f32("Dimension", dimension, 0.01f, 0.01f, f32_max)) {
					cam.width = dimension;
					cam.height = dimension;
				}

				gui_checkbox("Bloom", cam.bloom.active);
				if (cam.bloom.active) {

					gui_drag_f32("Threshold", cam.bloom.threshold, 0.001f, 0.f, 1.f);
					gui_drag_f32("Intensity", cam.bloom.intensity, 0.001f, 0.f, 1.f);
				}
			}

			if (LightComponent::ID == comp_id) {

				LightComponent& l = *reinterpret_cast<LightComponent*>(comp);

				bool direction = l.light_type == LightType_Direction;
				if (gui_checkbox("Directional", direction, 10u)) {
					if (direction) l.light_type = LightType_Direction;
					else l.light_type = LightType_Point;
				}

				gui_drag_color("Color", l.color);
				gui_drag_f32("Intensity", l.intensity, 0.05f, 0.0f, f32_max);
				gui_drag_f32("Range", l.range, 0.1f, 0.0f, f32_max);
				gui_drag_f32("Smoothness", l.smoothness, 0.005f, 0.0f, 1.f);

				gui_separator(5.f);

				gui_checkbox("Shadow Mapping", l.shadow_mapping_enabled);

				if (l.shadow_mapping_enabled) {
					// TODO
				}
			}

			if (BodyComponent::ID == comp_id) {

				BodyComponent& b = *reinterpret_cast<BodyComponent*>(comp);
				const SceneData& scene = *get_scene_data();

				// TODO: Use combobox
				bool static_ = b.body_type == BodyType_Static;
				bool dynamic = b.body_type == BodyType_Dynamic;
				bool projectile = b.body_type == BodyType_Projectile;

				if (gui_checkbox("Static", static_)) {
					b.body_type = static_ ? BodyType_Static : BodyType_Dynamic;
				}
				if (gui_checkbox("Dynamic", dynamic)) {
					b.body_type = dynamic ? BodyType_Dynamic : BodyType_Static;
				}
				if (gui_checkbox("Projectile", projectile)) {
					b.body_type = projectile ? BodyType_Projectile : BodyType_Static;
				}

				if (scene.physics.in_3D) {
					gui_drag_v3_f32("Size", b.size, 0.005f, 0.f);
					gui_drag_v3_f32("Offset", b.offset, 0.005f);
					gui_drag_v3_f32("Velocity", b.vel, 0.01f);
				}
				else {
					v2_f32 v2;

					v2 = vec3_to_vec2(b.size);
					if (gui_drag_v2_f32("Size", v2, 0.005f, 0.f))
						b.size = vec2_to_vec3(v2);

					v2 = vec3_to_vec2(b.offset);
					if (gui_drag_v2_f32("Offset", v2, 0.005f))
						b.offset = vec2_to_vec3(v2);

					v2 = vec3_to_vec2(b.vel);
					if (gui_drag_v2_f32("Velocity", v2, 0.01f))
						b.vel = vec2_to_vec3(v2);
				}
				
				gui_drag_f32("Mass", b.mass, 0.1f, 0.0f);
				gui_drag_f32("Friction", b.friction, 0.001f, 0.0f, 1.f);
				gui_drag_f32("Bounciness", b.bounciness, 0.005f, 0.0f, 1.f);

				bool is_trigger = b.flags & BodyComponentFlag_Trigger;

				if (gui_checkbox("Trigger", is_trigger)) {
					if (is_trigger)
						b.flags |= BodyComponentFlag_Trigger;
					else
						b.flags = b.flags & (~BodyComponentFlag_Trigger);
				}
			}

			ShowComponentEvent e;
			e.comp_id = comp_id;
			e.comp = comp;

			gui_push_id("User");
			event_dispatch("show_component_info", &e);
			gui_pop_id();

			egui_end_component();
		}

		if (remove) {

			remove_component_by_id(entity, comp_id);
		}
    }

    SV_AUX void select_entity()
    {
		v2_f32 mouse = input.mouse_position;

		Ray ray = screen_to_world_ray(mouse, dev.camera.position, dev.camera.rotation, &dev.camera);

		XMVECTOR ray_origin = vec3_to_dx(ray.origin, 1.f);
		XMVECTOR ray_direction = vec3_to_dx(ray.direction, 0.f);

		Entity selected = SV_ENTITY_NULL;
		f32 distance = f32_max;

		XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
		XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

		XMVECTOR v0;
		XMVECTOR v1;
		XMVECTOR v2;
		XMVECTOR v3;

		// Select lights
		{
			for_each_comp<LightComponent>([&] (Entity entity, LightComponent& l)
				{
					if (is_entity_selected(entity))
						return true;

					v3_f32 pos = get_entity_world_position(entity);

					f32 min_scale = relative_scalar(0.02f, pos);
					f32 scale = SV_MAX(min_scale, 1.f);

					XMMATRIX tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);

					v0 = XMVector3Transform(p0, tm);
					v1 = XMVector3Transform(p1, tm);
					v2 = XMVector3Transform(p2, tm);
					v3 = XMVector3Transform(p3, tm);

					f32 dis = f32_max;

					v3_f32 intersection;

					// TODO: Ray vs Quad intersection

					if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {

						dis = vec3_length(intersection);
					}
					else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
			
						dis = SV_MIN(vec3_length(intersection), dis);
					}

					if (dis < distance) {
						distance = dis;
						selected = entity;
					}
					return true;
				});
		}

		if (selected == SV_ENTITY_NULL) {

			// Select meshes
			{
				for_each_comp<MeshComponent>([&] (Entity entity, MeshComponent& m)
					{
						if (is_entity_selected(entity))
							return true;

						if (m.mesh.get() == nullptr) return true;

						XMMATRIX wm = get_entity_world_matrix(entity);

						XMMATRIX itm = XMMatrixInverse(0, wm);

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

								f32 dis = vec3_length(intersection);
								if (dis < distance) {
									distance = dis;
									selected = entity;
								}
							}
						}

						return true;
					});
			}

			// Select sprites
			{
				ray.origin = v3_f32(ray_origin);
				ray.direction = v3_f32(ray_direction);

				for_each_comp<SpriteComponent>([&] (Entity entity, SpriteComponent&)
					{
						if (is_entity_selected(entity))
							return true;

						XMMATRIX tm = get_entity_world_matrix(entity);

						v0 = XMVector3Transform(p0, tm);
						v1 = XMVector3Transform(p1, tm);
						v2 = XMVector3Transform(p2, tm);
						v3 = XMVector3Transform(p3, tm);

						f32 dis = f32_max;

						v3_f32 intersection;

						// TODO: Ray vs Quad intersection

						if (intersect_ray_vs_traingle(ray, v3_f32(v0), v3_f32(v1), v3_f32(v2), intersection)) {

							dis = vec3_length(intersection);
						}
						else if (intersect_ray_vs_traingle(ray, v3_f32(v1), v3_f32(v3), v3_f32(v2), intersection)) {
			
							dis = SV_MIN(vec3_length(intersection), dis);
						}

						if (dis < distance) {
							distance = dis;
							selected = entity;
						}

						return true;
					});
			}
		}

		if (!input.keys[Key_Shift])
			editor.selected_entities.reset();

		if (selected != SV_ENTITY_NULL)
			editor.selected_entities.push_back(selected);
    }
    
    static void show_entity_popup(Entity entity, bool& destroy)
    {
		if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

			f32 y = 0.f;
			constexpr f32 H = 20.f;
	    
			destroy = gui_button("Destroy", 0u);
			y += H;
	    
			if (gui_button("Duplicate", 1u)) {
				duplicate_entity(entity);
			}
			y += H;
	    
			if (gui_button("Create Child", 2u)) {
				editor_create_entity(entity);
			}

			gui_end_popup();
		}
    }

    SV_INTERNAL void show_entity(Entity entity)
    {
		gui_push_id(entity);

		const char* name = get_entity_name(entity);

		u32 child_count = get_entity_childs_count(entity);

		bool destroy = false;

		if (child_count == 0u) {
	    
			if (gui_button(name, 0u)) {

				editor.selected_entities.reset();
				editor.selected_entities.push_back(entity);
			}

			show_entity_popup(entity, destroy);
		}
		else {
	    
			if (gui_button(name, 0u)) {
		
				editor.selected_entities.reset();
				editor.selected_entities.push_back(entity);
			}

			show_entity_popup(entity, destroy);

			const Entity* childs;
			child_count = get_entity_childs_count(entity);
			get_entity_childs(entity, &childs);

			foreach(i, child_count) {

				show_entity(childs[i]);

				get_entity_childs(entity, &childs);
				child_count = get_entity_childs_count(entity);

				if (i < child_count)
					i += get_entity_childs_count(childs[i]);
			}
		}

		if (destroy) {
			destroy_entity(entity);
			
			foreach(i, editor.selected_entities.size()) {
				
				if (editor.selected_entities[i] == entity) {
					editor.selected_entities.erase(i);
					break;
				}
			}
		}

		gui_pop_id();
    }

    SV_INTERNAL void update_hierarchy_folder(HierarchyFolder& folder)
    {
		u32 i = 0u;
	
		while(i < folder.entities.size()) {

			EditorEntity& entity = folder.entities[i];
	    
			if (!entity_exist(entity.handle)) {

				folder.entities.erase(i);
			}
			else ++i;
		}
	
		for (HierarchyFolder& f : folder.folders) {

			update_hierarchy_folder(f);
		}
    }

    // Returns if the folder should be destroyed
    SV_AUX bool display_create_popup(HierarchyFolder& folder)
    {
		bool destroy = false;
	
		gui_push_id("Create Popup");

		HierarchyFolder& root = editor.hierarchy.root;

		GuiPopupTrigger trigger = (&root == &folder) ? GuiPopupTrigger_Root : GuiPopupTrigger_LastWidget;
	
		if (gui_begin_popup(trigger)) {

			if (gui_button("Create Folder", 0u)) {
				// TODO: do undo actions
				HierarchyFolder& f = folder.folders.emplace_back();

				if (&root == &folder)
					strcpy(f.path, "Folder");
				else
					sprintf(f.path, "%s/Folder", folder.path);
		
				f.show = false;
				// TODO: show in cascade
				folder.show = true;
			}
		
			if (gui_button("Create Entity", 1u)) {
				editor_create_entity();
			}
		
			if (gui_button("Create Sprite", 2u)) {

				editor_create_entity(SV_ENTITY_NULL, "Sprite", construct_entity_sprite);
			}

			if (gui_button("Create 2D Camera", 3u)) {

				editor_create_entity(SV_ENTITY_NULL, "Camera", construct_entity_2D_camera);
			}

			if (&root != &folder) {

				if (gui_button("Destroy", 4u)) {

					destroy = true;
				}
			}

			gui_end_popup();
		}

		gui_pop_id();

		return destroy;
    }

    SV_INTERNAL void display_hierarchy_folder(HierarchyFolder& folder)
    {
		gui_push_id("Folders");

		u32 id = 0u;
	
		for (HierarchyFolder& f : folder.folders) {

			gui_checkbox(f.path, f.show, id++);

			display_create_popup(f);

			if (f.show) {
				display_hierarchy_folder(f);
			}
		}

		gui_pop_id();

		for (EditorEntity& e : folder.entities) {

			show_entity(e.handle);
		}
    }
    
    void display_entity_hierarchy()
    {
		auto& data = editor.hierarchy;
	
		// Asser that all the entities exists
		update_hierarchy_folder(data.root);
	
		if (gui_begin_window("Hierarchy")) {

			display_hierarchy_folder(data.root);

			display_create_popup(data.root);
	    
			gui_end_window();
		}
    }

    void display_entity_inspector()
    {
		Entity selected = (editor.selected_entities.size() == 1u) ? editor.selected_entities.back() : SV_ENTITY_NULL;

		if (gui_begin_window("Inspector")) {

			if (selected != SV_ENTITY_NULL) {

				// Entity name
				{
					const char* entity_name = get_entity_name(selected);
					egui_header(entity_name, 0u);
				}

				// Entity transform
				egui_transform(selected);

				// Entity components
				{
					u32 comp_count = get_entity_component_count(selected);

					gui_push_id("Entity Components");

					foreach(comp_index, comp_count) {

						CompRef ref = get_component_by_index(selected, comp_index);

						show_component_info(selected, ref.id, ref.ptr);
						comp_count = get_entity_component_count(selected);
					}

					gui_pop_id();
				}

				// Misc
				{
					gui_push_id("Misc info");

					SceneData* scene = get_scene_data();
		    
					bool is_player = selected == scene->player;
		    
					if (gui_checkbox("Player", is_player, 0u)) {

						if (is_player) {
							scene->player = selected;
						}
						else scene->player = SV_ENTITY_NULL;
					}

					gui_push_id("User");

					ShowEntityEvent event;
					event.entity = selected;

					event_dispatch("show_entity_info", &event);
		    
					gui_pop_id(2u);
				}

				if (gui_begin_popup(GuiPopupTrigger_Root)) {
		    
					u32 count = get_component_register_count();
					foreach(i, count) {

						CompID comp_id = CompID(i);

						if (get_component_by_id(selected, comp_id))
							continue;
			
						if (gui_button(get_component_name(comp_id), comp_id)) {
			    
							add_component_by_id(selected, comp_id);
						}
					}

					gui_end_popup();
				}
			}

			gui_end_window();
		}
    }

    SV_INTERNAL void display_asset_browser()
    {
		AssetBrowserInfo& info = editor.asset_browser;
		
		bool update_browser = false;
		char next_filepath[FILEPATH_SIZE + 1] = "";

		string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

		if (gui_begin_window("Create Folder", GuiWindowFlag_Temporal)) {

			static char foldername[FILENAME_SIZE + 1u] = "";
			
			gui_text_field(foldername, FILENAME_SIZE + 1u, 0u);

			if (gui_button("Create")) {
				
				update_browser = true;

				char filepath[FILEPATH_SIZE + 1u] = "assets/";
				string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
				string_append(filepath, foldername, FILEPATH_SIZE + 1u);

				if (!folder_create(filepath)) {
					SV_LOG_ERROR("Can't create the folder '%s'", filepath);
				}

				gui_hide_window("Create Folder");
				string_copy(foldername, "", FILENAME_SIZE + 1u);
			}

			gui_end_window();
		}

		if (gui_begin_window("Import Model", GuiWindowFlag_Temporal)) {

			static char foldername[FILENAME_SIZE + 1u] = "";
			static bool create_folder = false;

			gui_checkbox("Create Folder", create_folder);

			if (create_folder) {

				gui_text_field(foldername, FILENAME_SIZE + 1u, 0u);
			}

			if (gui_button("Import")) {

				if (string_size(foldername) == 0u)
					create_folder = false;

				char filepath[FILEPATH_SIZE + 1u] = "";

				const char* filters[] = {
					"Wavefront OBJ (.obj)", "*.obj",
					"All", "*",
					""
				};
			
				if (file_dialog_open(filepath, 2u, filters, "")) {

					ModelInfo model_info;
			    
					if (load_model(filepath, model_info)) {

						char dst[FILEPATH_SIZE + 1u];
						string_copy(dst, "assets/", FILEPATH_SIZE + 1u);
						string_append(dst, info.filepath, FILEPATH_SIZE + 1u);
						if (create_folder)
							string_append(dst, foldername, FILEPATH_SIZE + 1u);

						if (import_model(dst, model_info)) {
							SV_LOG_INFO("Model imported '%s'", filepath);
						}
						else SV_LOG_ERROR("Can't import the model '%s' at '%s'", filepath, dst);
					}
					else {
						SV_LOG_ERROR("Can't load the model '%s'", filepath);
					}
				}

				create_folder = false;
				string_copy(foldername, "", FILENAME_SIZE + 1u);
				gui_hide_window("Import Model");
			}

			gui_end_window();
		}

		if (gui_begin_window("Asset Browser")) {

			// TEMP
			if (input.unused && input.keys[Key_Control] && input.keys[Key_B] == InputState_Pressed) {

				string_copy(next_filepath, info.filepath, FILEPATH_SIZE + 1u);

				char* end = next_filepath;
				char* it = next_filepath + string_size(next_filepath) - 1u;

				while (it > end && *it != '/') --it;

				if (it >= end) next_filepath[0] = '\0';
				else if (*it == '/') *(it + 1u) = '\0';
		
		
				update_browser = true;
			}

			if (gui_select_filepath(info.filepath, next_filepath, 0u)) {

				update_browser = true;
			}
	    
			{
				gui_begin_grid(85.f, 3.f, 1u);

				foreach(i, info.elements.size()) {

					const AssetElement& e = info.elements[i];

					gui_push_id((u64)e.type);

					// TODO: ignore unused elements
					switch (e.type) {

					case AssetElementType_Directory:
					{
						if (gui_asset_button(e.name, editor.image.get(), editor.TEXCOORD_FOLDER, 0u)) {
			    
							if (e.type == AssetElementType_Directory && !update_browser) {

								update_browser = true;

								size_t new_size = strlen(info.filepath) + strlen(e.name) + 1u;
								if (new_size < FILEPATH_SIZE)
									sprintf(next_filepath, "%s%s/", info.filepath, e.name);
								else
									SV_LOG_ERROR("This filepath exceeds the max filepath size");
							}
						}

						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								update_browser = true;

								char filepath[FILEPATH_SIZE + 1u] = "assets/";
								string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
								string_append(filepath, e.name, FILEPATH_SIZE + 1u);

								if (folder_remove(filepath)) {
									SV_LOG_INFO("Folder '%s' removed", filepath);
								}
								else {
									SV_LOG_ERROR("Can't remove the folder '%s'", filepath);
								}
							}
							
							gui_end_popup();
						}
					}
					break;

					case AssetElementType_Texture:
					case AssetElementType_Mesh:
					case AssetElementType_Material:
					case AssetElementType_SpriteSheet:
					{
						AssetPackage pack;
						size_t filepath_size = strlen(info.filepath);
						size_t size = filepath_size + strlen(e.name) + strlen("assets/");
						SV_ASSERT(size <= FILEPATH_SIZE);
						if (size > FILEPATH_SIZE) continue;

						sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);

						if (e.type == AssetElementType_Texture) {

							TextureAsset tex;

							if (get_asset_from_file(tex, pack.filepath)) {

								gui_asset_button(e.name, tex.get(), {0.f, 0.f, 1.f, 1.f}, 0u);
							}
							// TODO: Set default image
							else {
								gui_asset_button(e.name, nullptr, {0.f, 0.f, 1.f, 1.f}, 0u);
							}
						}
						else {

							gui_asset_button(e.name, nullptr, {0.f, 0.f, 1.f, 1.f}, 0u);
						}

						u32 id;

						switch (e.type) {

						case AssetElementType_Texture:
							id = ASSET_BROWSER_PACKAGE_TEXTURE;
							break;

						case AssetElementType_Mesh:
							id = ASSET_BROWSER_PACKAGE_MESH;
							break;

						case AssetElementType_Material:
							id = ASSET_BROWSER_PACKAGE_MATERIAL;
							break;

						case AssetElementType_SpriteSheet:
							id = ASSET_BROWSER_PACKAGE_SPRITE_SHEET;
							break;

						default:
							id = u32_max;
							break;
						}

						if (id != u32_max) {
			    
							gui_send_package(&pack, sizeof(AssetPackage), id);
						}
					}
					break;

					}
				}

				gui_end_grid();
			}

			// Update per time
			if (!update_browser) {
				f64 now = timer_now();

				if (now - info.last_update > 1.0) {
					update_browser = true;
					sprintf(next_filepath, "%s", info.filepath);
				}
			}

			// Update browser elements
			if (update_browser) {

				strcpy(info.filepath, next_filepath);
				sprintf(next_filepath, "assets/%s", info.filepath);
		
				while (!file_exists(next_filepath) && next_filepath[0]) {

					size_t size = strlen(next_filepath) - 1u;
		    
					next_filepath[size] = '\0';
		    
					while (size && next_filepath[size] != '/') {
						--size;
						next_filepath[size] = '\0';
					}
				}

				if (strlen(next_filepath) < strlen("assets/")) {
					strcpy(next_filepath, "assets/");
				}
		
				// Clear browser data
				info.elements.clear();

				FolderIterator it;
				FolderElement e;
		
				bool res = folder_iterator_begin(next_filepath, &it, &e);

				if (res) {

					do {

						if (strcmp(e.name, ".") == 0 || strcmp(e.name, "..") == 0)
							continue;
			
						AssetElement element;
			
						// Select element type
						if (e.is_file) {

							if (strcmp(e.extension, "mesh") == 0) element.type = AssetElementType_Mesh;
							else if (strcmp(e.extension, "mat") == 0) element.type = AssetElementType_Material;
							else if (strcmp(e.extension, "png") == 0 || strcmp(e.extension, "jpg") == 0 || strcmp(e.extension, "gif") == 0 || strcmp(e.extension, "jpeg") == 0) element.type = AssetElementType_Texture;
							else if (strcmp(e.extension, "sprites") == 0) element.type = AssetElementType_SpriteSheet;
							else element.type = AssetElementType_Unknown;
						}
						else {
							element.type = AssetElementType_Directory;
						}

						// Set name
						size_t name_size = strlen(e.name);
						memcpy(element.name, e.name, name_size);
						element.name[name_size] = '\0';

						info.elements.emplace_back(element);
					}
					while (folder_iterator_next(&it, &e));
				}
				else {

					SV_LOG_ERROR("Can't create asset browser content at '%s'", next_filepath);
				}
		
		
				strcpy(info.filepath, next_filepath + strlen("assets/"));
				info.last_update = timer_now();
			}

			if (gui_begin_popup(GuiPopupTrigger_Root)) {

				u64 id = 0u;

				if (gui_button("Create Folder", id++)) {

					gui_show_window("Create Folder");
					gui_close_popup();
				}

				gui_separator(10.f);

				if (gui_button("Import Model", id++)) {

					gui_show_window("Import Model");
					gui_close_popup();
				}

				if (gui_button("Create Sprite Sheet", id++)) {

					char filepath[FILEPATH_SIZE + 1u] = "assets/";

					string_append(filepath, info.filepath, FILEPATH_SIZE + 1u);
					string_append(filepath, "test.sprites", FILEPATH_SIZE + 1u);

					bool show_win = false;

					if (!file_exists(filepath)) {
					
						Serializer s;
						serialize_begin(s);

						serialize_sprite_sheet(s, {});

						if (!serialize_end(s, filepath)) {
							SV_LOG_ERROR("Can't create the sprite sheet at '%s'", filepath);
						}
						else {

							show_win = true;
						}
					}
					else show_win = true;

					if (show_win) {

						gui_show_window("SpriteSheet Editor");
						SpriteSheetAsset& asset = editor.sprite_sheet_editor_data.current_sprite_sheet;
						if (!load_asset_from_file(asset, filepath)) {

							SV_LOG_ERROR("Unknown error loading '%s'", filepath);
							gui_hide_window("SpriteSheet Editor");
						}
					}
				}
		
				gui_end_popup();
			}
	    
			gui_end_window();
		}
    }

    void display_scene_settings()
    {
		SceneData& s = *get_scene_data();

		if (gui_begin_window("Go to scene", GuiWindowFlag_Temporal)) {
		
			gui_text_field(editor.next_scene_name, SCENENAME_SIZE + 1u, 0u);

			gui_separator(10.f);

			if (gui_button("GO!")) {
				set_scene(editor.next_scene_name);
				string_copy(editor.next_scene_name, "", SCENENAME_SIZE + 1u);

				gui_hide_window("Go to scene");
			}

			gui_end_window();
		}
	
		if (gui_begin_window("Scene Settings")) {

			gui_text(get_scene_name(), 0u);

			if (gui_button("Go to scene"))
				gui_show_window("Go to scene");

			if (gui_collapse("Rendering")) {
				gui_drag_color("Ambient Light", s.ambient_light);
			}

			if (gui_collapse("Physics")) {

				if (s.physics.in_3D) {
					gui_drag_v3_f32("Gravity", s.physics.gravity, 0.01f, -f32_max, f32_max);
				}
				else {
					v2_f32 v2;

					v2 = vec3_to_vec2(s.physics.gravity);
					if (gui_drag_v2_f32("Gravity", v2, 0.01f, -f32_max, f32_max))
						s.physics.gravity = vec2_to_vec3(v2);
				}
				
				gui_checkbox("3D", s.physics.in_3D);
			}
	    
			gui_end_window();
		}
    }

    SV_INTERNAL void display_spritesheet_editor()
    {
		if (gui_begin_window("SpriteSheet Editor")) {

			auto& data = editor.sprite_sheet_editor_data;

			SpriteSheet* sheet = data.current_sprite_sheet.get();

			if (sheet) {

				bool save = false;

				GPUImage* image = sheet->texture.get();

				// TODO: back button in the window
				
				switch (data.state) {

				case SpriteSheetEditorState_Main:
				{
					if (gui_button("Sprites", 0u)) {
						data.next_state = SpriteSheetEditorState_SpriteList;
					}
					if (gui_button("Sprite Animations", 1u)) {
						data.next_state = SpriteSheetEditorState_AnimationList;
					}
				}
				break;

				case SpriteSheetEditorState_SpriteList:
				{
					u64 id = 0u;

					if (egui_comp_texture("Texture", id++, &sheet->texture))
						save = true;
					
					if (gui_button("New Sprite", id++)) {
						data.next_state = SpriteSheetEditorState_NewSprite;
					}

					u32 remove_id = u32_max;

					GuiSpritePackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						
						Sprite& sprite = *it;
						if (gui_image_button(sprite.name, image, sprite.texcoord, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifySprite;
						}

						package.sprite_id = it.get_index();
						gui_send_package(&package, sizeof(GuiSpritePackage), GUI_PACKAGE_SPRITE);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprites.erase(remove_id);
						save = true;
					}
				}
				break;

				case SpriteSheetEditorState_NewSprite:
				{
					Sprite& sprite = data.temp_sprite;
					
					gui_image(image, 80.f, sprite.texcoord, 0u);

					gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

					gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

					if (gui_button("Save", 3u)) {

						if (sheet->add_sprite(NULL, sprite.name, sprite.texcoord)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_SpriteList;
						}
					}
				}
				break;

				case SpriteSheetEditorState_ModifySprite:
				{
					if (!sheet->sprites.exists(data.modifying_id))
						data.next_state = SpriteSheetEditorState_SpriteList;
					else {
						
						Sprite& sprite = data.temp_sprite;
					
						gui_image(image, 80.f, sprite.texcoord, 0u);

						gui_text_field(sprite.name, SPRITE_NAME_SIZE + 1u, 1u);

						gui_drag_v4_f32("Texcoord", sprite.texcoord, 0.001f, 0.f, 1.f, 2u);

						if (gui_button("Save sprite", 3u)) {
							
							if (sheet->modify_sprite(data.modifying_id, sprite.name, sprite.texcoord)) {
								
								// TEMP
								data.next_state = SpriteSheetEditorState_SpriteList;

								save = true;
							}
						}
					}
				}
				break;

				case SpriteSheetEditorState_AnimationList:
				{
					u64 id = 0u;
					
					if (gui_button("New Sprite Animation", id++)) {
						data.next_state = SpriteSheetEditorState_NewAnimation;
					}

					u32 remove_id = u32_max;

					GuiSpriteAnimationPackage package;
					string_copy(package.sprite_sheet_filepath, data.current_sprite_sheet.get_filepath(), FILEPATH_SIZE + 1u);
					
					for (auto it = sheet->sprite_animations.begin();
						 it.has_next();
						 ++it)
					{
						u32 index = it.get_index();
						SpriteAnimation& anim = *it;

						u32 current_sprite = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_sprite]);
						
						if (gui_image_button(anim.name, image, texcoord, index + 28349u)) {

							data.modifying_id = index;
							data.next_state = SpriteSheetEditorState_ModifyAnimation;
						}

						package.animation_id = it.get_index();

						gui_send_package(&package, sizeof(GuiSpriteAnimationPackage), GUI_PACKAGE_SPRITE_ANIMATION);
						
						if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

							if (gui_button("Remove", 0u)) {

								remove_id = index;
							}

							gui_end_popup();
						}
					}

					if (remove_id != u32_max) {

						sheet->sprite_animations.erase(remove_id);
						save = true;
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_NewAnimation:
				{
					SpriteAnimation& anim = data.temp_anim;

					if (anim.frames) {
						
						u32 current_spr = u32(data.simulation_time / anim.frame_time) % anim.frames;
						v4_f32 texcoord = sheet->get_sprite_texcoord(anim.sprites[current_spr]);
					
						gui_image(image, 80.f, texcoord, 0u);
					}
					else gui_image(NULL, 80.f, {}, 0u);

					gui_text_field(anim.name, SPRITE_NAME_SIZE + 1u, 1u);
					
					foreach(i, anim.frames) {

						u32 spr_id = anim.sprites[i];
						
						if (sheet->sprites.exists(spr_id)) {

							const Sprite& spr = sheet->sprites[spr_id];

							if (gui_image_button(spr.name, image, spr.texcoord, spr_id + 84390u)) {
								
							}
						}
						else {
							// TODO
						}
					}

					if (gui_button("Add Sprite", 2u)) {
						data.next_state = SpriteSheetEditorState_AddSprite;
					}
					if (gui_button("Save", 3u)) {
						if (sheet->add_sprite_animation(NULL, anim.name, anim.sprites, anim.frames, anim.frame_time)) {
							
							save = true;
							data.next_state = SpriteSheetEditorState_AnimationList;
						}
					}

					data.simulation_time += engine.deltatime;
				}
				break;

				case SpriteSheetEditorState_AddSprite:
				{
					for (auto it = sheet->sprites.begin();
						 it.has_next();
						 ++it)
					{
						if (gui_image_button(it->name, image, it->texcoord, it.get_index() + 38543u)) {

							data.next_state = SpriteSheetEditorState_NewAnimation;
							u32& spr = data.temp_anim.sprites[data.temp_anim.frames++];
							spr = it.get_index();
						}
					}
				}
				break;
					
				}

				// Initialize new state
				if (data.state != data.next_state) {

					switch (data.next_state) {

					case SpriteSheetEditorState_NewSprite:
					{
						string_copy(data.temp_sprite.name, "Name", SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = {0.f, 0.f, 1.f, 1.f};
					}
					break;

					case SpriteSheetEditorState_NewAnimation:
					{
						if (data.state != SpriteSheetEditorState_AddSprite) {
							
							string_copy(data.temp_anim.name, "Name", SPRITE_NAME_SIZE + 1u);
							data.temp_anim.frames = 0u;
							data.temp_anim.frame_time = 0.1f;
						}
					}
					break;

					case SpriteSheetEditorState_ModifySprite:
					{
						Sprite& s = sheet->sprites[data.modifying_id];
						string_copy(data.temp_sprite.name, s.name, SPRITE_NAME_SIZE + 1u);
						data.temp_sprite.texcoord = s.texcoord;
					}
					break;

					case SpriteSheetEditorState_AnimationList:
					{
						data.simulation_time = 0.f;
					}
					break;
						
					}

					data.state = data.next_state;
				}

				// Save SpriteSheet
				if (save) {

					Serializer s;
					serialize_begin(s);

					serialize_sprite_sheet(s, *sheet);

					serialize_end(s, data.current_sprite_sheet.get_filepath());
				}
			}
	    
			gui_end_window();
		}
		// Free asset
		else {
			unload_asset(editor.sprite_sheet_editor_data.current_sprite_sheet);
		}
    }

    SV_INTERNAL void display_gui()
    {
		if (_gui_begin()) {

			if (!editor.camera_focus && there_is_scene() && dev.debug_draw) {

				// Window management
				if (gui_begin_window("Window Manager", GuiWindowFlag_NoClose)) {

					if (gui_button("Hierarchy")) {
						gui_show_window("Hierarchy");
					}
					if (gui_button("Inspector")) {
						gui_show_window("Inspector");
					}
					if (gui_button("Asset Browser")) {
						gui_show_window("Asset Browser");
					}
					if (gui_button("Scene Settings")) {
						gui_show_window("Scene Settings");
					}

					gui_checkbox("Colisions", dev.draw_collisions);
					gui_checkbox("Postprocessing", dev.postprocessing);

					if (gui_button("Exit Project")) {
						dev.next_engine_state = EngineState_ProjectManagement;
					}
					gui_end_window();
				}
		
				display_entity_hierarchy();
				display_entity_inspector();
				display_asset_browser();
				display_scene_settings();
				display_spritesheet_editor();
				gui_display_style_editor();

				event_dispatch("display_gui", NULL);

			}
			else {
				// TODO
			}

			_gui_end();
		}
    }

    SV_INTERNAL void do_picking_stuff()
    {
		update_gizmos();
	
		// Entity selection
		if (input.unused && input.mouse_buttons[MouseButton_Left] == InputState_Released)
			select_entity();
    }
    
    SV_INTERNAL void update_edit_state()
    {	
		// Adjust camera
		dev.camera.adjust(os_window_aspect());

		for (u32 i = 0u; i < editor.selected_entities.size();) {

			if (!entity_exist(editor.selected_entities[i]))
				editor.selected_entities.erase(i);
			else ++i;
		}
	
		display_gui();

		if (dev.debug_draw && there_is_scene()) {

			control_camera();
		}

		if (there_is_scene())
			do_picking_stuff();
    }

    SV_INTERNAL void update_play_state()
    {
		if (dev.debug_draw) {
			display_gui();

			if (there_is_scene()) {
				control_camera();
				do_picking_stuff();
			}
		}
    }

    void update_project_state()
    {
		if (_gui_begin()) {

			// TEST
			{
				if (gui_begin_window("Test")) {

					TextureAsset tex;

					if (load_asset_from_file(tex, "$system/images/skymap.jpeg"))
						gui_image(tex.get(), 0.f, 0u, GuiImageFlag_Fullscreen);
		    
					gui_end_window();
				}
			}

			if (gui_button("New project", 0u)) {
		
				char path[FILEPATH_SIZE + 1u] = "";
		    
				if (file_dialog_save(path, 0u, nullptr, "")) {

					char* extension = filepath_extension(path);
					if (extension != nullptr) {
						*extension = '\0';
					}
					strcat(path, ".silver");

					const char* content = "test";
					bool res = file_write_text(path, content, strlen(content));

					if (res) {
			    
						char* name = filepath_name(path);
						*name = '\0';

						char folderpath[FILEPATH_SIZE + 1u];
			    
						sprintf(folderpath, "%s%s", path, "assets");
						res = folder_create(folderpath);
			    
						if (res) {
							sprintf(folderpath, "%s%s", path, "src");
							res = folder_create(folderpath);
						}

						if (res) {

							sprintf(folderpath, "%s%s", path, "src/build_unit.cpp");
							res = file_copy("$system/default_code.cpp", folderpath);
						}
					}
		    
			
					if (res)
						SV_LOG_INFO("Project in '%s' created", path);
		    
					else {
						SV_LOG_ERROR("Can't create the project in '%s'", path);
					}
				}
			}
			if (gui_button("Open project", 1u)) {

				char path[FILEPATH_SIZE + 1u] = "";

				const char* filter[] = {
					"Silver Engine (.silver)", "*.silver",
					"All", "*",
					""
				};

				if (file_dialog_open(path, 2u, filter, "")) {
			
					*filepath_name(path) = '\0';
					_engine_initialize_project(path);
					dev.next_engine_state = EngineState_Edit;
				}
			}
	    
			_gui_end();
		}    
    }
    
    void _editor_update()
    {
		bool exit = false;
	
		// CHANGE EDITOR MODE
		if (dev.next_engine_state != EngineState_None) {

			switch (dev.next_engine_state) {

			case EngineState_ProjectManagement:
			{
				_engine_close_project();
				editor.selected_entities.reset();
				exit = true;
				engine.update_scene = false;
				_gui_load("PROJECT");
			}
			break;

			case EngineState_Edit:
			{
				SV_LOG_INFO("Starting edit state");
				// TODO: Handle error
				_start_scene(get_scene_name());
		
				dev.debug_draw = true;
				engine.update_scene = false;
				_gui_load(engine.project_path);
			} break;

			case EngineState_Play:
			{
				SV_LOG_INFO("Starting play state");

				engine.update_scene = true;
			
				if (dev.engine_state == EngineState_Edit) {

					// TODO: handle error
					save_scene();

					dev.debug_draw = false;
					dev.draw_collisions = false;
					editor.selected_entities.reset();
				}
			} break;

			case EngineState_Pause:
			{
				SV_LOG_INFO("Game paused");
				engine.update_scene = false;
			} break;
			
			}

			dev.engine_state = dev.next_engine_state;
			dev.next_engine_state = EngineState_None;

			if (exit)
				return;
		}

		update_key_shortcuts();
	
		switch (dev.engine_state) {

		case EngineState_Edit:
			update_edit_state();
			break;

		case EngineState_Play:
			update_play_state();
			break;

		case EngineState_ProjectManagement:
			update_project_state();
			break;
	    
		}
    }

    SV_INTERNAL void draw_edit_state(CommandList cmd)
    {
		if (!dev.debug_draw) return;
	
		imrend_begin_batch(cmd);

		imrend_camera(ImRendCamera_Editor, cmd);

		// Draw selected entity
		for (Entity entity : editor.selected_entities) {

			MeshComponent* mesh_comp = get_component<MeshComponent>(entity);
			SpriteComponent* sprite_comp = get_component<SpriteComponent>(entity);

			if (mesh_comp && mesh_comp->mesh.get()) {

				u8 alpha = 5u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 30.f * 0.5f);
				XMMATRIX wm = get_entity_world_matrix(entity);

				imrend_push_matrix(wm, cmd);
				imrend_draw_mesh_wireframe(mesh_comp->mesh.get(), Color::Red(alpha), cmd);
				imrend_pop_matrix(cmd);
			}
			if (sprite_comp) {

				XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
				XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
				XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

				XMMATRIX tm = get_entity_world_matrix(entity);

				p0 = XMVector3Transform(p0, tm);
				p1 = XMVector3Transform(p1, tm);
				p2 = XMVector3Transform(p2, tm);
				p3 = XMVector3Transform(p3, tm);

				u8 alpha = 50u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 200.f * 0.5f);
				Color selection_color = Color::Red(alpha);

				imrend_draw_line(v3_f32(p0), v3_f32(p1), selection_color, cmd);
				imrend_draw_line(v3_f32(p1), v3_f32(p3), selection_color, cmd);
				imrend_draw_line(v3_f32(p3), v3_f32(p2), selection_color, cmd);
				imrend_draw_line(v3_f32(p2), v3_f32(p0), selection_color, cmd);
			}
		}

		// Draw 2D grid
		if (dev.camera.projection_type == ProjectionType_Orthographic && dev.debug_draw) {

			f32 width = dev.camera.width;
			f32 height = dev.camera.height;
			f32 mag = dev.camera.getProjectionLength();

			u32 count = 0u;
			for (f32 i = 0.01f; count < 3u; i *= 10.f) {

				if (mag / i <= 50.f) {

					Color color;

					switch (count++)
					{
					case 0:
						color = Color::Gray(50);
						break;

					case 1:
						color = Color::Gray(100);
						break;

					case 2:
						color = Color::Gray(150);
						break;

					case 3:
						color = Color::Gray(200);
						break;
					}

					color.a = 10u;

					imrend_draw_orthographic_grip(vec3_to_vec2(dev.camera.position), {}, { width, height }, i, color, cmd);
				}
			}
		}

		// Draw light probes
		{
			XMMATRIX tm;
			
			for_each_comp<LightComponent>([&] (Entity entity, LightComponent& light)
				{
					v3_f32 pos = get_entity_world_position(entity);

					f32 min_scale = relative_scalar(0.02f, pos);
					f32 scale = SV_MAX(min_scale, 1.f);
					
					tm = XMMatrixScaling(scale, scale, 1.f) * XMMatrixRotationQuaternion(vec4_to_dx(dev.camera.rotation)) * XMMatrixTranslation(pos.x, pos.y, pos.z);

					imrend_push_matrix(tm, cmd);

					imrend_draw_sprite({}, { 1.f, 1.f }, Color::White(), editor.image.get(), GPUImageLayout_ShaderResource, editor.TEXCOORD_LIGHT_PROBE, cmd);

					imrend_pop_matrix(cmd);

					if (is_entity_selected(entity) && light.light_type == LightType_Direction) {

						// Draw light direction

						v3_f32 dir = v3_f32::forward();

						XMVECTOR quat = vec4_to_dx(get_entity_world_rotation(entity));

						tm = XMMatrixRotationQuaternion(quat);

						dir = XMVector3Transform(vec3_to_dx(dir), tm);

						dir = vec3_normalize(dir) * scale * 1.5f;
						imrend_draw_line(pos, pos + dir, light.color, cmd);
					}

					return true;
				});
		}

		// Draw collisions
		if (dev.draw_collisions) {

			XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.5f, 1.f);
			XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.5f, 1.f);
			XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.5f, 1.f);
			XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.5f, 1.f);
			XMVECTOR p4 = XMVectorSet(-0.5f, 0.5f, -0.5f, 1.f);
			XMVECTOR p5 = XMVectorSet(0.5f, 0.5f, -0.5f, 1.f);
			XMVECTOR p6 = XMVectorSet(-0.5f, -0.5f, -0.5f, 1.f);
			XMVECTOR p7 = XMVectorSet(0.5f, -0.5f, -0.5f, 1.f);

			XMVECTOR v0, v1, v2, v3, v4, v5, v6, v7;

			XMMATRIX tm;

			if (get_scene_data()->physics.in_3D) {
		
				for_each_comp<BodyComponent>([&] (Entity entity, BodyComponent& body)
					{
						v3_f32 pos = get_entity_world_position(entity) + body.offset;
						v3_f32 scale = get_entity_world_scale(entity) * body.size;
		    
						tm = XMMatrixScalingFromVector(vec3_to_dx(scale)) * XMMatrixTranslation(pos.x, pos.y, pos.z);
		    
						v0 = XMVector3Transform(p0, tm);
						v1 = XMVector3Transform(p1, tm);
						v2 = XMVector3Transform(p2, tm);
						v3 = XMVector3Transform(p3, tm);
						v4 = XMVector3Transform(p4, tm);
						v5 = XMVector3Transform(p5, tm);
						v6 = XMVector3Transform(p6, tm);
						v7 = XMVector3Transform(p7, tm);

						imrend_draw_line(v3_f32(v0), v3_f32(v1), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v1), v3_f32(v3), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v3), v3_f32(v2), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v0), v3_f32(v2), Color::Green(), cmd);

						imrend_draw_line(v3_f32(v4), v3_f32(v5), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v5), v3_f32(v7), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v7), v3_f32(v6), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v4), v3_f32(v6), Color::Green(), cmd);

						imrend_draw_line(v3_f32(v0), v3_f32(v4), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v1), v3_f32(v5), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v2), v3_f32(v6), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v3), v3_f32(v7), Color::Green(), cmd);

						return true;
					});
			}
			else {

				for_each_comp<BodyComponent>([&] (Entity entity, BodyComponent& body)
					{
						v2_f32 pos = vec3_to_vec2(get_entity_world_position(entity)) + vec3_to_vec2(body.offset);
						v2_f32 scale = vec3_to_vec2(get_entity_world_scale(entity)) * vec3_to_vec2(body.size);
		    
						tm = XMMatrixScalingFromVector(vec2_to_dx(scale)) * XMMatrixTranslation(pos.x, pos.y, 0.f);
		    
						v0 = XMVector3Transform(p0, tm);
						v1 = XMVector3Transform(p1, tm);
						v2 = XMVector3Transform(p2, tm);
						v3 = XMVector3Transform(p3, tm);

						imrend_draw_line(v3_f32(v0), v3_f32(v1), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v1), v3_f32(v3), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v3), v3_f32(v2), Color::Green(), cmd);
						imrend_draw_line(v3_f32(v0), v3_f32(v2), Color::Green(), cmd);

						return true;
					});
			}
		}

		// Draw gizmos
		draw_gizmos(renderer->gfx.offscreen, cmd);

		XMMATRIX vpm = XMMatrixIdentity();

		if (dev.debug_draw)
			vpm = dev.camera.view_projection_matrix;
		else {

			CameraComponent* cam = get_main_camera();
		
			if (cam) {

				vpm = cam->view_projection_matrix;
			}
		}

		imrend_flush(cmd);
    }

    void _editor_draw()
    {
		CommandList cmd = graphics_commandlist_get();

		switch (dev.engine_state) {

		case EngineState_Edit:
		case EngineState_Play:
			draw_edit_state(cmd);
			break;
    
		}

		// Draw gui
		if (dev.debug_draw)
			_gui_draw(cmd);
    }

}

#endif
