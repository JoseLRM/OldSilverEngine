#if SV_DEV

#include "core/renderer.h"
#include "debug/console.h"

namespace sv {

    enum GizmosTransformMode : u32 {
	GizmosTransformMode_None,
	GizmosTransformMode_Position
    };

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
	v2_f32 start_offset;
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

    struct GlobalEditorData {

	Entity selected_entity = SV_ENTITY_NULL;
	bool camera_focus = false;

	AssetBrowserInfo asset_browser;
	GizmosInfo gizmos;
    };

    GlobalEditorData editor;

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

	// Display windows
	if (input.keys[Key_F2] == InputState_Pressed) {
	    dev.display_windows = !dev.display_windows;
	}

	// Console show - hide
	if (input.keys[Key_F3] == InputState_Pressed) {
	    dev.console_active = !dev.console_active;
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
	
	if (dev.game_state != GameState_Play) {

	    // Close engine or play game
	    if (input.keys[Key_F11] == InputState_Pressed) {

		if (input.keys[Key_Control] && input.keys[Key_Alt])
		    engine.close_request = true;
		else
		    dev.next_game_state = GameState_Play;
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
		dev.next_game_state = GameState_Edit;
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

    SV_INLINE static void world_to_screen_line(const XMMATRIX& vpm, v3_f32 _p0, v3_f32 _p1, v2_f32& res0, v2_f32& res1)
    {
	XMVECTOR p0 = _p0.getDX(1.f);
	XMVECTOR p1 = _p1.getDX(1.f);

	p0 = XMVector4Transform(p0, vpm);
	p1 = XMVector4Transform(p1, vpm);

	f32 d = XMVectorGetW(p0);
	p0 = XMVectorDivide(p0, XMVectorSet(d, d, d, 1.f));
	d = XMVectorGetW(p1);
	p1 = XMVectorDivide(p1, XMVectorSet(d, d, d, 1.f));

	res0 = v2_f32(p0);
	res1 = v2_f32(p1);
    }

    static void update_gizmos()
    {
	GizmosInfo& info = editor.gizmos;

	if (!input.unused || editor.selected_entity == SV_ENTITY_NULL) {
	    info.focus = false;
	    return;
	}

	if (!info.focus) {

	    info.object = GizmosObject_None;

	    v3_f32 position = get_entity_position(editor.selected_entity);

	    const XMMATRIX& vpm = dev.camera.view_projection_matrix;
	    v2_f32 mouse_position = input.mouse_position * 2.f;

	    constexpr f32 GIZMOS_SIZE = 1.f;

	    switch (info.mode)
	    {

	    case GizmosTransformMode_Position:
	    {
		v2_f32 c, y;
		world_to_screen_line(vpm, position, position + v3_f32::up(), c, y);

		// y axis
		{
		    v2_f32 dir0 = y - c;
		    v2_f32 dir1 = mouse_position - c;

		    f32 max_len = dir0.length();
		    dir0.normalize();

		    f32 dot = dir1.dot(dir0);

		    if (dot > 0.f && dot <= max_len) {

			v2_f32 projection = c + dir0 * dot;

			f32 dist = (projection - mouse_position).length();

			if (dist < 0.01f) {
			    info.object = GizmosObject_AxisY;
			    info.start_offset.y = position.y + 1.f;
			}
		    }
		}
	    }
	    break;

	    }

	    if (info.object != GizmosObject_None && input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
				
		info.focus = true;

	    }
	}

	else {

	    if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

		info.focus = false;
	    }
	    else {

		v3_f32& position = *get_entity_position_ptr(editor.selected_entity);
		v2_f32 mouse_position = input.mouse_position * 2.f;

		v2_f32 p0, p1;
		world_to_screen_line(dev.camera.view_projection_matrix, position, position + v3_f32::up(), p0, p1);

		v2_f32 dir0 = p1 - p0;
		v2_f32 dir1 = mouse_position - p0;

		dir0.normalize();
		f32 dot = dir1.dot(dir0);

		v2_f32 projection = p0 + dot * dir0;
		XMVECTOR pos = projection.getDX();
		pos = XMVector3Transform(pos, dev.camera.inverse_view_projection_matrix);

		position.y = XMVectorGetY(pos);
	    }
	}
    }

    static void draw_gizmos(GPUImage* offscreen, CommandList cmd)
    {
	if (editor.selected_entity == SV_ENTITY_NULL) return;

	GizmosInfo& info = editor.gizmos;

	v3_f32 position = get_entity_position(editor.selected_entity);

	constexpr f32 GIZMOS_SIZE = 1.f;


	switch (info.mode)
	{

	case GizmosTransformMode_Position:
	{
	    draw_debug_line(position, position + v3_f32::right() * GIZMOS_SIZE, Color::Red(), cmd);

	    Color color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
	    draw_debug_line(position, position + v3_f32::up() * GIZMOS_SIZE, color, cmd);

	    draw_debug_line(position, position + v3_f32::forward() * GIZMOS_SIZE, Color::Blue(), cmd);
	}
	break;

	}
    }

    /////////////////////////////////////////////// COMMANDS ///////////////////////////////////////////////////////

    bool command_show_window(const char** args, u32 argc)
    {
	bool show = true;

	if (argc == 0u) {
	    SV_LOG_ERROR("This command needs some argument");
	    return false;
	}

	if (argc > 2u) {
	    SV_LOG_ERROR("Too much arguments");
	    return false;
	}

	if (argc == 2u) {

	    if (strcmp(args[1], "show") == 0 || strcmp(args[1], "1") == 0) show = true;
	    else if (strcmp(args[1], "hide") == 0 || strcmp(args[1], "0") == 0) show = false;
	    else {
		SV_LOG_ERROR("Invalid argument '%s'", args[1]);
		return false;
	    }
	}

	bool res = show ? gui_show_window(dev.gui, args[0]) : gui_hide_window(dev.gui, args[0]);

	if (res) {
	    SV_LOG("%s '%s'", show ? "Showing" : "Hiding", args[0]);
	}
	else SV_LOG_ERROR("Window '%s' not found", args[0]);

	return res;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    bool _editor_initialize()
    {
	SV_CHECK(gui_create(hash_string("EDITOR GUI"), &dev.gui));

	// Register commands
	SV_CHECK(register_command("wnd", command_show_window));

	return true;
    }

    bool _editor_close()
    {
	SV_CHECK(gui_destroy(dev.gui));
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

		egui_comp_drag_v2_f32("Size", 0u, &b.size, 0.005f, 0.f, f32_max);
		egui_comp_drag_v2_f32("Offset", 1u, &b.offset, 0.005f);
		egui_comp_drag_f32("Mass", 2u, &b.mass, 0.1f, 0.0f, f32_max);
		egui_comp_drag_f32("Friction", 3u, &b.friction, 0.001f, 0.0f, 1.f);
		egui_comp_drag_f32("Bounciness", 4u, &b.bounciness, 0.005f, 0.0f, 1.f);
	    }

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
	if (gui_begin_popup(dev.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 0x3254fa + u64(entity), GuiLayout_Flow)) {

	    f32 y = 0.f;
	    constexpr f32 H = 20.f;

	    gui_bounds(dev.gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
	    destroy = gui_button(dev.gui, "Destroy", 0u);
	    y += H;

	    gui_bounds(dev.gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
	    if (gui_button(dev.gui, "Duplicate", 1u)) {
		duplicate_entity(entity);
	    }
	    y += H;

	    gui_bounds(dev.gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(y), GuiCoord::IPixel(y + H));
	    
	    if (gui_button(dev.gui, "Create Child", 2u)) {
		editor_create_entity(entity);
	    }

	    gui_end_popup(dev.gui);
	}
    }

    SV_INTERNAL void show_entity(Entity entity)
    {
	GUI* g = dev.gui;

	gui_push_id(g, entity);

	const char* name = get_entity_name(entity);

	u32 child_count = get_entity_childs_count(entity);

	bool destroy = false;

	if (child_count == 0u) {

	    bool same = editor.selected_entity == entity;

	    if (same) {
		gui_push_style(dev.gui, GuiStyle_ButtonColor, Color{240u, 40, 40, 255u});
		gui_push_style(dev.gui, GuiStyle_ButtonHotColor, Color{240u, 40, 40, 255u});
	    }
	    
	    if (gui_button(g, name, 0u)) {

		editor.selected_entity = entity;
	    }

	    if (same) {
		gui_pop_style(dev.gui, 2u);
	    }

	    show_entity_popup(entity, destroy);
	}
	else {
	    
	    gui_begin_container(g, 1u, GuiLayout_Flow);
	    
	    if (gui_button(g, name, 0u)) {

		editor.selected_entity = entity;
	    }

	    show_entity_popup(entity, destroy);

	    gui_end_container(g);

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

	gui_pop_id(g);
    }

    void display_entity_hierarchy()
    {
	if (egui_begin_window("Hierarchy")) {

	    u32 entity_count = get_entity_count();

	    foreach(entity_index, entity_count) {

		Entity entity = get_entity_by_index(entity_index);

		u32 childs = get_entity_childs_count(entity);
		show_entity(entity);

		if (entity_exist(entity))
		    childs = get_entity_childs_count(entity);

		entity_count = get_entity_count();
		entity_index += childs;
	    }

	    if (gui_begin_popup(dev.gui, GuiPopupTrigger_Parent, MouseButton_Right, 0x5634c, GuiLayout_Flow)) {
		
		if (gui_button(dev.gui, "Create Entity", 0u)) {
		    editor_create_entity();
		}
		
		if (gui_button(dev.gui, "Create Sprite", 1u)) {

		    editor_create_entity(SV_ENTITY_NULL, "Sprite", construct_entity_sprite);
		}

		gui_end_popup(dev.gui);
	    }
	    
	    egui_end_window();
	}
    }

    void display_entity_inspector()
    {
	GUI* g = dev.gui;
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

		    gui_push_id(g, "Entity Components");

		    foreach(comp_index, comp_count) {

			CompRef ref = get_component_by_index(selected, comp_index);

			show_component_info(selected, ref.id, ref.ptr);
			comp_count = get_entity_component_count(selected);
		    }

		    gui_pop_id(g);
		}

		if (gui_begin_popup(g, GuiPopupTrigger_Parent, MouseButton_Right, 0xabc2544 + selected, GuiLayout_Flow)) {
		    u32 count = get_component_register_count();
		    foreach(i, count) {

			CompID comp_id = CompID(i);

			if (get_component_by_id(selected, comp_id))
			    continue;
			
			if (gui_button(g, get_component_name(comp_id), comp_id)) {
			    
			    add_component_by_id(selected, comp_id);
			}
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
	char next_filepath[FILEPATH_SIZE];

	if (egui_begin_window("Asset Browser")) {

	    constexpr f32 WIDTH = 25.f;
	    AssetBrowserInfo& info = editor.asset_browser;

	    {		
		gui_begin_container(gui, 0u, GuiLayout_Flow);
		
		if (info.filepath[0]) {

		    // TODO: Adjust using name size
		    f32 width = gui_parent_bounds(gui).z * f32(os_window_size().x);

		    u32 folder_count = 1u;
		    for (char c : info.filepath)
			if (c == '/') ++folder_count;

		    u32 count = SV_MIN(SV_MAX(u32(width / WIDTH), 1u), folder_count);
		    SV_ASSERT(count != 0u);
		    --count;

		    const char* folder_offset = info.filepath;
		    char folder_name[FILEPATH_SIZE];

		    folder_name[0] = 'r';
		    folder_name[1] = 'o';
		    folder_name[2] = 'o';
		    folder_name[3] = 't';
		    folder_name[4] = '\0';

		    foreach(i, count) {

			if (i != 0u) {

			    const char* end = folder_offset;
			    while (*end != '/') {
				++end;
			    }

			    size_t name_size = end - folder_offset;
			    memcpy(folder_name, folder_offset, name_size);
			    folder_name[name_size] = '\0';

			    folder_offset = end;
			    ++folder_offset;
			}
			
			if (gui_button(gui, folder_name, i))
			{

			    if (!update_browser) {

				const char* end = info.filepath;

				i = count - i;
				while (i) {

				    if (*end == '/') {
					--i;
				    }

				    ++end;
				}

				sprintf(next_filepath, "%s", end);
				update_browser = true;
				break;
			    }
			}
		    }
		}

		gui_end_container(gui);
	    }

	    {
		gui_bounds(gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(40.f), GuiCoord::Relative(0.f));
		gui_begin_container(gui, 1u, GuiLayout_Flow);

		gui_bounds(gui, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_begin_grid(gui, u32(info.elements.size()), 150.f, 5.f);

		foreach(i, info.elements.size()) {

		    const AssetElement& e = info.elements[i];

		    // TODO: ignore unused elements
		    gui_begin_grid_element(gui, 69u + i);
		    
		    gui_bounds(gui, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		    if (gui_button(gui, nullptr, 0u)) {

			if (e.type == AssetElementType_Directory && !update_browser) {

			    update_browser = true;

			    size_t new_size = strlen(info.filepath) + strlen(e.name) + 1u;
			    if (new_size < FILEPATH_SIZE)
				sprintf(next_filepath, "%s%s/", info.filepath, e.name);
			    else
				SV_LOG_ERROR("This filepath exceeds the max filepath size");
			}
		    }

		    if (e.type != AssetElementType_Directory) {

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
			
			    AssetPackage pack;
			    size_t filepath_size = strlen(info.filepath);
			    size_t size = filepath_size + strlen(e.name);
			    SV_ASSERT(size < AssetPackage::MAX_SIZE);

			    memcpy(pack.filepath, info.filepath, filepath_size);
			    memcpy(pack.filepath + filepath_size, e.name, strlen(e.name));
			    pack.filepath[size] = '\0';
			    
			    gui_send_package(gui, &pack, sizeof(AssetPackage), id);
			}
		    }

		    gui_bounds(gui, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(0.2f));
		    gui_text(gui, e.name, 1u);

		    gui_end_grid_element(gui);
		}

		gui_end_grid(gui);
		gui_end_container(gui);
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
		
		while (!file_exists(next_filepath) && next_filepath[0]) {

		    size_t size = strlen(next_filepath) - 1u;
		    
		    next_filepath[size] = '\0';
		    
		    while (size && next_filepath[size] != '/') {
			--size;
			next_filepath[size] = '\0';
		    }
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
		

		sprintf(info.filepath, "%s", next_filepath);
		info.last_update = timer_now();
	    }
	    
	    egui_end_window();
	}
    }

    void display_scene_settings()
    {
	SceneData& s = *get_scene_data();
	
	if (egui_begin_window("Scene Manager")) {

	    gui_text(dev.gui, get_scene_name(), 0u);
	    
	    egui_comp_color("Ambient Light", 1u, &s.ambient_light);
	    
	    egui_end_window();
	}
    }

    void _editor_update()
    {
	// CHANGE EDITOR MODE
	if (dev.game_state != dev.next_game_state) {

	    switch (dev.next_game_state) {

	    case GameState_Edit:
	    {
		SV_LOG_INFO("Starting edit state");
		// TODO: Handle error
		_start_scene(get_scene_name());
		
		dev.display_windows = true;
		dev.debug_draw = true;
	    } break;

	    case GameState_Play:
	    {
		SV_LOG_INFO("Starting play state");
			
		if (dev.game_state == GameState_Edit) {

		    // TODO: handle error
		    save_scene();

		    dev.display_windows = false;
		    dev.debug_draw = false;
		    dev.draw_collisions = false;
		}
	    } break;

	    case GameState_Pause:
	    {
		SV_LOG_INFO("Game paused");
	    } break;
			
	    }
		    
	    dev.game_state = dev.next_game_state;
	}
		
	update_key_shortcuts();

	// Adjust camera
	dev.camera.adjust(os_window_aspect());

	if (!entity_exist(editor.selected_entity)) {
	    editor.selected_entity = SV_ENTITY_NULL;
	}

	if (egui_begin()) {

	    if (!editor.camera_focus && there_is_scene() && dev.display_windows) {

		// Window management
		{
		    gui_push_id(dev.gui, "WINDOW MANAGER");

		    gui_xbounds(dev.gui, GuiCoord::Pixel(5.f), GuiCoord::Pixel(150.f));
		    gui_ybounds(dev.gui, GuiCoord::IPixel(30.f), GuiAlign_Top, GuiDim::Adjust());

		    gui_push_style(dev.gui, GuiStyle_ContainerColor, Color::Red(50u));
		    gui_push_style(dev.gui, GuiStyle_ButtonColor, Color::White());
		    gui_push_style(dev.gui, GuiStyle_ButtonHotColor, Color::Red());
		    
		    gui_begin_container(dev.gui, 0u, GuiLayout_Flow);

		    if (egui_button("Hierarchy", 0u)) {
			gui_show_window(dev.gui, "Hierarchy");
		    }
		    if (egui_button("Inspector", 1u)) {
			gui_show_window(dev.gui, "Inspector");
		    }
		    if (egui_button("Asset Browser", 2u)) {
			gui_show_window(dev.gui, "Asset Browser");
		    }
		    if (egui_button("Scene Manager", 3u)) {
			gui_show_window(dev.gui, "Scene Manager");
		    }
		    
		    gui_end_container(dev.gui);

		    gui_pop_style(dev.gui, 3u);
		    
		    gui_pop_id(dev.gui);
		}
		
		display_entity_hierarchy();
		display_entity_inspector();
		display_asset_browser();
		display_scene_settings();

		// MENU
		
		gui_push_id(dev.gui, "MENU");
		    
		if (gui_begin_menu_item(dev.gui, "Game", 0u, GuiLayout_Flow)) {

		    if (dev.game_state == GameState_Edit) {

			if (egui_button("Play", 0u)) {
			    dev.next_game_state = GameState_Play;
			}
		    }
		    else if (dev.game_state == GameState_Play) {

			if (egui_button("Stop", 0u)) {
			    dev.next_game_state = GameState_Edit;
			}
		    }

		    if (egui_button("Clear Scene", 1u)) {
			clear_scene();
		    }

		    gui_end_menu_item(dev.gui);
		}

		if (gui_begin_menu_item(dev.gui, "View", 1u, GuiLayout_Flow)) {

		    // TODO: create specific function
		    egui_comp_bool("Colisions", 0u, &dev.draw_collisions);

		    gui_end_menu_item(dev.gui);
		}

		gui_pop_id(dev.gui);
	    }
	    else {
		// TODO
	    }

	    egui_end();
	}

	if (dev.debug_draw && there_is_scene()) {

	    control_camera();
	}

	if (input.unused && there_is_scene()) {

	    // Entity selection
	    if (input.mouse_buttons[MouseButton_Left] == InputState_Released)
		select_entity();

	    update_gizmos();
	}
    }

    void _editor_draw()
    {
	CommandList cmd = graphics_commandlist_get();
		    
	if (there_is_scene()) {

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

	// Draw gui
	gui_draw(dev.gui, cmd);
    }

}

#endif
