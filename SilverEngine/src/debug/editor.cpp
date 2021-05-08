#if SV_DEV

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
	AssetElementType_Directory,
    };

    struct AssetElement {
	char name[FILENAME_SIZE];
	AssetElementType type;
    };

    struct AssetBrowserInfo {
	char filepath[FILEPATH_SIZE] = {};
	List<AssetElement> elements;
	f64 last_update = 0.0;
    };

    struct EditorEntity {
	Entity handle;
    };

    struct HierarchyFolder {
	char path[FILEPATH_SIZE];
	List<EditorEntity> entities;
	List<HierarchyFolder> folders;
	bool show;
    };
    
    struct HierarchyData {
	HierarchyFolder root;
    };

    struct GlobalEditorData {

	Entity selected_entity = SV_ENTITY_NULL;
	bool camera_focus = false;

	AssetBrowserInfo asset_browser;
	GizmosInfo gizmos;

	HierarchyData hierarchy;
	
	TextureAsset image;
	static constexpr v4_f32 TEXCOORD_FOLDER = { 0.f, 0.f, 0.2f, 0.2f };

	struct {
	    Color color = Color{240u, 40, 40, 255u};
	    Color strong_color = Color::Red();
	} palette;

	char next_scene_name[SCENENAME_SIZE + 1u] = "";
    };

    GlobalEditorData editor;

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

    SV_AUX v2_f32 world_point_to_screen(const XMMATRIX& vpm, const v3_f32& position)
    {
	XMVECTOR p = position.getDX(1.f);

	p = XMVector4Transform(p, vpm);
	
	f32 d = XMVectorGetW(p);
	p = XMVectorDivide(p, XMVectorSet(d, d, d, 1.f));
	
	return v2_f32(p);
    }

    SV_AUX v2_f32 project_line_onto_line(v2_f32 origin, v2_f32 d0, v2_f32 d1)
    {
	return origin + (d1.dot(d0) / d1.dot(d1)) * d1;
    }
    SV_AUX v3_f32 project_line_onto_line(v3_f32 origin, v3_f32 d0, v3_f32 d1)
    {
	return origin + (d1.dot(d0) / d1.dot(d1)) * d1;
    }

    SV_AUX void intersect_line_vs_plane(v3_f32 line_point, v3_f32 line_direction, v3_f32 plane_point, v3_f32 plane_normal, v3_f32& intersection, f32& distance)
    {
	distance = (plane_point - line_point).dot(plane_normal) / line_direction.dot(plane_normal);
	intersection = line_point + distance * line_direction;
    }

    SV_AUX void closest_points_between_two_lines(v3_f32 l0_pos, v3_f32 l0_dir, v3_f32 l1_pos, v3_f32 l1_dir, v3_f32& l0_res, v3_f32& l1_res)
    {
	v3_f32 u = l0_dir;
	v3_f32 v = l1_dir;
	v3_f32 w = l0_pos - l1_pos;
	f32 a = u.dot(u);         // always >= 0
	f32 b = u.dot(v);
	f32 c = v.dot(v);         // always >= 0
	f32 d = u.dot(w);
	f32 e = v.dot(w);
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
		
	    XMVECTOR pos = position.getDX(1.f);
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

    SV_INTERNAL void update_gizmos()
    {
	GizmosInfo& info = editor.gizmos;

	if (!input.unused || editor.selected_entity == SV_ENTITY_NULL) {
	    info.focus = false;
	    return;
	}

	v3_f32& position = *get_entity_position_ptr(editor.selected_entity);

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

			f32 dist0 = (p0 - p1).length();
			
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

		    v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + dev.camera.position.getVec2();
		    v2_f32 to_mouse = mouse - position.getVec2();
			
		    foreach (i, 2u) {

			v2_f32 axis_direction = ((i == 0) ? v2_f32::right() : v2_f32::up()) * info.axis_size;

			v2_f32 projection = project_line_onto_line(position.getVec2(), to_mouse, axis_direction);
			f32 dist0 = (mouse - projection).length();
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

		    v2_f32 mouse = input.mouse_position * v2_f32(dev.camera.width, dev.camera.height) + dev.camera.position.getVec2();
		
		    if (info.object == GizmosObject_AxisX)
			position.x = mouse.x - info.start_offset.x;
		    else if (info.object == GizmosObject_AxisY)
			position.y = mouse.y - info.start_offset.y;
		}
	    }
	}
    }

    SV_INTERNAL void draw_gizmos(GPUImage* offscreen, CommandList cmd)
    {
	if (editor.selected_entity == SV_ENTITY_NULL) return;

	GizmosInfo& info = editor.gizmos;

	v3_f32 position = get_entity_position(editor.selected_entity);

	f32 axis_size = info.axis_size;

	switch (info.mode)
	{

	case GizmosTransformMode_Position:
	{
	    Color color = ((info.object == GizmosObject_AxisX) ? (info.focus ? Color::Silver() : Color{255u, 50u, 50u, 255u}) : Color::Red());
	    draw_debug_line(position, position + v3_f32::right() * axis_size, color, cmd);

	    color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
	    draw_debug_line(position, position + v3_f32::up() * axis_size, color, cmd);

	    color = ((info.object == GizmosObject_AxisZ) ? (info.focus ? Color::Silver() : Color{50u, 50u, 255u, 255u}) : Color::Blue());
	    draw_debug_line(position, position + v3_f32::forward() * axis_size, color, cmd);
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
	engine.update_scene = false;

	return true;
    }

    bool _editor_close()
    {
	SV_CHECK(_gui_close());
	return true;
    }

    static void show_component_info(Entity entity, CompID comp_id, BaseComponent* comp)
    {
	bool remove;

	if (egui_begin_component(entity, comp_id, &remove)) {

	    if (SpriteComponent::ID == comp_id) {

		SpriteComponent& spr = *reinterpret_cast<SpriteComponent*>(comp);

		egui_comp_color("Color", 0u, &spr.color);
		egui_comp_texture("Texture", 1u, &spr.texture);
		egui_comp_drag_v4_f32("Coords", 2u, &spr.texcoord, 0.01f, 0.f, 1.f);

		bool xflip = spr.flags & SpriteComponentFlag_XFlip;
		bool yflip = spr.flags & SpriteComponentFlag_YFlip;

		if (egui_comp_bool("XFlip", 3u, &xflip)) spr.flags = spr.flags ^ SpriteComponentFlag_XFlip;
		if (egui_comp_bool("YFlip", 4u, &yflip)) spr.flags = spr.flags ^ SpriteComponentFlag_YFlip;
	    }
	    
	    if (MeshComponent::ID == comp_id) {

		MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);

		egui_comp_mesh("Mesh", 0u, &m.mesh);
		egui_comp_material("Material", 1u, &m.material);
	    }

	    if (CameraComponent::ID == comp_id) {

		CameraComponent& cam = *reinterpret_cast<CameraComponent*>(comp);

		SceneData& d = *get_scene_data();
		bool main = d.main_camera == entity;

		if (egui_comp_bool("MainCamera", 0u, &main)) {

		    if (main) d.main_camera = entity;
		    else d.main_camera = SV_ENTITY_NULL;
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
		egui_comp_drag_f32("Far", 2u, &cam.far, far_adv, far_min, far_max);
		if (egui_comp_drag_f32("Dimension", 3u, &dimension, 0.01f, 0.01f)) {
		    cam.width = dimension;
		    cam.height = dimension;
		}
	    }

	    if (LightComponent::ID == comp_id) {

		LightComponent& l = *reinterpret_cast<LightComponent*>(comp);

		egui_comp_color("Color", 0u, &l.color);
		egui_comp_drag_f32("Intensity", 1u, &l.intensity, 0.05f, 0.0f, f32_max);
		egui_comp_drag_f32("Range", 2u, &l.range, 0.1f, 0.0f, f32_max);
		egui_comp_drag_f32("Smoothness", 3u, &l.smoothness, 0.005f, 0.0f, 1.f);
	    }

	    if (BodyComponent::ID == comp_id) {

		BodyComponent& b = *reinterpret_cast<BodyComponent*>(comp);

		// TODO: Use combobox
		bool static_ = b.body_type == BodyType_Static;
		bool dynamic = b.body_type == BodyType_Dynamic;
		bool projectile = b.body_type == BodyType_Projectile;

		if (egui_comp_bool("Static", 0u, &static_)) {
		    b.body_type = static_ ? BodyType_Static : BodyType_Dynamic;
		}
		if (egui_comp_bool("Dynamic", 1u, &dynamic)) {
		    b.body_type = dynamic ? BodyType_Dynamic : BodyType_Static;
		}
		if (egui_comp_bool("Projectile", 2u, &projectile)) {
		    b.body_type = projectile ? BodyType_Projectile : BodyType_Static;
		}
		
		egui_comp_drag_v2_f32("Size", 3u, &b.size, 0.005f, 0.f, f32_max);
		egui_comp_drag_v2_f32("Offset", 4u, &b.offset, 0.005f);
		egui_comp_drag_v2_f32("Velocity", 5u, &b.vel, 0.01f);
		egui_comp_drag_f32("Mass", 6u, &b.mass, 0.1f, 0.0f, f32_max);
		egui_comp_drag_f32("Friction", 7u, &b.friction, 0.001f, 0.0f, 1.f);
		egui_comp_drag_f32("Bounciness", 8u, &b.bounciness, 0.005f, 0.0f, 1.f);

		bool is_trigger = b.flags & BodyComponentFlag_Trigger;

		if (egui_comp_bool("Trigger", 9u, &is_trigger)) {
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
	    for_each_comp<MeshComponent>([&] (Entity entity, MeshComponent& m)
		{
		    if (editor.selected_entity == entity)
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

			    f32 dis = intersection.length();
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

	    XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
	    XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
	    XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
	    XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

	    XMVECTOR v0;
	    XMVECTOR v1;
	    XMVECTOR v2;
	    XMVECTOR v3;

	    for_each_comp<SpriteComponent>([&] (Entity entity, SpriteComponent&)
		{
		    if (editor.selected_entity == entity)
			return false;

		    XMMATRIX tm = get_entity_world_matrix(entity);

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
			
			dis = SV_MIN(intersection.length(), dis);
		    }

		    if (dis < distance) {
			distance = dis;
			selected = entity;
		    }

		    return true;
		});
	}
	
	
	editor.selected_entity = selected;
    }
    
    static void show_entity_popup(Entity entity, bool& destroy)
    {
	if (gui_begin_popup(GuiPopupTrigger_LastWidget, MouseButton_Right, 0x3254fa + u64(entity), GuiLayout_Flow)) {

	    f32 y = 0.f;
	    constexpr f32 H = 20.f;

	    gui_bounds(GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
	    destroy = gui_button("Destroy", 0u);
	    y += H;

	    gui_bounds(GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
	    if (gui_button("Duplicate", 1u)) {
		duplicate_entity(entity);
	    }
	    y += H;

	    gui_bounds(GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
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

	    bool same = editor.selected_entity == entity;

	    if (same) {
		gui_push_style(GuiStyle_ButtonColor, editor.palette.color);
		gui_push_style(GuiStyle_ButtonHotColor, editor.palette.strong_color);
	    }
	    
	    if (gui_button(name, 0u)) {

		editor.selected_entity = entity;
	    }

	    if (same) {
		gui_pop_style(2u);
	    }

	    show_entity_popup(entity, destroy);
	}
	else {
	    
	    gui_begin_container(1u, GuiLayout_Flow);
	    
	    if (gui_button(name, 0u)) {

		editor.selected_entity = entity;
	    }

	    show_entity_popup(entity, destroy);

	    gui_end_container();

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
	    if (editor.selected_entity == entity)
		editor.selected_entity = SV_ENTITY_NULL;
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

	GuiPopupTrigger trigger = (&root == &folder) ? GuiPopupTrigger_Parent : GuiPopupTrigger_LastWidget;
	
	if (gui_begin_popup(trigger, MouseButton_Right, 0x5634c, GuiLayout_Flow)) {

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

	    gui_checkbox(f.path, &f.show, id++);

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
	
	if (egui_begin_window("Hierarchy")) {

	    display_hierarchy_folder(data.root);

	    display_create_popup(data.root);
	    
	    egui_end_window();
	}
    }

    void display_entity_inspector()
    {
	Entity selected = editor.selected_entity;

	if (egui_begin_window("Inspector")) {

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
		    
		    if (gui_checkbox("Player", &is_player, 0u)) {

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

		if (gui_begin_popup(GuiPopupTrigger_Parent, MouseButton_Right, 0xabc2544 + selected, GuiLayout_Flow)) {
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

	    egui_end_window();
	}
    }

    static void display_asset_browser()
    {
	bool update_browser = false;
	char next_filepath[FILEPATH_SIZE];

	if (egui_begin_window("Asset Browser")) {

	    constexpr f32 WIDTH = 25.f;
	    AssetBrowserInfo& info = editor.asset_browser;

	    {
		gui_begin_container(0u, GuiLayout_Flow);
		
		gui_same_line(3u);

		if (gui_button("<", 0u) && info.filepath[0]) {

		    update_browser = true;
		    size_t len = strlen(info.filepath);

		    info.filepath[--len] = '\0';
		    
		    while (len && info.filepath[len - 1u] != '/') {
			--len;
		    }
		    memcpy(next_filepath, info.filepath, len);
		    next_filepath[len] = '\0';
		}
		
		gui_button(">", 1u);
		
		char filepath[FILEPATH_SIZE + 1u] = "assets/";
		strcat(filepath, info.filepath);

		gui_text(filepath, 2u);
		
		gui_end_container();
	    }

	    {
		gui_begin_container(1u, GuiLayout_Grid);

		gui_push_id("Asset Elements");
		gui_push_style(GuiStyle_ContainerColor, editor.palette.color);

		foreach(i, info.elements.size()) {

		    const AssetElement& e = info.elements[i];

		    gui_push_id((u64)e.type);

		    // TODO: ignore unused elements
		    gui_begin_container(i, GuiLayout_Free);

		    gui_bounds(GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

		    switch (e.type) {

		    case AssetElementType_Directory:
		    {
			gui_push_style(GuiStyle_ButtonImage, editor.image.get());
			gui_push_style(GuiStyle_ButtonTexcoord, editor.TEXCOORD_FOLDER);
			gui_push_style(GuiStyle_ButtonColor, Color::White());
			gui_push_style(GuiStyle_ButtonHotColor, editor.palette.strong_color);

			if (gui_button("", 0u)) {

			    if (e.type == AssetElementType_Directory && !update_browser) {

				update_browser = true;

				size_t new_size = strlen(info.filepath) + strlen(e.name) + 1u;
				if (new_size < FILEPATH_SIZE)
				    sprintf(next_filepath, "%s%s/", info.filepath, e.name);
				else
				    SV_LOG_ERROR("This filepath exceeds the max filepath size");
			    }
			}

			gui_pop_style(4u);
		    }
		    break;

		    case AssetElementType_Texture:
		    case AssetElementType_Mesh:
		    case AssetElementType_Material:
		    {
			AssetPackage pack;
			size_t filepath_size = strlen(info.filepath);
			size_t size = filepath_size + strlen(e.name) + strlen("assets/");
			SV_ASSERT(size <= FILEPATH_SIZE);
			if (size > FILEPATH_SIZE) continue;

			sprintf(pack.filepath, "assets/%s%s", info.filepath, e.name);

			u32 styles = 0u;

			if (e.type == AssetElementType_Texture) {

			    TextureAsset tex;

			    if (get_asset_from_file(tex, pack.filepath)) {

				gui_image(tex.get(), GPUImageLayout_ShaderResource, 0u);
			    }
			    // TODO: Set default image
			    else {

			    }
			}
			else {

			    gui_image(nullptr, GPUImageLayout_ShaderResource, 0u);
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

			default:
			    id = u32_max;
			    break;
			}

			if (id != u32_max) {
			    
			    gui_send_package(&pack, sizeof(AssetPackage), id);
			}

			if (styles)
			    gui_pop_style(styles);
		    }
		    break;

		    }

		    gui_ybounds(GuiCoord::Relative(0.05f), GuiCoord::Relative(0.3f));
		    gui_text(e.name, 1u);

		    gui_end_container();
		    gui_pop_id();
		}

		gui_pop_style(1u);
		gui_pop_id();

		gui_end_container();
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

	    if (gui_begin_popup(GuiPopupTrigger_Parent, MouseButton_Right, 69u, GuiLayout_Flow)) {

		if (gui_button("Import Model", 0u)) {

		    char filepath[FILEPATH_SIZE + 1u] = "";

		    const char* filters[] = {
			"Wavefront OBJ (.obj)", "*.obj",
			"All", "*",
			""
		    };
			
		    if (file_dialog_open(filepath, 2u, filters, "")) {

			ModelInfo model_info;
			    
			if (load_model(filepath, model_info)) {

			    char folder_name[FILEPATH_SIZE + 1u];
			    char* c_name = filepath_name(filepath);
			    strcpy(folder_name, c_name);
			    c_name = filepath_extension(folder_name);
			    if (c_name) c_name[0] = '\0';

			    char dst[FILEPATH_SIZE + 1u];
			    strcpy(dst, "assets/");
			    strcat(dst, info.filepath);
			    strcat(dst, folder_name);

			    // TODO: This folder may be duplicated

			    if (import_model(dst, model_info)) {
				SV_LOG_INFO("Model imported '%s'", filepath);
			    }
			    else SV_LOG_ERROR("Can't import the model '%s' at '%s'", filepath, dst);
			}
			else {
			    SV_LOG_ERROR("Can't load the model '%s'", filepath);
			}
		    }
		}
		
		gui_end_popup();
	    }
	    
	    egui_end_window();
	}
    }

    void display_scene_settings()
    {
	SceneData& s = *get_scene_data();
	
	if (egui_begin_window("Scene Manager")) {

	    gui_text(get_scene_name(), 0u);

	    if (gui_checkbox("Go to scene", 1u)) {
		
		gui_begin_container(2u, GuiLayout_Flow);
		
		gui_text_field(editor.next_scene_name, SCENENAME_SIZE + 1u, 0u);

		if (gui_button("GO!", 1u)) {
		    set_scene(editor.next_scene_name);
		    strcpy(editor.next_scene_name, "");
		}

		gui_end_container();
	    }

	    if (gui_checkbox("Rendering", 3u)) {
		
		gui_begin_container(4u, GuiLayout_Flow);
		
		egui_comp_color("Ambient Light", 0u, &s.ambient_light);

		gui_end_container();
	    }

	    if (gui_checkbox("Physics", 5u)) {
		
		gui_begin_container(6u, GuiLayout_Flow);
		
		egui_comp_drag_v2_f32("Gravity", 0u, &s.gravity, 0.01f);

		gui_end_container();
	    }
	    
	    egui_end_window();
	}
    }

    SV_INTERNAL void display_gui()
    {
	if (egui_begin()) {

	    if (!editor.camera_focus && there_is_scene() && dev.debug_draw) {

		// Window management
		{
		    gui_push_id("WINDOW MANAGER");

		    gui_xbounds(GuiCoord::Pixel(5.f), GuiCoord::Pixel(150.f));
		    gui_ybounds(GuiCoord::IPixel(30.f), GuiAlign_Top, GuiDim::Adjust());

		    gui_push_style(GuiStyle_ContainerColor, editor.palette.color);
		    gui_push_style(GuiStyle_ButtonColor, Color::White());
		    gui_push_style(GuiStyle_ButtonHotColor, editor.palette.strong_color);
		    
		    gui_begin_container(0u, GuiLayout_Flow);

		    if (egui_button("Hierarchy", 0u)) {
			gui_show_window("Hierarchy");
		    }
		    if (egui_button("Inspector", 1u)) {
			gui_show_window("Inspector");
		    }
		    if (egui_button("Asset Browser", 2u)) {
			gui_show_window("Asset Browser");
		    }
		    if (egui_button("Scene Manager", 3u)) {
			gui_show_window("Scene Manager");
		    }

		    gui_checkbox("Colisions", &dev.draw_collisions, 4u);

		    if (gui_button("Exit Project", 5u)) {
			dev.next_engine_state = EngineState_ProjectManagement;
		    }
		    
		    gui_end_container();

		    gui_pop_style(3u);
		    
		    gui_pop_id();
		}
		
		display_entity_hierarchy();
		display_entity_inspector();
		display_asset_browser();
		display_scene_settings();

	    }
	    else {
		// TODO
	    }

	    egui_end();
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

	if (!entity_exist(editor.selected_entity)) {
	    editor.selected_entity = SV_ENTITY_NULL;
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
	if (egui_begin()) {

	    gui_bounds(GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
	    gui_begin_container(0u, GuiLayout_Flow);

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
	
	    gui_end_container();
	    
	    egui_end();
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
		editor.selected_entity = SV_ENTITY_NULL;
		exit = true;
		engine.update_scene = false;
	    }
	    break;

	    case EngineState_Edit:
	    {
		SV_LOG_INFO("Starting edit state");
		// TODO: Handle error
		_start_scene(get_scene_name());
		
		dev.debug_draw = true;
		engine.update_scene = false;
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
		    editor.selected_entity = SV_ENTITY_NULL;
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
	
	begin_debug_batch(cmd);

	// Draw selected entity
	if (editor.selected_entity != SV_ENTITY_NULL) {

	    MeshComponent* mesh_comp = get_component<MeshComponent>(editor.selected_entity);
	    SpriteComponent* sprite_comp = get_component<SpriteComponent>(editor.selected_entity);

	    if (mesh_comp && mesh_comp->mesh.get()) {

		u8 alpha = 5u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 50.f * 0.5f);
		XMMATRIX wm = get_entity_world_matrix(editor.selected_entity);
		draw_debug_mesh_wireframe(mesh_comp->mesh.get(), wm, Color::Red(alpha), cmd);
	    }
	    if (sprite_comp) {

		XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
		XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
		XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

		XMMATRIX tm = get_entity_world_matrix(editor.selected_entity);

		p0 = XMVector3Transform(p0, tm);
		p1 = XMVector3Transform(p1, tm);
		p2 = XMVector3Transform(p2, tm);
		p3 = XMVector3Transform(p3, tm);

		u8 alpha = 50u + u8(f32(sin(timer_now() * 3.5) + 1.0) * 200.f * 0.5f);
		Color selection_color = Color::Red(alpha);

		draw_debug_line(v3_f32(p0), v3_f32(p1), selection_color, cmd);
		draw_debug_line(v3_f32(p1), v3_f32(p3), selection_color, cmd);
		draw_debug_line(v3_f32(p3), v3_f32(p2), selection_color, cmd);
		draw_debug_line(v3_f32(p2), v3_f32(p0), selection_color, cmd);
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

		    draw_debug_orthographic_grip(dev.camera.position.getVec2(), {}, { width, height }, i, color, cmd);
		}
	    }
	}

	// Draw collisions
	if (dev.draw_collisions) {

	    XMVECTOR p0 = XMVectorSet(-0.5f, 0.5f, 0.f, 1.f);
	    XMVECTOR p1 = XMVectorSet(0.5f, 0.5f, 0.f, 1.f);
	    XMVECTOR p2 = XMVectorSet(-0.5f, -0.5f, 0.f, 1.f);
	    XMVECTOR p3 = XMVectorSet(0.5f, -0.5f, 0.f, 1.f);

	    XMVECTOR v0, v1, v2, v3;

	    XMMATRIX tm;
		
	    for_each_comp<BodyComponent>([&] (Entity entity, BodyComponent& body)
		{
		    v2_f32 pos = get_entity_position2D(entity) + body.offset;
		    v2_f32 scale = get_entity_scale2D(entity) * body.size;
		    
		    tm = XMMatrixScalingFromVector(scale.getDX()) * XMMatrixTranslation(pos.x, pos.y, 0.f);
		    
		    v0 = XMVector3Transform(p0, tm);
		    v1 = XMVector3Transform(p1, tm);
		    v2 = XMVector3Transform(p2, tm);
		    v3 = XMVector3Transform(p3, tm);

		    draw_debug_line(v3_f32(v0), v3_f32(v1), Color::Green(), cmd);
		    draw_debug_line(v3_f32(v1), v3_f32(v3), Color::Green(), cmd);
		    draw_debug_line(v3_f32(v3), v3_f32(v2), Color::Green(), cmd);
		    draw_debug_line(v3_f32(v0), v3_f32(v2), Color::Green(), cmd);

		    return true;
		});
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

	end_debug_batch(true, false, vpm, cmd);
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
	    gui_draw(cmd);
    }

}

#endif
