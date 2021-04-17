#include "core/gui.h"

#include "core/renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI& gui = *gui_

namespace sv {

    enum GuiHeader : u32 {
	GuiHeader_Widget,
	GuiHeader_EndOfParent,
	GuiHeader_StylePush,
	GuiHeader_StylePop,
	GuiHeader_SameLine
    };
    
    enum GuiWidgetType : u32 {
	GuiWidgetType_None,
	GuiWidgetType_Container,
	GuiWidgetType_Window,
	GuiWidgetType_Popup,
	GuiWidgetType_Button,
	GuiWidgetType_Slider,
	GuiWidgetType_Label,
	GuiWidgetType_Checkbox,
	GuiWidgetType_Drag,
	GuiWidgetType_MenuItem,
	GuiWidgetType_MenuContainer,
	GuiWidgetType_Package,
	GuiWidgetType_Reciver,
    };

    enum GuiPrimitive : u32 {
	GuiPrimitive_f32,
	GuiPrimitive_u8,
    };

    //////////////////////////////// STYLE STRUCTS /////////////////////////////////////

    struct GuiContainerStyle {
	Color color = Color::Gray(100u);
    };

    struct GuiWindowStyle {
	Color color = Color::Gray(170u, 20u);
	Color decoration_color = Color::Gray(100u, 200u);
	Color outline_color = Color::Gray(50u, 200u);
	f32 decoration_height = 0.04f;
	f32 outline_size = 0.001f;
	f32 min_width = 0.1f;
	f32 min_height = 0.f;
    };

    struct GuiPopupStyle {
	Color color = Color::Gray(140u);
    };

    struct GuiButtonStyle {
	Color color = Color::Gray(200u, 150u);
	Color hot_color = Color::White(230u);
	Color text_color = Color::Black();
    };

    struct GuiSliderStyle {
	Color color = Color::Gray(5u);
	Color button_color = Color::White();
	v2_f32 button_size = { 0.01f, 0.03f };
    };

    struct GuiLabelStyle {
	Color text_color = Color::White();
	TextAlignment text_alignment = TextAlignment_Center;
	Color background_color = Color::Gray(100u, 0u);
    };

    struct GuiCheckboxStyle {
	Color button_color = Color::Gray(100u);
	Color background_color = Color::White();
	GuiCheckboxShape shape = GuiCheckboxShape_Quad;
	f32 shape_size_mult = 0.7f;
    };

    struct GuiDragStyle {
	Color text_color = Color::Black();
	Color background_color = Color::White();
    };

    struct GuiMenuItemStyle {
	Color color = Color::White();
	Color hot_color = Color::Gray(200u);
	Color text_color = Color::Black();
    };

    struct GuiFlowLayoutStyle {
	f32 x0 = 0.1f;
	f32 x1 = 0.9f;
	f32 sub_x0 = 0.f;
	f32 sub_x1 = 1.f;
    };

    struct GuiStyleData {
	GuiContainerStyle container;
	GuiWindowStyle window;
	GuiPopupStyle popup;
	GuiButtonStyle button;
	GuiSliderStyle slider;
	GuiLabelStyle label;
	GuiCheckboxStyle checkbox;
	GuiDragStyle drag;
	GuiMenuItemStyle menuitem;
	GuiFlowLayoutStyle flow_layout;
    };

    ///////////////////////////////// RAW STRUCTS ///////////////////////////////////////

    enum GuiCoordType : u8 {
	GuiCoordType_Coord,
	GuiCoordType_Dim,
    };
    
    struct Raw_Coords {

	struct Coord {
	    GuiCoord _0;
	    GuiCoord _1;
	};

	struct Dim {
	    GuiCoord c;
	    GuiAlign align;
	    GuiDim d;
	};
	
	GuiCoordType xtype;
	Coord xcoord;
	Dim xdim;
	GuiCoordType ytype;
	Coord ycoord;
	Dim ydim;
    };

    struct Raw_Button {
	const char* text;
    };

    struct Raw_Drag {
	GuiPrimitive primitive;
	u64 value;
	u64 min;
	u64 max;
	u64 adv;
    };

    struct Raw_Checkbox {
	bool value;
    };

    struct Raw_Label {
	const char* text;
    };

    struct Raw_Container {
    };

    struct Raw_Window {
    };

    struct Raw_Popup {
	v4_f32 bounds;
    };

    struct Raw_MenuItem {
	const char* text;
	bool active;
    };

    struct Raw_Package {
	const void* package;
	u32 package_size;
	u64 package_id;
    };

    struct Raw_Reciver {
	u64 package_id;
    };

    ///////////////////////////////// WIDGET STRUCTS ///////////////////////////////////

    struct GuiIndex {
	u32 index;
	u64 id;

	GuiIndex() = default;
	GuiIndex(u32 index, u64 id) : index(index), id(id) {}
    };

    struct GuiParentInfo {

	u32 widget_offset;
	u32 widget_count;

	u32 parent_index;
	u32 menu_item_count;
	bool has_vertical_scroll;
	v4_f32 child_bounds;

	f32 vertical_offset;
	f32 min_y;
	f32 max_y;

	f32 vertical_amplitude; // The total vertical size that ocupe the childs
	f32 horizontal_amplitude; // The total horizontal size that ocupe the childs

	GuiLayout layout;
	union {
	    struct {
		f32 yoff;
		u32 same_line_count;
		u32 last_same_line;
	    } flow;
	};
	
    };

    struct GuiWindowState {

	v4_f32 bounds;
	bool show;
	std::string title;
	u64 id;

    };

    struct GuiWidget {

	v4_f32 bounds;
	u32 index;
	u64 id;
	GuiWidgetType type;
	Raw_Coords raw_coords;

	union Widget {

	    // I hate u
	    Widget() : window({}) {}
			
	    struct {
		const char* text;
		bool hot;
		bool pressed;
		GuiButtonStyle style;
	    } button;

	    struct {
		GuiPrimitive primitive;
		u64 value;
		u64 min;
		u64 max;
		u64 adv;
		GuiDragStyle style;
	    } drag;

	    struct {
		bool value;
		GuiCheckboxStyle style;
	    } checkbox;

	    struct {
		const char* text;
		GuiLabelStyle style;
	    } label;

	    struct {
		const char* text;
		GuiMenuItemStyle style;
		bool hot;
		bool active;
	    } menu_item;

	    struct {
		GuiParentInfo parent_info;
	    } menu_container;

	    struct {
		GuiParentInfo parent_info;
		GuiContainerStyle style;
	    } container;

	    struct {
		GuiParentInfo parent_info;
		GuiWindowState* state;
		GuiWindowStyle style;
	    } window;

	    struct {
		GuiParentInfo parent_info;
		bool close_request;
		GuiPopupStyle style;
	    } popup;

	    struct {
		void* data;
		u32 size;
		u64 package_id;
	    } package;

	    struct {
		u64 package_id;
		bool recived;
	    } reciver;

	} widget;

	GuiWidget() = default;

    };

    struct WritingParentInfo {
	GuiWidget* parent;
	GuiLayout layout;
    };

    struct GuiLayoutData {
	GuiLayout type;
    };

    struct GUI {

	// CONTEXT
	    
	RawList buffer;
	List<WritingParentInfo> parent_stack;
	
	GuiIndex current_focus;
	Raw_Coords free_bounds;
	
	List<u64> ids;
	u32 style_count;
	u64 current_id;

	struct {
	    u64 id;
	    GuiWidgetType type;
	}last;
	
	// STATE

	List<GuiWidget> widgets;
	List<GuiIndex> indices;
	
	RawList style_stack;
	
	u32 root_menu_count;
	
	struct {
	    GuiWidgetType type;
	    u64 id;
	    u32 action;
	} focus;

	bool catch_input;

	GuiStyleData style;
	GuiStyleData temp_style;
	
	// STATIC STATE
	struct {
	    std::unordered_map<u64, GuiWindowState> window;
	    std::unordered_map<u64, bool> checkbox;
	} static_state;

	// SETTINGS

	f32 scale;
	v2_f32 resolution;
	f32 aspect;
	v2_f32 mouse_position;
	
	// AUX

	v2_f32 begin_position;
	u64 package_id;		
	RawList package_data;
	
	// TEMP

	struct {
	    u32 element_count;
	    u32 columns;
	    f32 element_size;
	    u32 element_index;
	    f32 xoff;
	    f32 padding;
	} grid;

    };


    bool gui_create(u64 hashcode, GUI** pgui)
    {
	GUI& gui = *new GUI();

	gui.package_id = 0u;

	// Get last static state
	{
	    Archive archive;
	    bool res = bin_read(hash_string("GUI STATE"), archive);
	    if (res) {

		u32 window_count;
		archive >> window_count;

		foreach(i, window_count) {

		    GuiWindowState s;
		    archive >> s.title >> s.show >> s.bounds;
		    s.id = hash_string(s.title.c_str());
		    gui.static_state.window[s.id] = s;
		}
	    }
	    else {
		SV_LOG_WARNING("Can't load the last gui static state");
	    }
	}

	*pgui = (GUI*)& gui;

	return true;
    }

    bool gui_destroy(GUI* gui_)
    {
	PARSE_GUI();

	// Save static state
	{
	    Archive archive;

	    archive << u32(gui.static_state.window.size());

	    for (const auto& it : gui.static_state.window) {

		const GuiWindowState& s = it.second;
		archive << s.title << s.show << s.bounds;
	    }

	    bool res = bin_write(hash_string("GUI STATE"), archive);

	    if (!res) {

		SV_LOG_ERROR("Can't save the gui static state");
	    }
	}

	delete& gui;
	return true;
    }

    ////////////////////////////////////////// UTILS ////////////////////////////////////////////////

    SV_AUX void write_buffer(GUI& gui, const void* data, size_t size)
    {
	gui.buffer.write_back(data, size);
    }

    template<typename T>
    SV_AUX void write_buffer(GUI& gui, const T& t)
    {
	write_buffer(gui, &t, sizeof(T));
    }

    SV_AUX void write_text(GUI& gui, const char* text)
    {

	if (text == nullptr) {

	    char null = '\0';
	    write_buffer(gui, &null, 1u);
	}
	else {
	    size_t text_size = strlen(text);
	    write_buffer(gui, text, text_size + 1u);
	}
    }

    SV_INLINE static void set_focus(GUI& gui, GuiWidgetType type, u64 id, u32 action = 0u)
    {
	gui.focus.type = type;
	gui.focus.id = id;
	gui.focus.action = action;
    }

    SV_INLINE static void free_focus(GUI& gui)
    {
	gui.focus.type = GuiWidgetType_None;
    }

    SV_AUX bool ignore_scroll(GuiWidgetType type)
    {
	switch (type)
	{
	case GuiWidgetType_MenuItem:
	case GuiWidgetType_MenuContainer:
	case GuiWidgetType_Window:
	case GuiWidgetType_Popup:
	    return true;

	default:
	    return false;
	}
    }
    
    SV_INLINE static bool is_parent(GuiWidgetType type)
    {
	switch (type)
	{
	case GuiWidgetType_MenuContainer:
	case GuiWidgetType_Container:
	case GuiWidgetType_Window:
	case GuiWidgetType_Popup:
	    return true;

	default:
	    return false;
	}
    }

    SV_INLINE static bool is_parent(const GuiWidget& w)
    {
	return is_parent(w.type);
    }

