#if SV_DEV

#include "renderer.h"
#include "dev.h"

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
	AssetElementType_Directory,
    };

    struct AssetElement {
	char name[FILENAME_SIZE];
	AssetElementType type;
    };

    struct AssetBrowserInfo {
	std::string filepath;
	List<AssetElement> elements;
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

	    Transform trans = get_entity_transform(engine.scene, editor.selected_entity);
	    v3_f32 position = trans.getLocalPosition();

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

		Transform trans = get_entity_transform(engine.scene, editor.selected_entity);
		v3_f32 position = trans.getLocalPosition();
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

		trans.setPosition(position);
	    }
	}
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

	    Color color = ((info.object == GizmosObject_AxisY) ? (info.focus ? Color::Silver() : Color::Lime()) : Color::Green());
	    draw_debug_line(position, position + v3_f32::up() * GIZMOS_SIZE, color, cmd);

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

		//MeshComponent& m = *reinterpret_cast<MeshComponent*>(comp);
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
		egui_comp_drag_f32("Far", 2u, &cam.far, far_adv, far_min, far_max);
		if (egui_comp_drag_f32("Dimension", 3u, &dimension, 0.01f, 0.01f)) {
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

	    for (ComponentView<MeshComponent> v : meshes) {

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

	    for (ComponentView<SpriteComponent> v : sprites) {

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
	    
	    egui_end_window();
	}
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

			auto pair = get_component_by_index(engine.scene, selected, comp_index);
			CompID comp_id = pair.first;
			BaseComponent* comp = pair.second;

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

	    {
		constexpr f32 WIDTH = 80.f;
		
		gui_begin_container(gui, 0u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(0.f), GuiCoord::IPixel(33.f));

		if (info.filepath.size()) {

		    // TODO: Adjust using name size
		    f32 width = gui_parent_bounds(gui).z * f32(os_window_size().x);

		    u32 folder_count = 1u;
		    for (char c : info.filepath)
			if (c == '/') ++folder_count;

		    u32 count = SV_MIN(SV_MAX(u32(width / WIDTH), 1u), folder_count);
		    SV_ASSERT(count != 0u);
		    --count;

		    f32 offset = width - f32(count) * WIDTH;

		    const char* folder_offset = info.filepath.c_str();
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

			f32 x = offset + (count - i) * WIDTH;
			if (gui_button(gui, folder_name, i, GuiCoord::IPixel(x), GuiCoord::IPixel(x - WIDTH), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f)))
			{

			    if (!update_browser) {

				const char* end = info.filepath.c_str();

				i = count - i;
				while (i) {

				    if (*end == '/') {
					--i;
				    }

				    ++end;
				}

				next_filepath = info.filepath.substr(end - info.filepath.c_str());
				update_browser = true;
				break;
			    }
			}
		    }
		}

		gui_end_container(gui);
	    }

	    {
		gui_begin_container(gui, 1u, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(40.f), GuiCoord::Relative(0.f));

		gui_begin_grid(gui, u32(info.elements.size()), 150.f, 5.f, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));

		foreach(i, info.elements.size()) {

		    const AssetElement& e = info.elements[i];

		    // TODO: ignore unused elements
		    gui_begin_grid_element(gui, 69u + i);

		    if (gui_button(gui, nullptr, 0u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f),
				   GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), editor.style.button_style)) {

			if (e.type == AssetElementType_Directory && !update_browser) {

			    update_browser = true;
			    next_filepath = info.filepath + e.name + '/';
			}
		    }

		    if (e.type != AssetElementType_Directory) {

			AssetPackage pack;
			size_t size = info.filepath.size() + strlen(e.name);
			SV_ASSERT(size < AssetPackage::MAX_SIZE);

			memcpy(pack.filepath, info.filepath.data(), info.filepath.size());
			memcpy(pack.filepath + info.filepath.size(), e.name, strlen(e.name));
			pack.filepath[size] = '\0';

			gui_send_package(gui, &pack, sizeof(AssetPackage), ASSET_BROWSER_PACKAGE);

		    }

		    gui_text(gui, e.name, 1u, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(0.2f));

		    gui_end_grid_element(gui);
		}

		gui_end_grid(gui);
		gui_end_container(gui);
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
		
		while (!file_exists(next_filepath.c_str()) && next_filepath.size()) {
		    
		    next_filepath.pop_back();
		    while (next_filepath.size() && next_filepath.back() != '/') {
			next_filepath.pop_back();
		    }
		}
		
		// Clear browser data
		info.elements.clear();

		FolderIterator it;
		FolderElement e;

		Result res = folder_iterator_begin(next_filepath.c_str(), &it, &e);

		if (result_okay(res)) {

		    do {
		    
			AssetElement element;

			// Select element type
			if (e.is_file) {
			    element.type = AssetElementType_Unknown;
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

		    SV_LOG_ERROR("Can't create asset browser content at '%s': %s", next_filepath.c_str(), result_str(res));
		}
		

		info.filepath = std::move(next_filepath);
		info.last_update = timer_now();
	    }
	    
	    egui_end_window();
	}
	    }

	    void update_editor()
	    {
		// CHANGE EDITOR MODE
		if (dev.game_state != dev.next_game_state) {

		    switch (dev.next_game_state) {

		    case GameState_Edit:
		    {
			SV_LOG_INFO("Starting edit state");
			// TODO: Handle error
			std::string name = engine.scene->name;
			Result res = close_scene(engine.scene);
			res = initialize_scene(&engine.scene, name.c_str());

			if (result_fail(res)) {
			    SV_LOG_ERROR("Can't init the scene '%s': %s", name.c_str(), result_str(res));
			}
			
			dev.debug_draw = true;
		    } break;

		    case GameState_Play:
		    {
			SV_LOG_INFO("Starting play state");
			
			if (dev.game_state == GameState_Edit) {

			    char filepath[FILEPATH_SIZE];

			    if (user_get_scene_filepath(engine.scene->name.c_str(), filepath)) {

				Result res = save_scene(engine.scene, filepath);
				if (result_fail(res))
				    SV_LOG_ERROR("Can't save the scene '%s': %s", engine.scene->name.c_str(), result_str(res));
			    }
			    else
				SV_LOG_INFO("The scene '%s' is not serializable (Specified by the user)", engine.scene->name.c_str());

			    dev.debug_draw = false;
			}
		    } break;

		    case GameState_Pause:
		    {
			SV_LOG_INFO("Game paused");
		    } break;
			
		    }
		    
		    dev.game_state = dev.next_game_state;
		}
		
		// KEY SHORTCUTS
		if (dev.game_state != GameState_Play) {

		    if (input.keys[Key_F11] == InputState_Pressed) {

			if (input.keys[Key_Control] && input.keys[Key_Alt])
			    engine.close_request = true;
			else
			    dev.next_game_state = GameState_Play;
		    }
		    if (input.keys[Key_F10] == InputState_Pressed) {
			
			os_window_set_fullscreen(os_window_state() != WindowState_Fullscreen);
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
		else {

		    if (input.keys[Key_F11] == InputState_Pressed) {
			dev.next_game_state = GameState_Edit;
		    }
		}

		// Adjust camera
		dev.camera.adjust(os_window_aspect());

		if (!entity_exist(engine.scene, editor.selected_entity)) {
		    editor.selected_entity = SV_ENTITY_NULL;
		}

		if (egui_begin()) {

		    if (!editor.camera_focus && engine.scene != nullptr && dev.game_state != GameState_Play) {
			display_entity_hierarchy();
			display_entity_inspector();
			display_asset_browser();
		    }
		    else {
			// TODO
		    }

		    if (dev.game_state != GameState_Play) {
			
			gui_push_id(dev.gui, "MENU");
		    
			if (gui_begin_menu_item(dev.gui, "Game", 0u)) {

			    if (egui_button("Play", 0u)) {
				dev.next_game_state = GameState_Play;
			    }

			    if (egui_button("Clear Scene", 1u)) {
				clear_scene(engine.scene);
			    }

			    gui_end_menu_item(dev.gui);
			}

			if (gui_begin_menu_item(dev.gui, "View", 1u)) {

			    if (egui_button("Hierarchy", 0u)) {
				gui_show_window(dev.gui, "Hierarchy");
			    }
			    if (egui_button("Inspector", 1u)) {
				gui_show_window(dev.gui, "Inspector");
			    }
			    if (egui_button("Asset Browser", 2u)) {
				gui_show_window(dev.gui, "Asset Browser");
			    }

			    gui_end_menu_item(dev.gui);
			}

			gui_pop_id(dev.gui);
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

		    update_gizmos();
		}
	    }

	    void draw_editor()
	    {
		CommandList cmd = graphics_commandlist_get();
		    
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
		    draw_gizmos(gfx.offscreen, cmd);

		    end_debug_batch(true, false, dev.camera.view_projection_matrix, cmd);
		}

		// Draw gui
		gui_draw(dev.gui, cmd);
	    }

	}

#endif