    SV_INLINE static GuiParentInfo* get_parent_info(GUI& gui, GuiWidget& parent)
    {
	if (is_parent(parent)) {

	    return reinterpret_cast<GuiParentInfo*>(&parent.widget);
	}
	else return nullptr;
    }
    SV_INLINE static const GuiParentInfo* get_parent_info(const GUI& gui, const GuiWidget& parent)
    {
	if (is_parent(parent)) {

	    return reinterpret_cast<const GuiParentInfo*>(&parent.widget);
	}
	else return nullptr;
    }

    SV_INLINE static GuiParentInfo* get_parent_info(GUI& gui, const GuiIndex& index)
    {
	GuiWidget& w = gui.widgets[index.index];
	return get_parent_info(gui, w);
    }
    SV_AUX GuiParentInfo* get_parent_info(GUI& gui, u32 index)
    {
	GuiWidget& w = gui.widgets[index];
	return get_parent_info(gui, w);
    }

    SV_AUX void write_widget(GUI& gui, GuiWidgetType type, u64 id, void* data, GuiParentInfo* parent_info = nullptr, GuiLayout layout = GuiLayout_Flow)
    {
	write_buffer(gui, GuiHeader_Widget);
	write_buffer(gui, type);
	write_buffer(gui, id);

	// Write parent raw data
	if (is_parent(type)) {

	    write_buffer(gui, layout);
	    
	    if (parent_info) {

		write_buffer(gui, parent_info->vertical_amplitude);
		write_buffer(gui, parent_info->horizontal_amplitude);
		write_buffer(gui, parent_info->vertical_offset);
		write_buffer(gui, parent_info->has_vertical_scroll);
	    }
	    else {
		write_buffer(gui, 0.f);
		write_buffer(gui, 0.f);
		write_buffer(gui, 0.f);
		write_buffer(gui, false);
	    }
	}
	
	// Write layout info
	{
	    GuiLayout parent_layout;
	    if (gui.parent_stack.empty())
		parent_layout = GuiLayout_Free;
	    else {

		parent_layout = gui.parent_stack.back().layout;
	    }

	    if (parent_layout == GuiLayout_Free)
		write_buffer(gui, gui.free_bounds);
	}

	switch (type)
	{
	case GuiWidgetType_Container:
	{
	    Raw_Container* raw = (Raw_Container*)data;
	    write_buffer(gui, *raw);
	}
	break;

	case GuiWidgetType_Window:
	{
	    Raw_Window* raw = (Raw_Window*)data;
	    write_buffer(gui, *raw);
	}
	break;

	case GuiWidgetType_Popup:
	{
	    Raw_Popup* raw = (Raw_Popup*)data;
	    write_buffer(gui, *raw);
	}
	break;

	case GuiWidgetType_Button:
	{
	    Raw_Button* raw = (Raw_Button*)data;

	    write_text(gui, raw->text);
	}
	break;

	case GuiWidgetType_Slider:
	    break;

	case GuiWidgetType_Label:
	{
	    Raw_Label* raw = (Raw_Label*)data;

	    write_text(gui, raw->text);
	}
	break;

	case GuiWidgetType_Checkbox:
	{
	    Raw_Checkbox* raw = (Raw_Checkbox*)data;
	    write_buffer(gui, *raw);
	}
	break;

	case GuiWidgetType_Drag:
	{
	    Raw_Drag* raw = (Raw_Drag*)data;
	    write_buffer(gui, *raw);
	}
	break;

	case GuiWidgetType_MenuItem:
	{
	    Raw_MenuItem* raw = (Raw_MenuItem*)data;

	    write_text(gui, raw->text);
	    write_buffer(gui, raw->active);
	}
	break;

	case GuiWidgetType_Package:
	{
	    Raw_Package* raw = (Raw_Package*)data;

	    write_buffer(gui, raw->package_size);
	    if (raw->package_size) {
		write_buffer(gui, raw->package, raw->package_size);
	    }

	    write_buffer(gui, raw->package_id);
	}
	break;

	case GuiWidgetType_Reciver:
	{
	    Raw_Reciver* raw = (Raw_Reciver*)data;
	    write_buffer(gui, *raw);
	}
	break;

	default:
	    break;
	}

	gui.last.id = id;
	gui.last.type = type;
    }
    
    SV_AUX f32 compute_coord(const GUI& gui, const GuiCoord& coord, f32 resolution, f32 parent_coord, f32 parent_dimension)
    {
	f32 res = 0.5f;

	// Centred coord
	switch (coord.constraint)
	{
	case GuiCoordConstraint_Relative:
	    res = coord.value * parent_dimension;
	    break;

	case GuiCoordConstraint_Center:
	    res = 0.5f * parent_dimension;
	    break;

	case GuiCoordConstraint_Pixel:
	case GuiCoordConstraint_InversePixel:
	    res = coord.value / resolution;
	    break;

	}

	// Inverse coord
	if (coord.constraint == GuiCoordConstraint_InversePixel) {
	    res = parent_dimension - res;
	}

	// Parent offset
	res += (parent_coord - parent_dimension * 0.5f);

	return res;
    }

    SV_AUX f32 compute_dimension(const GUI& gui, const GuiDim& dim, f32 resolution, f32 parent_dimension, f32 aspect_dimension, f32 adjust_value, bool xaxis)
    {
	f32 res = 0.5f;

	// Centred coord
	switch (dim.constraint)
	{
	case GuiDimConstraint_Relative:
	    res = dim.value * parent_dimension;
	    break;

	case GuiDimConstraint_Aspect:
	    res = dim.value * aspect_dimension * (xaxis ? (1.f / gui.aspect) : gui.aspect);
	    break;

	case GuiDimConstraint_Pixel:
	    res = dim.value / resolution;
	    break;

	case GuiDimConstraint_Absolute:
	    res = dim.value;
	    break;

	case GuiDimConstraint_Scale:
	    res = dim.value * gui.scale;
	    break;

	case GuiDimConstraint_Adjust:
	    res = dim.value * adjust_value;
	    break;
	    
	}
	
	return res;
    }
    
    SV_AUX v2_f32 compute_axis_bounds(const GUI& gui, GuiWidget* widget, bool xaxis, const v4_f32 parent_bounds, f32 aspect_dimension)
    {
	f32 _0, _1;

	const Raw_Coords& raw = widget->raw_coords;

	if (xaxis) {
	    if (raw.xtype == GuiCoordType_Coord) {
		_0 = compute_coord(gui, raw.xcoord._0, gui.resolution.x, parent_bounds.x, parent_bounds.z);
		_1 = compute_coord(gui, raw.xcoord._1, gui.resolution.x, parent_bounds.x, parent_bounds.z);
	    }
	    else {

		f32 adjust_value = 0.f;

		if (is_parent(*widget)) {

		    adjust_value = get_parent_info(gui, *widget)->horizontal_amplitude;
		}
		
		_0 = compute_coord(gui, raw.xdim.c, gui.resolution.x, parent_bounds.x, parent_bounds.z);
		_1 = compute_dimension(gui, raw.xdim.d, gui.resolution.x, parent_bounds.z, aspect_dimension, adjust_value, true);

		switch (raw.xdim.align) {
		    
		case GuiAlign_Center:
		    _0 -= _1 * 0.5f;
		    break;

		case GuiAlign_Right:
		    _0 -= _1;
		    break;
		    
		}

		_1 = _0 + _1;
	    }
	}
	else {
	    if (raw.ytype == GuiCoordType_Coord) {
		_0 = compute_coord(gui, raw.ycoord._0, gui.resolution.y, parent_bounds.y, parent_bounds.w);
		_1 = compute_coord(gui, raw.ycoord._1, gui.resolution.y, parent_bounds.y, parent_bounds.w);
	    }
	    else {

		f32 adjust_value = 0.f;

		if (is_parent(*widget)) {

		    adjust_value = get_parent_info(gui, *widget)->vertical_amplitude;
		}
		
		_0 = compute_coord(gui, raw.ydim.c, gui.resolution.y, parent_bounds.y, parent_bounds.w);
		_1 = compute_dimension(gui, raw.ydim.d, gui.resolution.y, parent_bounds.w, aspect_dimension, adjust_value, false);

		switch (raw.ydim.align) {
		    
		case GuiAlign_Center:
		    _0 -= _1 * 0.5f;
		    break;

		case GuiAlign_Top:
		    _0 -= _1;
		    break;
		    
		}

		_1 = _0 + _1;
	    }
	}

	if (_1 < _0) std::swap(_0, _1);

	return { _0, _1, };
    }
    
    SV_INTERNAL v4_f32 compute_widget_bounds(const GUI& gui, GuiWidget* widget, GuiWidget* parent)
    {
	// TODO: Debug assertions

	v4_f32 parent_bounds;
	if (parent) {

	    const GuiParentInfo* parent_info = get_parent_info(gui, *parent);
	    parent_bounds = parent_info->child_bounds;
	}
	else parent_bounds = { 0.5f, 0.5f, 1.f, 1.f };

	v2_f32 xaxis, yaxis;

	const Raw_Coords& raw = widget->raw_coords;
	
	if (raw.xtype == GuiCoordType_Dim && raw.xdim.d.constraint == GuiDimConstraint_Aspect) {
	    
	    yaxis = compute_axis_bounds(gui, widget, false, parent_bounds, 0.5f);
	    xaxis = compute_axis_bounds(gui, widget, true, parent_bounds, yaxis.y - yaxis.x);
	}
	else {

	    xaxis = compute_axis_bounds(gui, widget, true, parent_bounds, 0.5f);
	    yaxis = compute_axis_bounds(gui, widget, false, parent_bounds, xaxis.y - xaxis.x);
	}


	f32 w = xaxis.y - xaxis.x;
	f32 h = yaxis.y - yaxis.x;
	f32 x = xaxis.x + w * 0.5f;
	f32 y = yaxis.x + h * 0.5f;

	SV_ASSERT(w >= 0.f && h >= 0.f);
	
	return { x, y, w, h };
    }

    SV_AUX bool mouse_in_bounds(const GUI& gui, const v4_f32& bounds)
    {
	return abs(bounds.x - gui.mouse_position.x) <= bounds.z * 0.5f && abs(bounds.y - gui.mouse_position.y) <= bounds.w * 0.5f;
    }

    SV_AUX v4_f32 compute_window_decoration_bounds(const GUI& gui, const GuiWidget& w)
    {
	auto& window = w.widget.window;

	v4_f32 bounds;
	bounds.x = w.bounds.x;
	bounds.y = w.bounds.y + w.bounds.w * 0.5f + window.style.decoration_height * 0.5f + window.style.outline_size * gui.aspect;
	bounds.z = w.bounds.z + window.style.outline_size * 2.f;
	bounds.w = window.style.decoration_height;
	return bounds;
    }

    SV_AUX v4_f32 compute_window_closebutton_bounds(const GUI& gui, const GuiWidget& w, const v4_f32 decoration_bounds)
    {
	v4_f32 bounds;
	bounds.x = decoration_bounds.x + decoration_bounds.z * 0.5f - decoration_bounds.w * 0.5f / gui.aspect;
	bounds.y = decoration_bounds.y;
	bounds.z = 0.006f;
	bounds.w = 0.006f * gui.aspect;
		
	return bounds;
    }

    static void update_widget(GUI& gui, const GuiIndex& index)
    {
	GuiParentInfo* parent_info = get_parent_info(gui, index);

	// If it is a parent, update childs
	if (parent_info) {

	    GuiIndex* it = gui.indices.data() + parent_info->widget_offset;
	    GuiIndex* end = it + parent_info->widget_count;

	    // Update childs parents
	    while (it != end) {

		GuiParentInfo* p = get_parent_info(gui, *it);
		if (p) {

		    update_widget(gui, *it);
		    it += p->widget_count;
		}

		++it;
	    }

	    // Update normal widgets
	    it = gui.indices.data() + parent_info->widget_offset;

	    while (it != end) {

		GuiParentInfo* p = get_parent_info(gui, *it);
		if (p) {

		    it += p->widget_count;
		}
		else {

		    update_widget(gui, *it);
		}

		++it;
	    }
	}

	GuiWidget& w = gui.widgets[index.index];

	switch (w.type)
	{

	case GuiWidgetType_Button:
	{
	    auto& button = w.widget.button;

	    if (input.unused && mouse_in_bounds(gui, w.bounds)) {

		button.hot = true;

		// TODO: Set focus and press when is released

		if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {
		    button.pressed = true;
		    input.unused = false;
		}
	    }
	}
	break;

	case GuiWidgetType_Drag:
	case GuiWidgetType_Checkbox:
	case GuiWidgetType_MenuItem:
	case GuiWidgetType_Package:
	{
	    if (input.unused) {
				
		if (mouse_in_bounds(gui, w.bounds)) {

		    if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

			set_focus(gui, w.type, index.id);
			input.unused = false;
		    }
		}
	    }

	    if (w.type == GuiWidgetType_MenuItem) {
				
		if (mouse_in_bounds(gui, w.bounds)) {
		    w.widget.menu_item.hot = true;
		}
	    }
	}
	break;

	case GuiWidgetType_MenuContainer:
	{
	    bool any = false;

	    foreach(i, MouseButton_MaxEnum) {
		if (input.mouse_buttons[i] == InputState_Pressed) {
		    any = true;
		    break;
		}
	    }

	    if (any && !mouse_in_bounds(gui, w.bounds)) {

		GuiWidget* menu = &gui.widgets[w.index - 1u];
		menu->widget.menu_item.active = false;
	    }
	}
	break;

	case GuiWidgetType_Popup:
	{
	    auto& popup = w.widget.popup;

	    bool any = false;

	    foreach(i, MouseButton_MaxEnum) {
		if (input.mouse_buttons[i] == InputState_Pressed) {
		    any = true;
		    break;
		}
	    }

	    if (any && !mouse_in_bounds(gui, w.bounds)) {

		popup.close_request = true;
	    }
	}
	break;

	case GuiWidgetType_Window:
	{
	    auto& window = w.widget.window;
	    GuiWindowState& state = *window.state;

	    if (input.unused) {

		InputState button = input.mouse_buttons[MouseButton_Left];

		v4_f32 decoration = compute_window_decoration_bounds(gui, w);

		if (mouse_in_bounds(gui, decoration)) {

		    input.unused = false;
					
		    if (button == InputState_Pressed) {

			v4_f32 closebutton_bounds = compute_window_closebutton_bounds(gui, w, decoration);

			if (mouse_in_bounds(gui, closebutton_bounds)) 
			    set_focus(gui, GuiWidgetType_Window, state.id, 6u);
			else {
			    gui.begin_position = v2_f32(window.state->bounds.x, window.state->bounds.y) - gui.mouse_position;
			    set_focus(gui, GuiWidgetType_Window, state.id, 0u);
			}
		    }
		}
		else if (button == InputState_Pressed) {
					
		    const v4_f32& content = window.state->bounds;

		    constexpr f32 SELECTION_SIZE = 0.02f;

		    bool right = mouse_in_bounds(gui, { content.x + content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui.aspect });
		    bool left = mouse_in_bounds(gui, { content.x - content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui.aspect });
		    bool bottom = mouse_in_bounds(gui, { content.x, content.y - content.w * 0.5f, content.z + SELECTION_SIZE * 2.f, SELECTION_SIZE * gui.aspect });

		    u32 action = u32_max;

		    if (right && bottom) {
			action = 4u;
		    }
		    else if (left && bottom) {
			action = 5u;
		    }
		    else if (right) {
			action = 1u;
		    }
		    else if (left) {
			action = 2u;
		    }
		    else if (bottom) {
			action = 3u;
		    }

		    if (action != u32_max) {
			input.unused = false;
			set_focus(gui, GuiWidgetType_Window, state.id, action);
		    }
		}
	    }
	}
	break;

	}


	// Catch the input if the mouse is inside the parent and update scroll wheel
	if (parent_info) {

	    if (!gui.catch_input) {

		gui.catch_input = mouse_in_bounds(gui, w.bounds);
	    }

	    if (input.unused && parent_info->has_vertical_scroll) {

		const v4_f32& scrollable_bounds = parent_info->child_bounds;

		if (input.mouse_wheel != 0.f && mouse_in_bounds(gui, scrollable_bounds)) {

		    parent_info->vertical_offset += input.mouse_wheel * 0.01f;
		    input.unused = false;
		}

		parent_info->vertical_offset = SV_MAX(SV_MIN(parent_info->vertical_offset, parent_info->max_y), parent_info->min_y);
	    }
	}
    }

    static void update_focus(GUI& gui, const GuiIndex& index)
    {
	GuiWidget& w = gui.widgets[index.index];

	switch (w.type)
	{

	case GuiWidgetType_Drag:
	{
	    auto& drag = w.widget.drag;

	    if (input.mouse_buttons[MouseButton_Left] == InputState_None) {
		free_focus(gui);
	    }
	    else {

		switch (drag.primitive) {

		case GuiPrimitive_f32:
		{
		    f32& value = *reinterpret_cast<f32*>(&drag.value);
		    f32 adv = *reinterpret_cast<f32*>(&drag.adv);
		    f32 min = *reinterpret_cast<f32*>(&drag.min);
		    f32 max = *reinterpret_cast<f32*>(&drag.max);
		    
		    value += input.mouse_dragged.x * gui.resolution.x * adv;
		    value = SV_MAX(SV_MIN(value, max), min);
		}
		break;
		
		case GuiPrimitive_u8:
		{
		    u8& value = *reinterpret_cast<u8*>(&drag.value);
		    u8 adv = *reinterpret_cast<u8*>(&drag.adv);
		    u8 min = *reinterpret_cast<u8*>(&drag.min);
		    u8 max = *reinterpret_cast<u8*>(&drag.max);

		    i32 add = (i32)round(input.mouse_dragged.x * gui.resolution.x * f32(adv)); 
		    i32 new_value = (i32)value + add;
		    new_value = SV_MAX(SV_MIN(new_value, i32(max)), i32(min));

		    value = (u8)new_value;
		}
		break;
		    
		}
	    }
	}
	break;

	case GuiWidgetType_Checkbox:
	{
	    auto& cb = w.widget.checkbox;

	    if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

		free_focus(gui);
		cb.value = !cb.value;
	    }
	}
	break;

	case GuiWidgetType_MenuItem:
	{
	    auto& menu = w.widget.menu_item;

	    if (input.mouse_buttons[MouseButton_Left] != InputState_Hold) {

		free_focus(gui);

		if (mouse_in_bounds(gui, w.bounds)) {
		    menu.active = !menu.active;
		}
	    }
	}
	break;

	case GuiWidgetType_Package:
	{
	    auto& p = w.widget.package;

	    InputState mouse = input.mouse_buttons[MouseButton_Left];
	    if (mouse == InputState_Released || mouse == InputState_None) {

		free_focus(gui);
		gui.package_id = p.package_id;
		gui.package_data.reset();
		gui.package_data.write_back(p.data, p.size);

		foreach(i, gui.indices.size()) {

		    GuiWidget* w = &gui.widgets[gui.indices[i].index];
		    if (w->type == GuiWidgetType_Reciver && w->widget.reciver.package_id == p.package_id && mouse_in_bounds(gui, w->bounds)) {
			w->widget.reciver.recived = true;
			break;
		    }
		}
	    }
	}
	break;
		
	case GuiWidgetType_Window:
	{
	    auto& window = w.widget.window;

	    if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

		if (gui.focus.action == 6u) {

		    v4_f32 decoration_bounds = compute_window_decoration_bounds(gui, w);
		    v4_f32 closebutton_bounds = compute_window_closebutton_bounds(gui, w, decoration_bounds);

		    if (mouse_in_bounds(gui, closebutton_bounds)) {
			window.state->show = false;
		    }
		}

		free_focus(gui);
	    }
	    else {

		switch (gui.focus.action)
		{

		case 0u:
		{
		    v2_f32 new_position = gui.mouse_position + gui.begin_position;
		    window.state->bounds.x = new_position.x;
		    window.state->bounds.y = new_position.y;
		}
		break;

		case 1u: // RIGHT
		{
		    f32 width = std::max(gui.mouse_position.x - (window.state->bounds.x - window.state->bounds.z * 0.5f), window.style.min_width);
		    window.state->bounds.x -= (window.state->bounds.z - width) * 0.5f;
		    window.state->bounds.z = width;
		}
		break;

		case 2u: // LEFT
		{
		    f32 width = std::max((window.state->bounds.x + window.state->bounds.z * 0.5f) - gui.mouse_position.x, window.style.min_width);
		    window.state->bounds.x += (window.state->bounds.z - width) * 0.5f;
		    window.state->bounds.z = width;
		}
		break;

		case 3u: // BOTTOM
		{
		    f32 height = std::max((window.state->bounds.y + window.state->bounds.w * 0.5f) - gui.mouse_position.y, window.style.min_height);
		    window.state->bounds.y += (window.state->bounds.w - height) * 0.5f;
		    window.state->bounds.w = height;
		}
		break;

		case 4u: // RIGHT - BOTTOM
		{
		    f32 width = std::max(gui.mouse_position.x - (window.state->bounds.x - window.state->bounds.z * 0.5f), window.style.min_width);
		    window.state->bounds.x -= (window.state->bounds.z - width) * 0.5f;
		    window.state->bounds.z = width;
		    f32 height = std::max((window.state->bounds.y + window.state->bounds.w * 0.5f) - gui.mouse_position.y, window.style.min_height);
		    window.state->bounds.y += (window.state->bounds.w - height) * 0.5f;
		    window.state->bounds.w = height;
		}
		break;

		case 5u: // LEFT - BOTTOM
		{
		    f32 width = std::max((window.state->bounds.x + window.state->bounds.z * 0.5f) - gui.mouse_position.x, window.style.min_width);
		    window.state->bounds.x += (window.state->bounds.z - width) * 0.5f;
		    window.state->bounds.z = width;
		    f32 height = std::max((window.state->bounds.y + window.state->bounds.w * 0.5f) - gui.mouse_position.y, window.style.min_height);
		    window.state->bounds.y += (window.state->bounds.w - height) * 0.5f;
		    window.state->bounds.w = height;
		}
		break;

		}
	    }
	}
	break;

	}
    }

    //////////////////////////////////////////////// BEGIN ///////////////////////////////////////////////////

    void gui_begin(GUI* gui_, f32 scale, f32 width, f32 height)
    {
	PARSE_GUI();

	gui.scale = scale;
	gui.resolution = { width, height };
	gui.aspect = width / height;

	gui.style_count = 0u;
	
	gui.ids.reset();
	gui.parent_stack.reset();
	
	gui.mouse_position = input.mouse_position + 0.5f;
	
	gui.last.id = 0u;
	gui.last.type = GuiWidgetType_None;

	gui.catch_input = false;

	// Reset buffer
	gui.buffer.reset();
    }

    //////////////////////////////////////////////// END ///////////////////////////////////////////////////

    SV_INLINE static void _read(u8*& it, void* data, size_t size)
    {
	memcpy(data, it, size);
	it += size;
    }

    template<typename T>
    SV_INLINE static T _read(u8*& it)
    {
	T t;
	_read(it, &t, sizeof(T));
	return t;
    }

    SV_AUX void compute_layout(GUI& gui, GuiWidget& w, GuiParentInfo* parent_info, u8*& it)
    {
	GuiLayout layout = parent_info ? parent_info->layout : GuiLayout_Free;

	switch (layout) {

	case GuiLayout_Free:
	    w.raw_coords = _read<Raw_Coords>(it);
	    break;

	case GuiLayout_Flow:
	{
	    auto& data = parent_info->flow;
	    auto& style = gui.temp_style.flow_layout;
	    
	    if (!ignore_scroll(w.type)) {

		SV_ASSERT(data.same_line_count != 0u);
		
		if (data.same_line_count == 1u) {

		    f32 x0 = style.x0;
		    f32 x1 = style.x1;
		    if (x1 < x0) std::swap(x0, x1);
		    
		    f32 width = x1 - x0;
		    f32 element_width = width / f32(data.last_same_line);

		    u32 count = data.last_same_line - 1u;
		    u32 end_index = w.index;
		    u32 begin_index = w.index;

		    while(count) {

			SV_ASSERT(begin_index > 1u);
			--begin_index;

			GuiWidget& widget = gui.widgets[begin_index];
			if (!ignore_scroll(widget.type)) {
			    --count;
			}
		    }

		    f32 max_height = 0.f;

		    for (u32 i = begin_index; i <= end_index; ++i) {

			GuiWidget& widget = gui.widgets[i];
			
			auto& coords = widget.raw_coords;

			f32 sub_x0 = x0 + f32(i - begin_index) * element_width;
			f32 sub_x1 = x0 + f32(i - begin_index + 1u) * element_width;
			
			f32 sub_width = sub_x1 - sub_x0;
			sub_x0 = sub_x0 + style.sub_x0 * sub_width;
			sub_x1 = sub_x0 + style.sub_x1 * sub_width;
			
			coords.xtype = GuiCoordType_Coord;
			coords.xcoord._0 = GuiCoord::Relative(sub_x0); 
			coords.xcoord._1 = GuiCoord::Relative(sub_x1); 

			coords.ytype = GuiCoordType_Dim;
			coords.ydim.c = GuiCoord::IPixel(data.yoff);
			coords.ydim.align = GuiAlign_Top;
	    
			switch (widget.type) {

			case GuiWidgetType_Container:
			    coords.ydim.d = GuiDim::Adjust();

			    max_height = SV_MAX(max_height, get_parent_info(gui, widget)->vertical_amplitude * gui.resolution.y);
			    
			    break;

			default:
			    coords.ydim.d = GuiDim::Pixel(20.f);
			    max_height = SV_MAX(max_height, 20.f);
			    break;
		
			}	    
		    }

		    // Padding
		    data.yoff += max_height + 5.f;
		    data.last_same_line = 1u;
		}
		else
		    --data.same_line_count;
	    }
	}
	break;

	}
    }

    SV_INLINE static void read_widget(GUI& gui, GuiWidgetType type, u64 id, u32& current_parent, u8*& it)
    {
	// Add widget
	GuiWidget& w = gui.widgets.emplace_back();
	w.type = type;
	w.id = id;
	w.index = u32(gui.widgets.size() - 1u);
	gui.indices.emplace_back(w.index, id);

	// Set focus
	if (gui.focus.type == w.type && gui.focus.id == id) {
	    gui.current_focus = { w.index, id };
	}

	// Get parent
	GuiWidget* parent = (current_parent == u32_max) ? nullptr : &gui.widgets[current_parent];

	if (is_parent(w)) {
	    
	    GuiParentInfo* parent_info = get_parent_info(gui, w);
	    parent_info->parent_index = current_parent;
	    current_parent = w.index;
	    parent_info->widget_offset = u32(gui.indices.size());
	    parent_info->menu_item_count = 0u;
	    parent_info->min_y = f32_max;
	    parent_info->max_y = f32_min;

	    parent_info->layout = _read<GuiLayout>(it);
	    parent_info->vertical_amplitude = _read<f32>(it);
	    parent_info->horizontal_amplitude = _read<f32>(it);
	    parent_info->vertical_offset = _read<f32>(it);
	    parent_info->has_vertical_scroll = _read<bool>(it);

	    if (parent_info->layout == GuiLayout_Flow) {

		parent_info->flow.yoff = 0.f;
		parent_info->flow.same_line_count = 1u;
		parent_info->flow.last_same_line = 1u;
	    }
	}
	
	// Compute bounds
	compute_layout(gui, w, parent ? get_parent_info(gui, *parent) : nullptr, it);

	// Read raw data
	switch (type)
	{
	case GuiWidgetType_MenuContainer:
	    break;

	case GuiWidgetType_Container:
	{
	    Raw_Container raw = _read<Raw_Container>(it);
	    w.widget.container.style = gui.temp_style.container;
	}
	break;

	case GuiWidgetType_Popup:
	{
	    Raw_Popup raw = _read<Raw_Popup>(it);
	    w.widget.popup.style = gui.temp_style.popup;
	    w.widget.popup.close_request = false;
	    w.bounds = raw.bounds;
	}
	break;

	case GuiWidgetType_Window:
	{
	    Raw_Window raw = _read<Raw_Window>(it);
	    w.widget.window.style = gui.temp_style.window;

	    GuiWindowState& state = gui.static_state.window[id];
	    w.bounds = state.bounds;
	    w.widget.window.state = &state;
	}
	break;

	case GuiWidgetType_Button:
	{
	    auto& button = w.widget.button;

	    button.text = (const char*)it;
	    it += strlen(button.text) + 1u;
	    if (*button.text == '\0')
		button.text = nullptr;

	    button.style = gui.temp_style.button;
	}
	break;

	case GuiWidgetType_Label:
	{
	    auto& label = w.widget.label;

	    label.text = (const char*)it;
	    it += strlen(label.text) + 1u;
	    if (*label.text == '\0')
		label.text = nullptr;

	    label.style = gui.temp_style.label;
	}
	break;

	case GuiWidgetType_Slider:
	    break;

	case GuiWidgetType_Checkbox:
	{
	    auto& cb = w.widget.checkbox;
			
	    Raw_Checkbox raw = _read<Raw_Checkbox>(it);
	    cb.value = raw.value;
	    cb.style = gui.temp_style.checkbox;
	}
	break;

	case GuiWidgetType_Drag:
	{
	    auto& drag = w.widget.drag;

	    Raw_Drag raw = _read<Raw_Drag>(it);
	    drag.primitive = raw.primitive;
	    drag.value = raw.value;
	    drag.min = raw.min;
	    drag.max = raw.max;
	    drag.adv = raw.adv;
	    drag.style = gui.temp_style.drag;
	}
	break;

	case GuiWidgetType_MenuItem:
	{
	    auto& menu = w.widget.menu_item;

	    menu.text = (const char*)it;
	    it += strlen(menu.text) + 1u;
	    if (*menu.text == '\0')
		menu.text = nullptr;

	    menu.style = gui.temp_style.menuitem;
	    menu.active = _read<bool>(it);

	    if (parent) {

		GuiParentInfo* parent_info = get_parent_info(gui, *parent);
		++parent_info->menu_item_count;
	    }
	    else {
		++gui.root_menu_count;
	    }
	}
	break;

	case GuiWidgetType_Package:
	{
	    auto& p = w.widget.package;
			
	    p.size = _read<u32>(it);
	    if (p.size) {
		p.data = it;
		it += p.size;
	    }
	    else p.data = nullptr;

	    p.package_id = _read<u64>(it);
	}
	break;

	case GuiWidgetType_Reciver:
	{
	    auto& reciver = w.widget.reciver;

	    Raw_Reciver raw = _read<Raw_Reciver>(it);
	    reciver.package_id = raw.package_id;
	}
	break;

	default:
	    SV_ASSERT(0);
	}
    }

    // TEMP
    constexpr f32 MENU_HEIGHT = 0.02f;
    constexpr f32 MENU_WIDTH = 0.04f;

    constexpr f32 MENU_CONTAINER_HEIGHT = 0.1f;
    constexpr f32 MENU_CONTAINER_WIDTH = 0.1f;

    constexpr f32 SCROLL_SIZE = 0.005f;
    constexpr f32 SCROLL_BUTTON_SIZE_MULT = 0.2f;
	
    static void update_bounds(GUI& gui, GuiWidget* w, GuiWidget* parent, u32& menu_index)
    {
	// Update bounds
	switch (w->type)
	{
	case GuiWidgetType_Container:
	case GuiWidgetType_Button:
	case GuiWidgetType_Slider:
	case GuiWidgetType_Label:
	case GuiWidgetType_Checkbox:
	case GuiWidgetType_Drag:
	    w->bounds = compute_widget_bounds(gui, w, parent);
	    break;
		
	case GuiWidgetType_MenuItem:
	{
	    v4_f32 parent_bounds;

	    if (parent) {
		parent_bounds = parent->bounds;
	    }
	    else {
		parent_bounds = { 0.5f, 0.5f, 1.f, 1.f };
	    }

	    w->bounds.x = (parent_bounds.x - parent_bounds.z * 0.5f) + MENU_WIDTH * f32(menu_index);
	    w->bounds.y = parent_bounds.y + parent_bounds.w * 0.5f;
	    w->bounds.z = MENU_WIDTH;
	    w->bounds.w = MENU_HEIGHT;
	    w->bounds.x += w->bounds.z * 0.5f;
	    w->bounds.y -= w->bounds.w * 0.5f;

	    ++menu_index;
	}
	break;

	case GuiWidgetType_MenuContainer:
	{
	    GuiWidget* menu = &gui.widgets[w->index - 1u];
	    w->bounds.x = menu->bounds.x - menu->bounds.z * 0.5f;
	    w->bounds.y = menu->bounds.y - menu->bounds.z * 0.5f;
	    w->bounds.z = MENU_CONTAINER_WIDTH;
	    w->bounds.w = MENU_CONTAINER_HEIGHT;
	    w->bounds.x += w->bounds.z * 0.5f;
	    w->bounds.y -= w->bounds.w * 0.5f;
	}
	break;

	case GuiWidgetType_Package:
	case GuiWidgetType_Reciver:
	{
	    SV_ASSERT(w->index != 0u);
	    GuiWidget* last = &gui.widgets[w->index - 1u];

	    GuiParentInfo* parent_info = get_parent_info(gui, *last);
	    if (parent_info) w->bounds = parent_info->child_bounds;
	    else w->bounds = last->bounds;
	}
	break;

	}

	GuiParentInfo* parent_info = get_parent_info(gui, *w);
	if (parent_info) {
	    
	    // Update child bounds
	    parent_info->child_bounds = w->bounds;
	    if (parent_info->menu_item_count) {
				
		parent_info->child_bounds.w -= MENU_HEIGHT;
		parent_info->child_bounds.y -= MENU_HEIGHT * 0.5f;
	    }

	    if (parent_info->has_vertical_scroll) {
		
		parent_info->child_bounds.z -= SCROLL_SIZE;
		parent_info->child_bounds.x -= SCROLL_SIZE * 0.5f;
	    }

	    // Update childs
	    GuiIndex* it = gui.indices.data() + parent_info->widget_offset;
	    GuiIndex* end = it + parent_info->widget_count;

	    u32 _menu_index = 0u;

	    while (it != end) {

		GuiWidget* widget = &gui.widgets[it->index];
		update_bounds(gui, widget, w, _menu_index);

		if (!ignore_scroll(widget->type)) {

		    parent_info->min_y = SV_MIN(widget->bounds.y - widget->bounds.w * 0.5f, parent_info->min_y);
		    parent_info->max_y = SV_MAX(widget->bounds.y + widget->bounds.w * 0.5f, parent_info->max_y);
		}

		GuiParentInfo* p = get_parent_info(gui, *widget);
		if (p) {

		    it += p->widget_count;
		}

		++it;
	    }

	    parent_info->vertical_amplitude = parent_info->max_y - parent_info->min_y;
	    parent_info->horizontal_amplitude = 0.f; // TODO

	    // Compute child up and down offsets
	    f32 parent_min = parent_info->child_bounds.y - parent_info->child_bounds.w * 0.5f;
	    f32 parent_max = parent_info->child_bounds.y + parent_info->child_bounds.w * 0.5f;

	    // Enable - Disable scroll bars (this bool is saved in raw data and used in the next frame)
	    parent_info->has_vertical_scroll = parent_info->min_y < parent_min || parent_info->max_y > parent_max;

	    if (parent_info->has_vertical_scroll) {

		parent_info->min_y = SV_MIN(parent_info->min_y - parent_min, 0.f);
		parent_info->max_y = SV_MAX(parent_info->max_y - parent_max, 0.f);

		if (-parent_info->min_y + parent_info->max_y < 0.01f) {
		    parent_info->has_vertical_scroll = false;
		}
		else {
		    
		    // Update childs
		    it = gui.indices.data() + parent_info->widget_offset;

		    while (it != end) {

			GuiWidget* widget = &gui.widgets[it->index];

			if (!ignore_scroll(widget->type)) {

			    widget->bounds.y -= parent_info->vertical_offset;

			    GuiParentInfo* p = get_parent_info(gui, *widget);
			    if (p) {
				p->child_bounds.y -= parent_info->vertical_offset;
			    }
			}
			else {

			    GuiParentInfo* p = get_parent_info(gui, *widget);
			    if (p) {
				it += p->widget_count;
			    }
			}

			++it;
		    }
		}
	    }
	    
	    if (!parent_info->has_vertical_scroll) {

		parent_info->vertical_offset = 0.f;
		parent_info->min_y = 0.f;
		parent_info->max_y = 0.f;
	    }
	}
    }

    SV_AUX void get_style_data(GUI& gui, GuiStyle style, size_t* psize, void** pdst, bool temp = true)
    {
	*pdst = nullptr;
	*psize = 0u;

	GuiStyleData& s = temp ? gui.temp_style : gui.style;
		    
	switch (style) {
	    
	case GuiStyle_ContainerColor:
	    *pdst = &s.container.color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_WindowBackgroundColor:
	    *pdst = &s.window.color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_WindowDecorationColor:
	    *pdst = &s.window.decoration_color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_WindowOutlineColor:
	    *pdst = &s.window.outline_color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_WindowDecorationHeight:
	    *pdst = &s.window.decoration_height;
	    *psize = sizeof(f32);
	    break;
	    
	case GuiStyle_WindowOutlineSize:
	    *pdst = &s.window.outline_size;
	    *psize = sizeof(f32);
	    break;
	    
	case GuiStyle_WindowMinWidth:
	    *pdst = &s.window.min_width;
	    *psize = sizeof(f32);
	    break;
	    
	case GuiStyle_WindowMinHeight:
	    *pdst = &s.window.min_height;
	    *psize = sizeof(f32);
	    break;
	    
	case GuiStyle_PopupColor:
	    *pdst = &s.popup.color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_ButtonColor:
	    *pdst = &s.button.color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_ButtonHotColor:
	    *pdst = &s.button.hot_color;
	    *psize = sizeof(Color);
	    break;
	    
	case GuiStyle_ButtonTextColor:
	    *pdst = &s.button.text_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_SliderColor:
	    *pdst = &s.slider.color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_SliderButtonColor:
	    *pdst = &s.slider.button_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_SliderButtonSize:
	    *pdst = &s.slider.button_size;
	    *psize = sizeof(f32);
	    break;

	case GuiStyle_TextColor:
	    *pdst = &s.label.text_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_TextAlignment:
	    *pdst = &s.label.text_alignment;
	    *psize = sizeof(TextAlignment);
	    break;

	case GuiStyle_TextBackgroundColor:
	    *pdst = &s.label.background_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_CheckboxButtonColor:
	    *pdst = &s.checkbox.button_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_CheckboxBackgroundColor:
	    *pdst = &s.checkbox.background_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_CheckboxShape:
	    *pdst = &s.checkbox.shape;
	    *psize = sizeof(GuiCheckboxShape);
	    break;

	case GuiStyle_CheckboxShapeSizeMult:
	    *pdst = &s.checkbox.shape_size_mult;
	    *psize = sizeof(f32);
	    break;

	case GuiStyle_DragTextColor:
	    *pdst = &s.drag.text_color;
	    *psize = sizeof(Color);
	    break;

	case GuiStyle_DragBackgroundColor:
	    *pdst = &s.drag.background_color;
	    *psize = sizeof(Color);
	    break;

	    // TODO Menuitem
	case GuiStyle_FlowX0:
	    *pdst = &s.flow_layout.x0;
	    *psize = sizeof(f32);
	    break;

	case GuiStyle_FlowX1:
	    *pdst = &s.flow_layout.x1;
	    *psize = sizeof(f32);
	    break;

	case GuiStyle_FlowSubX0:
	    *pdst = &s.flow_layout.sub_x0;
	    *psize = sizeof(f32);
	    break;

	case GuiStyle_FlowSubX1:
	    *pdst = &s.flow_layout.sub_x1;
	    *psize = sizeof(f32);
	    break;

			
	}
    }

    void gui_end(GUI* gui_)
    {
	PARSE_GUI();

	// Reset last data
	gui.widgets.reset();
	gui.indices.reset();
	gui.root_menu_count = 0u;

	gui.current_focus.index = u32_max;

	if (gui.package_id != 0u) {

	    gui.package_id = 0u;
	    gui.package_data.reset();
	}

	SV_ASSERT(gui.ids.empty());
	SV_ASSERT(gui.parent_stack.empty());
	SV_ASSERT(gui.style_count == 0u);

	gui.temp_style = gui.style;
	gui.style_stack.reset();

	// Read widgets from raw data
	{
	    u8* it = gui.buffer.data();
	    u8* end = gui.buffer.data() + gui.buffer.size();

	    u32 current_parent = u32_max;

	    while (it != end) {

		GuiHeader header = _read<GuiHeader>(it);

		switch(header) {

		case GuiHeader_Widget:
		{
		    GuiWidgetType type = _read<GuiWidgetType>(it);
		    u64 id = _read<u64>(it);
		    read_widget(gui, type, id, current_parent, it);
		}
		break;

		case GuiHeader_EndOfParent:
		{
		    SV_ASSERT(current_parent != u32_max);

		    GuiParentInfo* parent_info = get_parent_info(gui, current_parent);
		    SV_ASSERT(parent_info);
		    parent_info->widget_count = u32(gui.indices.size()) - parent_info->widget_offset;
		    current_parent = parent_info->parent_index;
		}
		break;

		case GuiHeader_StylePush:
		{
		    GuiStyle style = _read<GuiStyle>(it);
		    u64 data = _read<u64>(it);

		    void* dst;
		    size_t size;

		    get_style_data(gui, style, &size, &dst);

		    if (dst && size) {
			gui.style_stack.write_back(dst, size);
			gui.style_stack.write_back(&style, sizeof(GuiStyle));
			memcpy(dst, &data, size);
		    }
		}
		break;

		case GuiHeader_StylePop:
		{
		    GuiStyle style;
		    gui.style_stack.read_and_pop_back(&style, sizeof(GuiStyle));

		    void* dst;
		    size_t size;

		    get_style_data(gui, style, &size, &dst);

		    if (dst && size) {
			gui.style_stack.read_and_pop_back(dst, size);
		    }
		}
		break;

		case GuiHeader_SameLine:
		{
		    SV_ASSERT(current_parent != u32_max);
		    GuiParentInfo* parent_info = get_parent_info(gui, current_parent);
		    u32 count = _read<u32>(it);
		    SV_ASSERT(parent_info->flow.same_line_count == 1u);
		    parent_info->flow.same_line_count = count;
		    parent_info->flow.last_same_line = count;
		}
		    
		}
	    }
	}

	// Compute widget bounds
	{
	    u32 menu_index = 0u;

	    foreach(i, gui.indices.size()) {

		const GuiIndex& index = gui.indices[i];
		GuiWidget* widget = &gui.widgets[index.index];

		update_bounds(gui, widget, nullptr, menu_index);

		GuiParentInfo* parent_info = get_parent_info(gui, *widget);

		if (parent_info) {
					
		    i += parent_info->widget_count;
		}
	    }
	}

	// Update focus
	if (gui.current_focus.index != u32_max) {

	    update_focus(gui, gui.current_focus);
	    input.unused = false;
	}

	// Update widgets
	foreach(i, gui.indices.size()) {

	    const GuiIndex& w = gui.indices[i];

	    update_widget(gui, w);

	    GuiParentInfo* parent_info = get_parent_info(gui, w);
	    if (parent_info) {

		i += parent_info->widget_count;
	    }
	}

	// Catch input
	if (gui.catch_input)
	    input.unused = false;
    }

    SV_INLINE static void update_id(GUI& gui)
    {
	gui.current_id = 0U;
	for (u64 id : gui.ids)
	    hash_combine(gui.current_id, id);
    }

    void gui_global_style_get(GUI* gui, GuiStyle style, void* value, size_t size)
    {
	void* dst;
	size_t write_size;
	get_style_data(*gui, style, &write_size, &dst, false);

	if (dst && write_size) {

	    SV_ASSERT(write_size == size);
	    memcpy(value, dst, write_size);
	}
    }
    
    void gui_global_style_set(GUI* gui, GuiStyle style, const void* value, size_t size)
    {
	void* dst;
	size_t write_size;
	get_style_data(*gui, style, &write_size, &dst, false);

	if (dst && write_size) {

	    SV_ASSERT(write_size == size);
	    memcpy(dst, value, write_size);
	}
    }

    void gui_push_style(GUI* gui, GuiStyle style, const void* value, size_t size)
    {
	SV_ASSERT(size <= sizeof(u64));
	
	++gui->style_count;
	write_buffer(*gui, GuiHeader_StylePush);

	write_buffer(*gui, style);
	
	u64 data;
	memcpy(&data, value, size);
	write_buffer(*gui, value, sizeof(u64));
    }
    
    void gui_pop_style(GUI* gui, u32 count)
    {
	SV_ASSERT(gui->style_count >= count);
	
	foreach(i, count) {

	    write_buffer(*gui, GuiHeader_StylePop);
	}

	gui->style_count -= count;
    }

    void gui_same_line(GUI* gui, u32 count)
    {
	SV_ASSERT(count != 0u);
	SV_ASSERT(gui->parent_stack.size() && gui->parent_stack.back().layout == GuiLayout_Flow);
	write_buffer(*gui, GuiHeader_SameLine);
	write_buffer(*gui, count);
    }

    void gui_push_id(GUI* gui_, u64 id)
    {
	PARSE_GUI();
	gui.ids.push_back(id);
	update_id(gui);
    }

    void gui_push_id(GUI* gui_, const char* id)
    {
	PARSE_GUI();
	gui.ids.push_back((u64)id);
	update_id(gui);
    }

    void gui_pop_id(GUI* gui_, u32 count)
    {
	PARSE_GUI();
	SV_ASSERT(gui.ids.size() >= count);
	gui.ids.pop_back(count);
	update_id(gui);
    }

    void gui_xbounds(GUI* gui_, GuiCoord x0, GuiCoord x1)
    {
	PARSE_GUI();
	gui.free_bounds.xtype = GuiCoordType_Coord;
	gui.free_bounds.xcoord._0 = x0;
	gui.free_bounds.xcoord._1 = x1;
    }
    
    void gui_xbounds(GUI* gui_, GuiCoord x, GuiAlign align, GuiDim w)
    {
	PARSE_GUI();
	gui.free_bounds.xtype = GuiCoordType_Dim;
	gui.free_bounds.xdim.c = x;
	gui.free_bounds.xdim.align = align;
	gui.free_bounds.xdim.d = w;
    }
    
    void gui_ybounds(GUI* gui_, GuiCoord y0, GuiCoord y1)
    {
	PARSE_GUI();
	gui.free_bounds.ytype = GuiCoordType_Coord;
	gui.free_bounds.ycoord._0 = y0;
	gui.free_bounds.ycoord._1 = y1;
    }
    
    void gui_ybounds(GUI* gui_, GuiCoord y, GuiAlign align, GuiDim h)
    {
	PARSE_GUI();
	gui.free_bounds.ytype = GuiCoordType_Dim;
	gui.free_bounds.ydim.c = y;
	gui.free_bounds.ydim.align = align;
	gui.free_bounds.ydim.d = h;
    }
    
    void gui_bounds(GUI* gui_, GuiCoord x, GuiAlign xalign, GuiDim w, GuiCoord y, GuiAlign yalign, GuiDim h)
    {
	PARSE_GUI();
	gui.free_bounds.xtype = GuiCoordType_Dim;
	gui.free_bounds.xdim.c = x;
	gui.free_bounds.xdim.align = xalign;
	gui.free_bounds.xdim.d = w;
	gui.free_bounds.ytype = GuiCoordType_Dim;
	gui.free_bounds.ydim.c = y;
	gui.free_bounds.ydim.align = yalign;
	gui.free_bounds.ydim.d = h;
    }
    
    void gui_bounds(GUI* gui_, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1)
    {
	PARSE_GUI();
	gui.free_bounds.xtype = GuiCoordType_Coord;
	gui.free_bounds.xcoord._0 = x0;
	gui.free_bounds.xcoord._1 = x1;
	gui.free_bounds.ytype = GuiCoordType_Coord;
	gui.free_bounds.ycoord._0 = y0;
	gui.free_bounds.ycoord._1 = y1;
    }

    ///////////////////////////////////////////// WIDGETS ///////////////////////////////////////////

    SV_INLINE static GuiWidget* find_widget(GUI& gui, u64 id, GuiWidgetType type)
    {
	GuiIndex res;
	res.index = u32_max;

	if (gui.parent_stack.empty() || gui.parent_stack.back().parent == nullptr) {

	    foreach(i, gui.indices.size()) {

		const GuiIndex& w = gui.indices[i];

		if (w.id == id) {

		    res = w;
		    break;
		}
		else {

		    GuiParentInfo* parent_info = get_parent_info(gui, w);
		    if (parent_info) i += parent_info->widget_count;
		}
	    }
	}
	else {

	    WritingParentInfo& info = gui.parent_stack.back();

	    GuiParentInfo* parent_info = get_parent_info(gui, *info.parent);
	    GuiIndex* it = gui.indices.data() + parent_info->widget_offset;
	    GuiIndex* end = it + parent_info->widget_count;

	    while (it != end) {

		if (it->id == id) {

		    res = *it;
		    break;
		}
		else {

		    parent_info = get_parent_info(gui, *it);
		    if (parent_info) it += parent_info->widget_count;
		}

		++it;
	    }
	}

	if (res.index == u32_max) return nullptr;

	GuiWidget* widget = &gui.widgets[res.index];

	if (widget->type != type) {
	    SV_ASSERT(!"Widget ID repeated with different widget type");
	    return nullptr;
	}

	return widget;
    }

    SV_INLINE void begin_parent(GUI& gui, GuiWidget* parent, u64 id, GuiLayout layout)
    {
	WritingParentInfo& info = gui.parent_stack.emplace_back();
	info.layout = layout;
	
	if (parent) {
	    SV_ASSERT(is_parent(parent->type));
	    info.parent = parent;
	}
	gui_push_id((GUI*)& gui, id);
    }

    SV_INLINE void end_parent(GUI& gui)
    {
	write_buffer(gui, GuiHeader_EndOfParent);
	gui_pop_id((GUI*)& gui);
	gui.parent_stack.pop_back();
    }

    void gui_begin_container(GUI* gui_, u64 id, GuiLayout layout)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);

	GuiWidget* w = find_widget(gui, id, GuiWidgetType_Container);

	Raw_Container raw;

	GuiParentInfo* parent_info;
	if (w) parent_info = get_parent_info(gui, *w);
	else parent_info = nullptr;

	write_widget(gui, GuiWidgetType_Container, id, &raw, parent_info, layout);
	begin_parent(gui, w, id, layout);
    }

    void gui_end_container(GUI* gui_)
    {
	PARSE_GUI();
	end_parent(gui);
    }

    bool gui_begin_popup(GUI* gui_, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, GuiLayout layout)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);

	GuiWidget* w = find_widget(gui, id, GuiWidgetType_Popup);

	if (w) {

	    auto& popup = w->widget.popup;

	    if (popup.close_request)
		return false;

	    Raw_Popup raw;
	    raw.bounds = w->bounds;

	    GuiParentInfo* parent_info = get_parent_info(gui, *w);

	    write_widget(gui, GuiWidgetType_Popup, w->id, &raw, parent_info, layout);
	    begin_parent(gui, w, id, layout);
	    return true;
	}
	else {

	    bool open = false;

	    if (input.mouse_buttons[mouse_button] == InputState_Released) {

		switch (trigger)
		{

		case GuiPopupTrigger_Parent:
		{
		    if (gui.parent_stack.size() && gui.parent_stack.back().parent != nullptr) {

			GuiWidget& parent = *gui.parent_stack.back().parent;

			if (mouse_in_bounds(gui, parent.bounds)) {

			    open = true;
							
			    GuiParentInfo* parent_info = get_parent_info(gui, parent);
			    SV_ASSERT(parent_info);

			    foreach(i, parent_info->widget_count) {

				const GuiIndex& w = gui.indices[parent_info->widget_offset + i];

				const v4_f32& bounds = gui.widgets[w.index].bounds;

				if (mouse_in_bounds(gui, bounds)) {
				    open = false;
				    break;
				}
			    }
			}
		    }
		}
		break;

		case GuiPopupTrigger_LastWidget:
		{
		    if (gui.last.type != GuiWidgetType_None) {

			GuiWidget* w = find_widget(gui, gui.last.id, gui.last.type);

			if (w && mouse_in_bounds(gui, w->bounds)) {
			    open = true;
			}
		    }
		}
		break;

		}
	    }

	    if (open) {

		Raw_Popup raw;
		raw.bounds.z = 0.1f;
		raw.bounds.w = 0.2f;
		raw.bounds.x = gui.mouse_position.x + raw.bounds.z * 0.5f;
		raw.bounds.y = gui.mouse_position.y - raw.bounds.w * 0.5f;

		write_widget(gui, GuiWidgetType_Popup, id, &raw, nullptr, layout);
		begin_parent(gui, w, id, layout);
		return true;
	    }
	}

	return false;
    }

    void gui_end_popup(GUI* gui_)
    {
	PARSE_GUI();
	end_parent(gui);
    }

    /////////////////////////////////////// WINDOW ////////////////////////////////////////////////

    bool gui_begin_window(GUI* gui_, const char* title, GuiLayout layout)
    {
	PARSE_GUI();
	u64 id = hash_string(title);

	GuiWindowState* state = nullptr;

	auto it = gui.static_state.window.find(id);
	if (it == gui.static_state.window.end()) {

	    GuiWindowState s;
	    s.bounds = { 0.5f, 0.5f, 0.1f, 0.3f };
	    s.show = true;
	    s.title = title;
	    s.id = id;

	    gui.static_state.window[id] = std::move(s);
	    state = &gui.static_state.window[id];
	}
	else state = &it->second;

	if (state->show) {
	    Raw_Window raw;

	    GuiWidget* w = find_widget(gui, id, GuiWidgetType_Window);

	    GuiParentInfo* parent_info;
	    if (w) parent_info = get_parent_info(gui, *w);
	    else parent_info = nullptr;

	    write_widget(gui, GuiWidgetType_Window, id, &raw, parent_info, layout);
	    begin_parent(gui, w, id, layout);
	}

	return state->show;
    }

    void gui_end_window(GUI* gui_)
    {
	PARSE_GUI();
	end_parent(gui);
    }

    SV_INLINE static GuiWindowState* get_window_state(GUI& gui, u64 id)
    {
	auto it = gui.static_state.window.find(id);
	if (it == gui.static_state.window.end()) return nullptr;
	return &it->second;
    }

    bool gui_show_window(GUI* gui_, const char* title)
    {
	PARSE_GUI();
	u64 id = hash_string(title);
	GuiWindowState* state = get_window_state(gui, id);

	if (state) {

	    state->show = true;
	    return true;
	}

	return false;
    }

    bool gui_hide_window(GUI* gui_, const char* title)
    {
	PARSE_GUI();
	u64 id = hash_string(title);
	GuiWindowState* state = get_window_state(gui, id);

	if (state) {

	    state->show = false;
	    return true;
	}

	return false;
    }

    /////////////////////////////////////// MENU ITEM /////////////////////////////////////////

    bool gui_begin_menu_item(GUI* gui_, const char* text, u64 id, GuiLayout layout)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);

	GuiWidget* w = find_widget(gui, id, GuiWidgetType_MenuItem);

	{
	    Raw_MenuItem raw;
	    raw.text = text;
	    raw.active = w ? w->widget.menu_item.active : false;

	    GuiParentInfo* parent_info;
	    if (w) parent_info = get_parent_info(gui, *w);
	    else parent_info = nullptr;

	    write_widget(gui, GuiWidgetType_MenuItem, id, &raw, parent_info);
	}

	if (w && w->widget.menu_item.active) {
			
	    u64 container_id = id;
	    hash_combine(container_id, 0x32d9f32ac);

	    write_widget(gui, GuiWidgetType_MenuContainer, container_id, nullptr, get_parent_info(gui, *w), layout);

	    GuiWidget* c = find_widget(gui, container_id, GuiWidgetType_MenuContainer);
	    begin_parent(gui, c, container_id, layout);

	    return true;
	}

	return false;
    }

    void gui_end_menu_item(GUI* gui_)
    {
	PARSE_GUI();
	end_parent(gui);
    }

    /////////////////////////////////////// PACKAGE ///////////////////////////////////////

    void gui_send_package(GUI* gui_, const void* package, u32 package_size, u64 package_id)
    {
	PARSE_GUI();

	SV_ASSERT(gui.last.type != GuiWidgetType_None);
		
	u64 id = gui.last.id;
	hash_combine(id, 0x89562f8a732db);
	hash_combine(package_id, 0xa89d4fb319);

	Raw_Package raw;
		
	if (gui.current_focus.index != u32_max && gui.current_focus.id == id) {
			
	    raw.package = package;
	    raw.package_size = package_size;
	    raw.package_id = package_id;
	}
	else raw.package_size = 0u;
		
	write_widget(gui, GuiWidgetType_Package, id, &raw, nullptr);
    }
	
    bool gui_recive_package(GUI* gui_, void** package, u32* package_size, u64 package_id)
    {
	PARSE_GUI();

	SV_ASSERT(gui.last.type != GuiWidgetType_None);
		
	u64 id = gui.last.id;
	hash_combine(id, 0x89562f8a732db);
	hash_combine(package_id, 0xa89d4fb319);

	if (gui.current_focus.index != u32_max && gui.focus.type == GuiWidgetType_Package) {

	    GuiWidget* package_widget = &gui.widgets[gui.current_focus.index];

	    if (package_widget->widget.package.package_id == package_id) {

		Raw_Reciver raw;
		raw.package_id = package_id;

		write_widget(gui, GuiWidgetType_Reciver, id, &raw, nullptr);
	    }
	}
		
	if (package_id == gui.package_id) {

	    GuiWidget* w = find_widget(gui, id, GuiWidgetType_Reciver);

	    if (w && w->widget.reciver.recived) {

		gui.package_id = 0u;
		*package = gui.package_data.data();
		if (package_size)* package_size = u32(gui.package_data.size());
		return true;
	    }
	}
		
	return false;
    }

    /////////////////////////////////////// COMMON WIDGETS ///////////////////////////////////////

    bool gui_button(GUI* gui_, const char* text, u64 id)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);

	{
	    Raw_Button raw;
	    raw.text = text;

	    write_widget(gui, GuiWidgetType_Button, id, &raw, nullptr);
	}

	GuiWidget* w = find_widget(gui, id, GuiWidgetType_Button);

	if (w) {
	    return w->widget.button.pressed;
	}
	else {
	    return false;
	}
    }

    SV_AUX bool gui_drag(GUI& gui, u64* value, u64 adv, u64 min, u64 max, u64 id, GuiPrimitive primitive)
    {
	hash_combine(id, gui.current_id);

	bool modified;

	if (gui.current_focus.index != u32_max && gui.current_focus.id == id) {

	    GuiWidget& w = gui.widgets[gui.current_focus.index];
	    SV_ASSERT(w.type == GuiWidgetType_Drag);

	    *value = w.widget.drag.value;
	    modified = true;
	}
	else modified = false;

	{
	    Raw_Drag raw;
	    raw.value = *value;
	    raw.min = min;
	    raw.max = max;
	    raw.adv = adv;
	    raw.primitive = primitive;

	    write_widget(gui, GuiWidgetType_Drag, id, &raw, nullptr);
	}

	return modified;
    }

    bool gui_drag_f32(GUI* gui, f32* value, f32 adv, f32 min, f32 max, u64 id)
    {
	u64 _value;
	u64 _adv;
	u64 _min;
	u64 _max;

	memcpy(&_value, value, sizeof(f32));
	memcpy(&_adv, &adv, sizeof(f32));
	memcpy(&_min, &min, sizeof(f32));
	memcpy(&_max, &max, sizeof(f32));

	bool res = gui_drag(*gui, &_value, _adv, _min, _max, id, GuiPrimitive_f32);

	if (res) {
	    memcpy(value, &_value, sizeof(f32));
	}

	return res;
    }

    bool gui_drag_u8(GUI* gui, u8* value, u8 adv, u8 min, u8 max, u64 id)
    {
	u64 _value;
	u64 _adv;
	u64 _min;
	u64 _max;

	memcpy(&_value, value, sizeof(u8));
	memcpy(&_adv, &adv, sizeof(u8));
	memcpy(&_min, &min, sizeof(u8));
	memcpy(&_max, &max, sizeof(u8));

	bool res = gui_drag(*gui, &_value, _adv, _min, _max, id, GuiPrimitive_u8);

	if (res) {
	    memcpy(value, &_value, sizeof(u8));
	}

	return res;
    }
    
    bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id)
    {
	return false;
    }

    void gui_text(GUI* gui_, const char* text, u64 id)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);
		
	Raw_Label raw;
	raw.text = text;

	write_widget(gui, GuiWidgetType_Label, id, &raw, nullptr);		
    }

    bool gui_checkbox(GUI* gui_, bool* value, u64 id)
    {
	PARSE_GUI();
	hash_combine(id, gui.current_id);

	bool modified;

	if (gui.current_focus.index != u32_max && gui.current_focus.id == id) {

	    GuiWidget& w = gui.widgets[gui.current_focus.index];
	    SV_ASSERT(w.type == GuiWidgetType_Checkbox);
	    *value = w.widget.checkbox.value;
	    modified = true;
	}
	else modified = false;

	{
	    Raw_Checkbox raw;
	    raw.value = *value;

	    write_widget(gui, GuiWidgetType_Checkbox, id, &raw, nullptr);
	}

	return modified;
    }

    bool gui_checkbox(GUI* gui_, u64 user_id)
    {
	PARSE_GUI();
	u64 id = user_id;
	hash_combine(id, gui.current_id);

	bool* value = nullptr;

	auto it = gui.static_state.checkbox.find(id);
	if (it == gui.static_state.checkbox.end()) {

	    gui.static_state.checkbox[id] = false;
	    value = &gui.static_state.checkbox[id];
	}
	else value = &it->second;
		
	gui_checkbox(gui_, value, user_id);
	return *value;
    }

    ///////////////////////////////////////////// GETTERS ///////////////////////////////////////////

    v4_f32 gui_parent_bounds(GUI* gui_)
    {
	PARSE_GUI();
	return (gui.parent_stack.size() && gui.parent_stack.back().parent) ? gui.parent_stack.back().parent->bounds : v4_f32{};
    }

    ///////////////////////////////////////////// RENDERING ///////////////////////////////////////////

    SV_INTERNAL void draw_widget(GUI& gui, const GuiWidget& w, CommandList cmd)
    {	
	switch (w.type)
	{

	case GuiWidgetType_Button:
	{
	    auto& button = w.widget.button;

	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, button.hot ? button.style.hot_color : button.style.color, cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	    if (button.text) {

		f32 font_size = size.y;

		draw_text(button.text, strlen(button.text)
			  , pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font_opensans.vertical_offset * font_size,
			  size.x, 1u, font_size, gui.aspect, TextAlignment_Center, &font_opensans, button.style.text_color, cmd);
	    }
	}
	break;

	case GuiWidgetType_Drag:
	{
	    begin_debug_batch(cmd);

	    v2_f32 pos;
	    v2_f32 size;

	    auto& drag = w.widget.drag;
	    pos = v2_f32{ w.bounds.x, w.bounds.y } *2.f - 1.f;
	    size = v2_f32{ w.bounds.z, w.bounds.w } *2.f;

	    draw_debug_quad(pos.getVec3(0.f), size, drag.style.background_color, cmd);

	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	    f32 font_size = size.y;

	    char strbuff[100u] = "\0";
	    
	    switch (drag.primitive) {

	    case GuiPrimitive_f32:
	    {
		f32 value = *reinterpret_cast<const f32*>(&drag.value);
		sprintf(strbuff, "%f", value);
	    }
	    break;

	    case GuiPrimitive_u8:
	    {
		u8 value = *reinterpret_cast<const u8*>(&drag.value);
		sprintf(strbuff, "%u", (u32)value);
	    }
	    break;
		
	    }

	    draw_text(strbuff, strlen(strbuff)
		      , pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font_opensans.vertical_offset * font_size,
		      size.x, 1u, font_size, gui.aspect, TextAlignment_Center, &font_opensans, drag.style.text_color, cmd);
	}
	break;

	case GuiWidgetType_MenuItem:
	{
	    auto& menu = w.widget.menu_item;

	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, menu.hot ? menu.style.hot_color : menu.style.color, cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	    if (menu.text) {

		f32 font_size = size.y;

		draw_text(menu.text, strlen(menu.text)
			  , pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font_opensans.vertical_offset * font_size,
			  size.x, 1u, font_size, gui.aspect, TextAlignment_Center, &font_opensans, menu.style.text_color, cmd);
	    }
	}
	break;

	case GuiWidgetType_Label:
	{
	    begin_debug_batch(cmd);

	    v2_f32 pos;
	    v2_f32 size;

	    auto& label = w.widget.label;
	    pos = v2_f32{ w.bounds.x, w.bounds.y } *2.f - 1.f;
	    size = v2_f32{ w.bounds.z, w.bounds.w } *2.f;

	    draw_debug_quad(pos.getVec3(0.f), size, label.style.background_color, cmd);

	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	    if (label.text) {

		f32 font_size = size.y;

		draw_text(label.text, strlen(label.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f -
			  font_opensans.vertical_offset * font_size, size.x, 1u, font_size, gui.aspect, label.style.text_alignment,
			  &font_opensans, label.style.text_color, cmd);
	    }
	}
	break;

	case GuiWidgetType_Checkbox:
	{
	    begin_debug_batch(cmd);

	    v2_f32 pos;
	    v2_f32 size;

	    auto& cb = w.widget.checkbox;
	    pos = v2_f32{ w.bounds.x, w.bounds.y } *2.f - 1.f;
	    size = v2_f32{ w.bounds.z, w.bounds.w } *2.f;

	    draw_debug_quad(pos.getVec3(0.f), size, cb.style.background_color, cmd);

	    size *= cb.style.shape_size_mult;

	    switch (cb.style.shape)
	    {
	    case GuiCheckboxShape_Quad:
		if (cb.value)
		    draw_debug_quad(pos.getVec3(0.f), size, cb.style.button_color, cmd);
		break;

	    case GuiCheckboxShape_Triangle:
		if (cb.value) {
		    draw_debug_triangle(
			    { pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
			    { pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
			    { pos.x, pos.y - size.y * 0.5f, 0.f }
			    , cb.style.button_color, cmd);
		}
		else {
		    draw_debug_triangle(
			    { pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
			    { pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, 0.f },
			    { pos.x + size.x * 0.5f, pos.y, 0.f }
			    , cb.style.button_color, cmd);
		}
		break;
	    }

	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);
	}
	break;

	case GuiWidgetType_Container:
	{
	    auto& container = w.widget.container;

	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, container.style.color, cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);
	}
	break;

	case GuiWidgetType_Reciver:
	{
	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, Color::White(180u), cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);
	}
	break;

	case GuiWidgetType_MenuContainer:
	{
	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, Color::White(), cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);
	}
	break;

	case GuiWidgetType_Popup:
	{
	    auto& popup = w.widget.popup;

	    v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y) * 2.f - 1.f;
	    v2_f32 size = v2_f32(w.bounds.z, w.bounds.w) * 2.f;

	    begin_debug_batch(cmd);
	    draw_debug_quad(pos.getVec3(), size, popup.style.color, cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);
	}
	break;

	case GuiWidgetType_Window:
	{
	    auto& window = w.widget.window;
	    const GuiWindowState& state = *window.state;
	    const GuiWindowStyle& style = window.style;

	    v4_f32 decoration = compute_window_decoration_bounds(gui, w);
	    v4_f32 closebutton = compute_window_closebutton_bounds(gui, w, decoration);

	    v2_f32 outline_size;
	    v2_f32 content_position;
	    v2_f32 content_size;

	    content_position = v2_f32{ w.bounds.x, w.bounds.y };
	    content_size = v2_f32{ w.bounds.z, w.bounds.w };

	    outline_size = content_size + v2_f32{ style.outline_size, style.outline_size * gui.aspect } *2.f;

	    // Normal to clip
	    content_position = content_position * 2.f - 1.f;
	    content_size = content_size * 2.f;
	    outline_size = outline_size * 2.f;
	    v2_f32 decoration_position = v2_f32(decoration.x, decoration.y) * 2.f - 1.f;
	    v2_f32 decoration_size = v2_f32(decoration.z, decoration.w) * 2.f;
	    v2_f32 closebutton_position = v2_f32(closebutton.x, closebutton.y) * 2.f - 1.f;
	    v2_f32 closebutton_size = v2_f32(closebutton.z, closebutton.w) * 2.f;

	    begin_debug_batch(cmd);

	    draw_debug_quad(content_position.getVec3(0.f), outline_size, style.outline_color, cmd);
	    draw_debug_quad(content_position.getVec3(0.f), content_size, style.color, cmd);
	    draw_debug_quad(decoration_position.getVec3(0.f), decoration_size, style.decoration_color, cmd);
	    draw_debug_quad(closebutton_position.getVec3(0.f), closebutton_size, Color::Red(), cmd);

	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	    if (state.title.size()) {

		f32 font_size = decoration_size.y * 0.5f;

		draw_text(state.title.c_str(), state.title.size()
			  , decoration_position.x - decoration_size.x * 0.5f + 0.01f, decoration_position.y +
			  decoration_size.y * 0.25f - font_opensans.vertical_offset * font_size, decoration_size.x, 1u, font_size,
			  gui.aspect, TextAlignment_Left, &font_opensans, cmd);
	    }
	}
	break;

	}

	// If it is a parent, draw childs and some parent stuff
	{
	    const GuiParentInfo* parent_info = get_parent_info(gui, w);
	    if (parent_info) {

		// Draw vertical scroll
		if (parent_info->has_vertical_scroll) {

		    f32 norm = 1.f - ((parent_info->vertical_offset - parent_info->min_y) / (parent_info->max_y - parent_info->min_y));

		    v4_f32 scroll_bounds = parent_info->child_bounds;
		    //scroll_bounds.y += parent_info->vertical_offset;

		    scroll_bounds.x += scroll_bounds.z * 0.5f + SCROLL_SIZE * 0.5f;
		    scroll_bounds.z = SCROLL_SIZE;
		    
		    f32 button_height = scroll_bounds.w * SCROLL_BUTTON_SIZE_MULT;
		    f32 button_space = scroll_bounds.w - button_height;
		    f32 button_y = (scroll_bounds.y + scroll_bounds.w * 0.5f) - (button_height * 0.5f) - (norm * button_space);
					
		    begin_debug_batch(cmd);
					
		    draw_debug_quad((v2_f32(scroll_bounds.x, scroll_bounds.y) * 2.f - 1.f).getVec3(0.f), v2_f32(scroll_bounds.z, scroll_bounds.w) * 2.f, Color::Red(), cmd);
		    draw_debug_quad((v2_f32(scroll_bounds.x, button_y) * 2.f - 1.f).getVec3(0.f), v2_f32(scroll_bounds.z, button_height) * 2.f, Color::Green(), cmd);
					
					
		    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

		}
				
		// Draw childs

		const GPUImageInfo& info = graphics_image_info(gfx.offscreen);

		Scissor last_scissor = graphics_scissor_get(0u, cmd);

		v4_f32 s_bounds;
		s_bounds.z = parent_info->child_bounds.z * f32(info.width);
		s_bounds.w = parent_info->child_bounds.w * f32(info.height);
		s_bounds.x = parent_info->child_bounds.x * f32(info.width) - (s_bounds.z * 0.5f);
		s_bounds.y = parent_info->child_bounds.y * f32(info.height) - (s_bounds.w * 0.5f);
		
		v4_f32 last_s_bounds;
		last_s_bounds.x = f32(last_scissor.x);
		last_s_bounds.y = f32(last_scissor.y);
		last_s_bounds.z = f32(last_scissor.width);
		last_s_bounds.w = f32(last_scissor.height);

		if (s_bounds.x < last_s_bounds.x) {

		    s_bounds.z -= last_s_bounds.x - s_bounds.x;
		    s_bounds.x = last_s_bounds.x;
		}
		if (s_bounds.x + s_bounds.z > last_s_bounds.x + last_s_bounds.z) {

		    s_bounds.z -= (s_bounds.x + s_bounds.z) - (last_s_bounds.x + last_s_bounds.z);
		}

		if (s_bounds.y < last_s_bounds.y) {

		    s_bounds.w -= last_s_bounds.y - s_bounds.y;
		    s_bounds.y = last_s_bounds.y;
		}
		if (s_bounds.y + s_bounds.w > last_s_bounds.y + last_s_bounds.w) {

		    s_bounds.w -= (s_bounds.y + s_bounds.w) - (last_s_bounds.y + last_s_bounds.w);
		}

		s_bounds.z = SV_MAX(s_bounds.z, 0.f);
		s_bounds.w = SV_MAX(s_bounds.w, 0.f);
		
		Scissor scissor;
		scissor.x = u32(s_bounds.x);
		scissor.y = u32(s_bounds.y);
		scissor.width = u32(s_bounds.z);
		scissor.height = u32(s_bounds.w);
	    
		graphics_scissor_set(scissor, 0u, cmd);
				
		GuiIndex* it = gui.indices.data() + parent_info->widget_offset;
		GuiIndex* end = it + parent_info->widget_count;

		// Draw normal widgets
		while (it != end) {

		    GuiParentInfo* p = get_parent_info(gui, *it);
		    if (p) {

			it += p->widget_count;
		    }
		    else draw_widget(gui, gui.widgets[it->index], cmd);

		    ++it;
		}

		// Draw parents
		it = gui.indices.data() + parent_info->widget_offset;

		while (it != end) {

		    GuiParentInfo* p = get_parent_info(gui, *it);
		    if (p) {

			draw_widget(gui, gui.widgets[it->index], cmd);
			it += p->widget_count;
		    }

		    ++it;
		}

		graphics_scissor_set(last_scissor, 0u, cmd);
	    }
	}
    }

    void gui_draw(GUI* gui_, CommandList cmd)
    {
	PARSE_GUI();

	foreach(i, gui.indices.size()) {

	    const GuiIndex& w = gui.indices[i];

	    draw_widget(gui, gui.widgets[w.index], cmd);

	    GuiParentInfo* parent_index = get_parent_info(gui, w);
	    if (parent_index) {

		i += parent_index->widget_count;
	    }
	}

	if (gui.focus.type == GuiWidgetType_Package) {

	    begin_debug_batch(cmd);
	    draw_debug_quad((gui.mouse_position * 2.f - 1.f).getVec3(0.f), v2_f32{ 0.01f, 0.01f * gui.aspect } * 2.f, Color::Green(), cmd);
	    end_debug_batch(true, false, XMMatrixIdentity(), cmd);

	}
    }

    /////////////////////////////////// HIGH LEVEL //////////////////////////////////////////

    void gui_begin_grid(GUI* gui_, u32 element_count, f32 element_size, f32 padding)
    {
	PARSE_GUI();
	auto& grid = gui.grid;

	v4_f32 parent_bounds = gui_parent_bounds(gui_);

	f32 width = parent_bounds.z * gui.resolution.x;

	grid.element_count = element_count;
	grid.element_index = 0u;
	grid.columns = SV_MAX(u32((width + padding) / (element_size + padding)), 1u);
	grid.element_size = element_size;
	grid.xoff = SV_MAX(width - grid.element_size * f32(grid.columns) - padding * f32(grid.columns - 1u), 0.f) * 0.5f;
	grid.padding = padding;
    }

    void gui_begin_grid_element(GUI* gui_, u64 id)
    {
	PARSE_GUI();
	auto& grid = gui.grid;
	SV_ASSERT(grid.element_index != grid.element_count);

	f32 x = f32(grid.element_index % grid.columns);
	f32 y = f32(grid.element_index / grid.columns);

	f32 x0 = grid.xoff + x * (grid.element_size + grid.padding);
	f32 y0 = y * (grid.element_size + grid.padding);
	f32 x1 = x0 + grid.element_size;
	f32 y1 = y0 + grid.element_size;

	++grid.element_index;

	gui_bounds(gui_, GuiCoord::Pixel(x0), GuiCoord::Pixel(x1), GuiCoord::IPixel(y0), GuiCoord::IPixel(y1));
	gui_begin_container(gui_, id, GuiLayout_Flow);
    }

    void gui_end_grid_element(GUI* gui_)
    {
	//PARSE_GUI();
	//auto& grid = gui.grid;

	gui_end_container(gui_);
    }

    void gui_end_grid(GUI* gui_)
    {
	//	PARSE_GUI();
	//auto& grid = gui.grid;
    }

    void gui_display_style_settings(GUI* gui)
    {
	if (gui_begin_window(gui, "Style Settings", GuiLayout_Flow)) {

	    u64 id = 0u;
	    
	    gui_push_style(gui, GuiStyle_TextAlignment, TextAlignment_Left);
	    
	    gui_same_line(gui, 2u);
	    gui_text(gui, "Container Color", id++);
	    gui_drag_color4(gui, &gui->style.container.color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Color", id++);
	    gui_drag_color4(gui, &gui->style.window.color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Decoration Color", id++);
	    gui_drag_color4(gui, &gui->style.window.decoration_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Outline Color", id++);
	    gui_drag_color4(gui, &gui->style.window.outline_color, id++);
	    
	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Decoration Height", id++);
	    gui_drag_f32(gui, &gui->style.window.decoration_height, 0.01f, 0.f, 1.f, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Min Width", id++);
	    gui_drag_f32(gui, &gui->style.window.min_width, 0.01f, 0.f, 1.f, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Window Min Height", id++);
	    gui_drag_f32(gui, &gui->style.window.min_height, 0.01f, 0.f, 1.f, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Popup Color", id++);
	    gui_drag_color4(gui, &gui->style.popup.color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Button Color", id++);
	    gui_drag_color4(gui, &gui->style.button.color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Button Hot Color", id++);
	    gui_drag_color4(gui, &gui->style.button.hot_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Button Text Color", id++);
	    gui_drag_color4(gui, &gui->style.button.text_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Slider Color", id++);
	    gui_drag_color4(gui, &gui->style.slider.color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Slider Button Color", id++);
	    gui_drag_color4(gui, &gui->style.slider.button_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Slider Button Size", id++);
	    //gui_drag_f32(gui, &gui->style.slider.button_size, 0.01f, 0.f, 1.f, id++);
	    gui_button(gui, "TODO", id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Text Color", id++);
	    gui_drag_color4(gui, &gui->style.label.text_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Text Background Color", id++);
	    gui_drag_color4(gui, &gui->style.label.background_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Checkbox Color", id++);
	    gui_drag_color4(gui, &gui->style.checkbox.button_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Checkbox Background Color", id++);
	    gui_drag_color4(gui, &gui->style.checkbox.background_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Drag Text Color", id++);
	    gui_drag_color4(gui, &gui->style.drag.text_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Drag Background Color", id++);
	    gui_drag_color4(gui, &gui->style.drag.background_color, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Flow x0", id++);
	    gui_drag_f32(gui, &gui->style.flow_layout.x0, 0.01f, 0.f, 1.f, id++);

	    gui_same_line(gui, 2u);
	    gui_text(gui, "Flow x1", id++);
	    gui_drag_f32(gui, &gui->style.flow_layout.x1, 0.01f, 0.f, 1.f, id++);

	    gui_pop_style(gui, 1u);
	    
	    gui_end_window(gui);
	}
    }
}
