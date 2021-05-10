#include "debug/gui.h"

#include "core/renderer.h"

namespace sv {

    enum GuiHeader : u32 {
	
	GuiHeader_BeginWindow,
	GuiHeader_EndWindow,
	
	GuiHeader_Widget,
    };
    
    enum GuiWidgetType : u32 {
	GuiWidgetType_None,
	GuiWidgetType_Root, // Special value, is used to save the focus of the root
	GuiWidgetType_Button,
	GuiWidgetType_Checkbox,
	GuiWidgetType_Drag,
	GuiWidgetType_Text,
	GuiWidgetType_Collapse,
	GuiWidgetType_Image,
    };

    struct GuiWidget {

	v4_f32 bounds;
	GuiWidgetType type;
	u64 id;

	union Widget {

	    // I hate u
	    Widget() : button({}) {}

	    struct {
		const char* text;
		bool hot;
		bool pressed;
	    } button;

	    struct {
		const char* text;
		bool pressed;
		bool value;
	    } checkbox;

	    struct {
		f32 adv;
		f32 min;
		f32 max;
		f32 value;
	    } drag;

	    struct {
		const char* text;
	    } text;

	    struct {
		const char* text;
		bool active;
	    } collapse;

	    struct {
		GPUImage* image;
		GPUImageLayout layout;
	    } image;
	    
	} widget;

	GuiWidget() = default;

    };

    enum GuiRootType : u32 {
	GuiRootType_Screen,
	GuiRootType_Window,
	GuiRootType_Popup
    };

    struct GuiRootInfo {

	List<GuiWidget> widgets;
	v4_f32 bounds = { 0.5f, 0.5f, 1.f, 1.f };
	f32 yoff;
	GuiRootType type = GuiRootType_Screen;
	void* state = nullptr;
	
    };

    struct GuiWindowState {

	bool show;
	bool active;
	char title[GUI_WINDOW_NAME_SIZE + 1u];
	u64 hash;
	GuiRootInfo root_info;

    };

    struct WritingParentInfo {
	GuiWidget* parent;
    };

    struct GUI {

	// CONTEXT
	    
	RawList buffer;
	List<WritingParentInfo> parent_stack;
	
	List<u64> ids;
	u64 current_id;
	
	// STATE
	
	GuiRootInfo root_info;

	GuiRootInfo* current_root;
	
	struct {
	    GuiRootInfo* root;
	    GuiWidgetType type;
	    u64 id;
	    u32 action;
	} focus;

	v2_f32 selection_offset;

	GuiWidget* current_focus = nullptr;

	ThickHashTable<GuiWindowState, 50u> windows;

	// SETTINGS

	f32 scale;

	// PRECOMPUTED

	v2_f32 resolution;
	f32 aspect;
	v2_f32 mouse_position;

    };

    static GUI* gui = nullptr;

    SV_AUX bool mouse_in_bounds(const v4_f32& bounds)
    {
	return abs(gui->mouse_position.x - bounds.x) < bounds.z * 0.5f &&
	    abs(gui->mouse_position.y - bounds.y) < bounds.w * 0.5f;
    }

    SV_AUX v4_f32 compute_checkbox_button(const GuiWidget& w)
    {
	v4_f32 button_bounds = w.bounds;
	button_bounds.z = button_bounds.w / gui->aspect;
	button_bounds.x = button_bounds.x - w.bounds.z * 0.5f + button_bounds.z * 0.5f;
	return button_bounds;
    }
    SV_AUX v4_f32 compute_collapse_button(const GuiWidget& w)
    {
	v4_f32 button_bounds = w.bounds;
	button_bounds.z = button_bounds.w / gui->aspect;
	button_bounds.x = button_bounds.x - w.bounds.z * 0.5f + button_bounds.z * 0.5f;
	return button_bounds;
    }
    SV_AUX v4_f32 compute_window_decoration(const v4_f32& b)
    {
	v4_f32 bounds;
	bounds.x = b.x;
	bounds.z = b.z;
	bounds.w = 0.03f;
	bounds.y = b.y + b.w * 0.5f + bounds.w * 0.5f;
	return bounds;
    }
    
    bool _gui_initialize()
    {
	gui = SV_ALLOCATE_STRUCT(GUI);

	// Get last static state
	{
	    Deserializer d;
	    
	    bool res = bin_read(hash_string("GUI STATE"), d, true);
	    if (res) {

		u32 window_count;
		deserialize_u32(d, window_count);

		foreach(i, window_count) {

		    GuiWindowState s;
		    deserialize_string(d, s.title, GUI_WINDOW_NAME_SIZE);
		    deserialize_bool(d, s.show);
		    deserialize_v4_f32(d, s.root_info.bounds);
		    
		    s.hash = hash_string(s.title);

		    GuiWindowState& state = gui->windows[s.hash];
		    state = s;
		    state.root_info.type = GuiRootType_Window;
		    state.root_info.state = &state;
		}

		deserialize_end(d);
	    }
	    else {
		SV_LOG_WARNING("Can't load the last gui static state");
	    }
	}

	return true;
    }

    bool _gui_close()
    {
	// Save static state
	{
	    Serializer s;

	    serialize_begin(s);

	    serialize_u32(s, u32(gui->windows.size()));

	    for (const GuiWindowState& state : gui->windows) {

		serialize_string(s, state.title);
		serialize_bool(s, state.show);
		serialize_v4_f32(s, state.root_info.bounds);
	    }

	    bool res = bin_write(hash_string("GUI STATE"), s, true);

	    if (!res) {

		SV_LOG_ERROR("Can't save the gui static state");
	    }
	}

	SV_FREE_STRUCT(gui);
	return true;
    }

    bool _gui_begin()
    {
	f32 width = (f32) os_window_size().x;
	f32 height = (f32) os_window_size().y;
	
	gui->resolution = { width, height };
	gui->aspect = width / height;
	
	gui->ids.reset();
	gui->parent_stack.reset();
	
	gui->mouse_position = input.mouse_position + 0.5f;
	
	gui->buffer.reset();

	gui->current_root = &gui->root_info;

	return true;
    }

    template<typename T>
    SV_AUX T gui_read(u8*& it)
    {
	T t;
	memcpy(&t, it, sizeof(T));
	it += sizeof(T);
	return t;
    }

    SV_AUX const char* gui_read_text(u8*& it)
    {
	const char* text = (const char*)it;
	it += strlen(text) + 1u;

	if (*text == '\0') text = nullptr;
	return text;
    }

    SV_AUX void reset_root(GuiRootInfo& root)
    {
	root.widgets.reset();
	root.yoff = 0.1f;
    }

    SV_AUX void set_focus(GuiRootInfo& root, GuiWidgetType type, u64 id, u32 action = 0u)
    {
	gui->focus.root = &root;
	gui->focus.type = type;
	gui->focus.id = id;
	gui->focus.action = action;
    }

    SV_AUX void free_focus()
    {
	gui->focus.root = nullptr;
	gui->focus.type = GuiWidgetType_None;
	gui->focus.id = 0u;
	gui->focus.action = 0u;
    }

    SV_AUX void read_widget(u8*& it)
    {
	GuiWidget& w = gui->current_root->widgets.emplace_back();

	w.bounds = {};
	w.type = gui_read<GuiWidgetType>(it);
	w.id = gui_read<u64>(it);

	f32 height = 0.f;
	f32 width = 0.9f;

	switch (w.type) {
		    
	case GuiWidgetType_Button:
	{
	    auto& button = w.widget.button;
	    button.text = gui_read_text(it);

	    button.hot = false;
	    button.pressed = false;

	    height = 0.1f;
	}
	break;

	case GuiWidgetType_Checkbox:
	{
	    auto& checkbox = w.widget.checkbox;
	    checkbox.text = gui_read_text(it);
	    checkbox.value = gui_read<bool>(it);
	    checkbox.pressed = false;

	    height = 0.1f;
	}
	break;
		
	case GuiWidgetType_Drag:
	{
	    auto& drag = w.widget.drag;
	    drag.adv = gui_read<f32>(it);
	    drag.min = gui_read<f32>(it);
	    drag.max = gui_read<f32>(it);
	    drag.value = gui_read<f32>(it);

	    height = 0.1f;
	}
	break;

	case GuiWidgetType_Text:
	{
	    auto& text = w.widget.text;
	    text.text = gui_read_text(it);

	    height = 0.1f;
	}
	break;

	case GuiWidgetType_Collapse:
	{
	    auto& collapse = w.widget.collapse;
	    collapse.text = gui_read_text(it);
	    collapse.active = gui_read<bool>(it);

	    height = 0.1f;
	}
	break;

	case GuiWidgetType_Image:
	{
	    auto& image = w.widget.image;
	    image.image = gui_read<GPUImage*>(it);
	    image.layout = gui_read<GPUImageLayout>(it);

	    f32 root_aspect = gui->current_root->bounds.z / gui->current_root->bounds.w;
	    height = width * root_aspect * gui->aspect;
	}
	break;
		    
	}
		
	f32& yoff = gui->current_root->yoff;
	w.bounds = { 0.5f, 1.f - (yoff + height * 0.5f), width, height };
	yoff += height + 0.05f;
    }

    SV_AUX void update_root_bounds(GuiRootInfo& root)
    {
	v4_f32 b = root.bounds;
		
	for (GuiWidget& w : root.widgets) {

	    w.bounds.x = (w.bounds.x * b.z) + b.x - (b.z * 0.5f);
	    w.bounds.y = (w.bounds.y * b.w) + b.y - (b.w * 0.5f);
	    w.bounds.z *= b.z;
	    w.bounds.w *= b.w;
	}
    }

    SV_AUX void update_focus()
    {
	if (gui->focus.type == GuiWidgetType_Root) {

	    v4_f32& bounds = gui->focus.root->bounds;

	    f32 min_width = 0.03f;
	    f32 min_height = 0.0f;

	    switch (gui->focus.action)
	    {

	    case 0u:
	    {
		bounds.x = gui->mouse_position.x + gui->selection_offset.x;
		bounds.y = gui->mouse_position.y + gui->selection_offset.y;
	    }
	    break;

	    case 1u: // RIGHT
	    {
		f32 width = SV_MAX(gui->mouse_position.x - (bounds.x - bounds.z * 0.5f), min_width);
		bounds.x -= (bounds.z - width) * 0.5f;
		bounds.z = width;
	    }
	    break;

	    case 2u: // LEFT
	    {
		f32 width = SV_MAX((bounds.x + bounds.z * 0.5f) - gui->mouse_position.x, min_width);
		bounds.x += (bounds.z - width) * 0.5f;
		bounds.z = width;
	    }
	    break;

	    case 3u: // BOTTOM
	    {
		f32 height = SV_MAX((bounds.y + bounds.w * 0.5f) - gui->mouse_position.y, min_height);
		bounds.y += (bounds.w - height) * 0.5f;
		bounds.w = height;
	    }
	    break;

	    case 4u: // RIGHT - BOTTOM
	    {
		f32 width = SV_MAX(gui->mouse_position.x - (bounds.x - bounds.z * 0.5f), min_width);
		bounds.x -= (bounds.z - width) * 0.5f;
		bounds.z = width;
		f32 height = SV_MAX((bounds.y + bounds.w * 0.5f) - gui->mouse_position.y, min_height);
		bounds.y += (bounds.w - height) * 0.5f;
		bounds.w = height;
	    }
	    break;

	    case 5u: // LEFT - BOTTOM
	    {
		f32 width = SV_MAX((bounds.x + bounds.z * 0.5f) - gui->mouse_position.x, min_width);
		bounds.x += (bounds.z - width) * 0.5f;
		bounds.z = width;
		f32 height = SV_MAX((bounds.y + bounds.w * 0.5f) - gui->mouse_position.y, min_height);
		bounds.y += (bounds.w - height) * 0.5f;
		bounds.w = height;
	    }
	    break;

	    }
	    
	    if (input.mouse_buttons[MouseButton_Left] == InputState_None)
		free_focus();
	}
	else if (gui->current_focus) {
	    
	    GuiWidget& w = *gui->current_focus;
	    
	    switch (w.type) {
		    
	    case GuiWidgetType_Button:
	    {
		InputState state = input.mouse_buttons[MouseButton_Left];
		
		if (state == InputState_Released || state == InputState_None) {

		    if (mouse_in_bounds(w.bounds))
			w.widget.button.pressed = true;
		    
		    free_focus();
		}
	    }
	    break;

	    case GuiWidgetType_Checkbox:
	    {
		InputState state = input.mouse_buttons[MouseButton_Left];
		
		if (state == InputState_Released || state == InputState_None) {

		    v4_f32 button_bounds = compute_checkbox_button(w);

		    if (mouse_in_bounds(button_bounds)) {
			w.widget.checkbox.pressed = true;
			w.widget.checkbox.value = !w.widget.checkbox.value;
		    }
		    
		    free_focus();
		}
	    }
	    break;

	    case GuiWidgetType_Drag:
	    {
		auto& drag = w.widget.drag;

		if (input.mouse_buttons[MouseButton_Left] == InputState_None) {
		    free_focus();
		}
		else {

		    f32& value = drag.value;
		    f32 adv = drag.adv;
		    f32 min = drag.min;
		    f32 max = drag.max;
		    
		    value += input.mouse_dragged.x * gui->resolution.x * adv;
		    value = SV_MAX(SV_MIN(value, max), min);
		}
	    }
	    break;

	    case GuiWidgetType_Collapse:
	    {
		InputState state = input.mouse_buttons[MouseButton_Left];
		
		if (state == InputState_Released || state == InputState_None) {

		    if (mouse_in_bounds(w.bounds)) {
			w.widget.collapse.active = !w.widget.collapse.active;
		    }
		    
		    free_focus();
		}
	    }
	    break;
		
	    }
	
	    
	    input.unused = false;
	}
    }

    SV_AUX void update_widget(GuiWidget& w, GuiRootInfo& root)
    {
	const v4_f32& bounds = w.bounds;

	switch (w.type) {

	case GuiWidgetType_Button:
	{
	    if (input.unused) {

		auto& button = w.widget.button;
			
		if (mouse_in_bounds(bounds)) {

		    button.hot = true;

		    if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
			set_focus(root, w.type, w.id);
		    }
		}
		else button.hot = false;
	    }
	} break;

	case GuiWidgetType_Checkbox:
	{
	    if (input.unused) {

		v4_f32 button_bounds = compute_checkbox_button(w);
			
		if (mouse_in_bounds(button_bounds)) {

		    if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
			set_focus(root, w.type, w.id);
		    }
		}
	    }
	} break;

	case GuiWidgetType_Drag:
	case GuiWidgetType_Collapse:
	{
	    if (input.unused) {
			
		if (mouse_in_bounds(bounds)) {

		    if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
			set_focus(root, w.type, w.id);
		    }
		}
	    }
	} break;
		
	}
    }

    SV_INTERNAL void update_root(GuiRootInfo& root)
    {
	GuiWindowState* window = (root.type == GuiRootType_Window) ? (GuiWindowState*)root.state : nullptr;
	
	if (window && !window.active) return;
	    
	for (GuiWidget& w : root.widgets) {
		
	    update_widget(w, root);
	}

	// Update window

	if (window && input.unused) {

	    v4_f32 decoration = compute_window_decoration(root.bounds);

	    // Limit bounds
	    {
		v4_f32& b = root.bounds;

		b.z = math_clamp01(b.z);
		b.w = math_clamp(0.f, b.w, 1.f - decoration.w);
		    
		f32 min_x = b.z * 0.5f;
		f32 max_x = 1.f - min_x;
		f32 min_y = b.w * 0.5f;
		f32 max_y = 1.f - b.w * 0.5f - decoration.w;

		b.x = math_clamp(min_x, b.x, max_x);
		b.y = math_clamp(min_y, b.y, max_y);
	    }

	    if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

		if (mouse_in_bounds(decoration)) {
		    
		    set_focus(root, GuiWidgetType_Root, window.hash, 0u);
		    gui->selection_offset = v2_f32{ root.bounds.x, root.bounds.y } - gui->mouse_position;
		    input.unused = false;
		}

		if (input.unused) {

		    const v4_f32& content = root.bounds;

		    constexpr f32 SELECTION_SIZE = 0.02f;

		    bool right = mouse_in_bounds({ content.x + content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui->aspect });
		    bool left = mouse_in_bounds({ content.x - content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui->aspect });
		    bool bottom = mouse_in_bounds({ content.x, content.y - content.w * 0.5f, content.z + SELECTION_SIZE * 2.f, SELECTION_SIZE * gui->aspect });

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
			set_focus(root, GuiWidgetType_Root, window.hash, action);
		    }
		}
	    }
	}
    }

    void _gui_end()
    {
	// Reset roots

	reset_root(gui->root_info);
	gui->current_focus = nullptr;

	for (GuiWindowState& window : gui->windows) {
	    reset_root(window.root_info);
	    window.active = false;
	}

	// Read buffer
	
	u8* it = gui->buffer.data();
	u8* end = it + gui->buffer.size();

	gui->current_root = &gui->root_info;

	while (it != end) {

	    GuiHeader header = gui_read<GuiHeader>(it);

	    switch (header) {

	    case GuiHeader_BeginWindow:
	    {
		SV_ASSERT(gui->current_root == &gui->root_info);

		u64 hash = gui_read<u64>(it);

		GuiWindowState* state = gui->windows.find(hash);
		SV_ASSERT(state);

		if (state) {
		    gui->current_root = &state->root_info;
		    state->active = true;
		}
	    }
	    break;

	    case GuiHeader_EndWindow:
	    {
		SV_ASSERT(gui->current_root != &gui->root_info);

		update_root_bounds(*gui->current_root);

		// Set new root
		gui->current_root = &gui->root_info;
	    }
	    break;

	    case GuiHeader_Widget:
	    {
		read_widget(it);
	    }
	    break;
		
	    }
	}

	update_root_bounds(gui->root_info);

	// Find focus
	if (gui->focus.type != GuiWidgetType_None && gui->focus.type != GuiWidgetType_Root) {

	    for (GuiWidget& w : gui->focus.root->widgets) {

		if (gui->focus.type == w.type && gui->focus.id == w.id) {
		    gui->current_focus = &w;
		    break;
		}
	    }

	    // Not found
	    if (gui->current_focus == nullptr) {
		free_focus();
	    }
	}

	update_focus();

	// Update focused root
	if (gui->focus.type != GuiWidgetType_None) {

	    update_root(*gui->focus.root);
	}

	// Update windows
	for (GuiWindowState& window : gui->windows) {

	    if (gui->focus.root != &window.root_info)
		update_root(window.root_info);
	}

	// Update screen
	if (gui->focus.root != &gui->root_info)
	    update_root(gui->root_info);
    }
    
    template<typename T>
    SV_AUX void gui_write(const T& t)
    {
	gui->buffer.write_back(&t, sizeof(T));
    }

    SV_AUX void gui_write_text(const char* text)
    {
	gui->buffer.write_back(text, strlen(text) + 1u);
    }

    bool gui_begin_window(const char* title)
    {
	u64 hash = hash_string(title);

	GuiWindowState* state = gui->windows.find(hash);

	if (state == nullptr) {

	    size_t title_size = strlen(title);
	    if (title_size > GUI_WINDOW_NAME_SIZE || title_size == 0u) {
		SV_LOG_ERROR("Invalid window name '%s'", title);
		return false;
	    }

	    GuiWindowState& s = gui->windows[hash];
	    s.root_info.bounds = { 0.5f, 0.5f, 0.1f, 0.3f };
	    s.root_info.type = GuiRootType_Window;
	    s.root_info.state = &s;
	    s.show = true;
	    strcpy(s.title, title);
	    s.hash = hash;
	    state = &s;
	}

	if (state->show) {

	    gui_write(GuiHeader_BeginWindow);
	    gui_write(hash);

	    gui->current_root = &state->root_info;

	    gui_push_id(state->hash);
	}

	return state->show;
    }

    void gui_end_window()
    {
	gui_write(GuiHeader_EndWindow);
	gui->current_root = &gui->root_info;
	gui_pop_id();
    }

    bool gui_show_window(const char* title)
    {
	u64 hash = hash_string(title);
	GuiWindowState* state = gui->windows.find(hash);

	if (state) {

	    state->show = true;
	    return true;
	}

	return false;
    }

    bool gui_hide_window(const char* title)
    {
	u64 hash = hash_string(title);
	GuiWindowState* state = gui->windows.find(hash);

	if (state) {

	    state->show = false;
	    return true;
	}

	return false;
    }

    SV_AUX void update_id()
    {
	gui->current_id = 0U;
	for (u64 id : gui->ids)
	    hash_combine(gui->current_id, id);
    }

    void gui_push_id(u64 id)
    {
	gui->ids.push_back(id);
	update_id();
    }
    
    void gui_push_id(const char* id)
    {
	gui_push_id(u64(id));
    }
    
    void gui_pop_id(u32 count)
    {
	SV_ASSERT(gui->ids.size() >= count);
	gui->ids.pop_back(count);
	update_id();
    }

    SV_AUX void compute_id(u64& id)
    {
	hash_combine(id, gui->current_id);
    }

    SV_AUX GuiWidget* find_widget(GuiWidgetType type, u64 id)
    {
	// TODO: Optimize
	SV_ASSERT(gui->current_root);

	for (GuiWidget& w : gui->current_root->widgets) {

	    if (w.type == type && w.id == id)
		return &w;
	}

	return nullptr;
    }

    bool gui_button(const char* text, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Button);
	gui_write(id);
	gui_write_text(text);
	
	GuiWidget* button = find_widget(GuiWidgetType_Button, id);

	bool pressed = false;

	if (button) {

	    pressed = button->widget.button.pressed;
	}
	
	return pressed;
    }

    bool gui_checkbox(const char* text, bool& value, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Checkbox);
	gui_write(id);
	gui_write_text(text);
	
	GuiWidget* checkbox = find_widget(GuiWidgetType_Checkbox, id);

	bool pressed = false;

	if (checkbox) {

	    pressed = checkbox->widget.checkbox.pressed;
	}

	if (pressed) {
	    value = checkbox->widget.checkbox.value;
	}

	gui_write(value);
	
	return pressed;
    }

    bool gui_checkbox(const char* text, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Checkbox);
	gui_write(id);
	gui_write_text(text);
	
	GuiWidget* checkbox = find_widget(GuiWidgetType_Checkbox, id);

	bool value = checkbox && checkbox->widget.checkbox.value;
	
	gui_write(value);
	
	return value;
    }

    bool gui_drag_f32(f32& value, f32 adv, f32 min, f32 max, u64 id)
    {
	compute_id(id);

	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Drag);
	gui_write(id);
	gui_write(adv);
	gui_write(min);
	gui_write(max);
	
	GuiWidget* drag = find_widget(GuiWidgetType_Drag, id);

	bool pressed = false;

	if (drag && gui->current_focus == drag) {

	    pressed = true;
	}

	if (pressed) {
	    value = drag->widget.drag.value;
	}

	gui_write(value);
	
	return pressed;
    }

    void gui_text(const char* text, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Text);
	gui_write(id);
	gui_write_text(text);
    }

    bool gui_collapse(const char* text, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Collapse);
	gui_write(id);
	gui_write_text(text);
	
	GuiWidget* collapse = find_widget(GuiWidgetType_Collapse, id);

	bool active = collapse && collapse->widget.collapse.active;

	gui_write(active);
	
	return active;
    }

    void gui_image(GPUImage* image, GPUImageLayout layout, u64 id)
    {
	compute_id(id);
	
	gui_write(GuiHeader_Widget);
	gui_write(GuiWidgetType_Image);
	gui_write(id);
	gui_write(image);
	gui_write(layout);
    }

    SV_AUX void draw_root(const GuiRootInfo& root, CommandList cmd) {

	const GuiWindowState* window = (root.type == GuiRootType_Window) ? (const GuiWindowState*)root.state : nullptr;

	if (window) {

	    if (!window->active) return;

	    const v4_f32& b = window->root_info.bounds;
	    v4_f32 decoration = compute_window_decoration(b);
	    
	    imrend_draw_quad({ b.x, b.y, 0.f }, { b.z, b.w }, Color::White(), cmd);
	    imrend_draw_quad({ decoration.x, decoration.y, 0.f }, { decoration.z, decoration.w }, Color::Gray(100u), cmd);
	}

	// Draw widgets
	for (const GuiWidget& w : root.widgets) {

	    switch (w.type) {

	    case GuiWidgetType_Button:
	    {
		auto& button = w.widget.button;

		const v4_f32& b = w.bounds;

		v2_f32 pos = { b.x, b.y };
		v2_f32 size = { b.z, b.w };

		Color color = Color::Salmon();

		if (gui->current_focus == &w)
		    color = Color::White();
		else if (button.hot)
		    color = Color::Red();
		    
		imrend_draw_quad(pos.getVec3(), size, color, cmd);

		if (button.text) {

		    Font& font = renderer_default_font();
			
		    f32 font_size = size.y;

		    imrend_draw_text(button.text, strlen(button.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, Color::Black(), cmd);
		}
	    }
	    break;

	    case GuiWidgetType_Checkbox:
	    {
		v2_f32 pos;
		v2_f32 size;

		auto& cb = w.widget.checkbox;
		v4_f32 b = compute_checkbox_button(w);
		pos = v2_f32{ b.x, b.y };
		size = v2_f32{ b.z, b.w };

		imrend_draw_quad(pos.getVec3(0.f), size, Color::Salmon(), cmd);

		// Check size
		v2_f32 s = size * 0.7f;
	    
		if (cb.value)
		    imrend_draw_quad(pos.getVec3(0.f), s, Color::Red(), cmd);

		size.x = w.bounds.z - size.x;
		pos.x = w.bounds.x + w.bounds.z * 0.5f - size.x * 0.5f;

		imrend_draw_quad(pos.getVec3(0.f), size, Color::Transparent(), cmd);
	    
		if (cb.text) {

		    size.x -= 0.01f; // Minus some margin
		    f32 font_size = size.y;

		    Font& font = renderer_default_font();

		    imrend_draw_text(cb.text, strlen(cb.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, Color::Black(), cmd);
		}
	    }
	    break;

	    case GuiWidgetType_Drag:
	    {
		v2_f32 pos;
		v2_f32 size;

		auto& drag = w.widget.drag;
		pos = v2_f32{ w.bounds.x, w.bounds.y };
		size = v2_f32{ w.bounds.z, w.bounds.w };

		imrend_draw_quad(pos.getVec3(0.f), size, Color::Salmon(), cmd);

		f32 font_size = size.y;

		char strbuff[100u] = "\0";
	    
		sprintf(strbuff, "%f", drag.value);

		Font& font = renderer_default_font();
		imrend_draw_text(strbuff, strlen(strbuff), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, Color::Black(), cmd);
	    }
	    break;

	    case GuiWidgetType_Text:
	    {
		v2_f32 pos;
		v2_f32 size;

		auto& text = w.widget.text;
		pos = v2_f32{ w.bounds.x, w.bounds.y };
		size = v2_f32{ w.bounds.z, w.bounds.w };

		imrend_draw_quad(pos.getVec3(0.f), size, Color::Transparent(), cmd);
	    
		if (text.text) {

		    Font& font = renderer_default_font();
		    f32 font_size = size.y + size.y * font.vertical_offset;

		    imrend_draw_text(text.text, strlen(text.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, Color::Black(), cmd);
		}
	    }
	    break;

	    case GuiWidgetType_Collapse:
	    {
		v2_f32 pos;
		v2_f32 size;

		auto& collapse = w.widget.collapse;

		pos = v2_f32{ w.bounds.x, w.bounds.y };
		size = v2_f32{ w.bounds.z, w.bounds.w };

		imrend_draw_quad(pos.getVec3(0.f), size, Color::Salmon(), cmd);

		// Arrow size
		v4_f32 arrow_bounds = compute_collapse_button(w);
		v2_f32 p = v2_f32(arrow_bounds.x, arrow_bounds.y);
		v2_f32 s = v2_f32(arrow_bounds.z, arrow_bounds.w) * 0.7f;
	    
		if (collapse.active) {

		    imrend_draw_triangle(
			    { p.x - s.x * 0.5f, p.y + s.y * 0.5f, 0.f },
			    { p.x + s.x * 0.5f, p.y + s.y * 0.5f, 0.f },
			    { p.x, p.y - s.y * 0.5f, 0.f }
			    , Color::White(), cmd);
		}
		else {
		    imrend_draw_triangle(
			    { p.x - s.x * 0.5f, p.y + s.y * 0.5f, 0.f },
			    { p.x - s.x * 0.5f, p.y - s.y * 0.5f, 0.f },
			    { p.x + s.x * 0.5f, p.y, 0.f }
			    , Color::White(), cmd);
		}

		size.x = w.bounds.z - arrow_bounds.z;
		pos.x = w.bounds.x + w.bounds.z * 0.5f - size.x * 0.5f;
	    
		if (collapse.text) {

		    size.x -= 0.01f; // Minus some margin
		    f32 font_size = size.y;

		    Font& font = renderer_default_font();

		    imrend_draw_text(collapse.text, strlen(collapse.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, Color::Black(), cmd);
		}
	    }
	    break;

	    case GuiWidgetType_Image:
	    {
		auto& image = w.widget.image;

		v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y);
		v2_f32 size = v2_f32(w.bounds.z, w.bounds.w);
	    
		imrend_draw_sprite(pos.getVec3(), size, Color::White(), image.image ? image.image : renderer_white_image(), image.layout, { 0.f, 0.f, 1.f, 1.f }, cmd);
	    }
	    break;
		    
	    }
		
	}
    }
    
    void _gui_draw(CommandList cmd)
    {
	imrend_begin_batch(cmd);

	imrend_camera(ImRendCamera_Normal, cmd);

	draw_root(gui->root_info, cmd);

	GuiRootInfo* focus = nullptr;

	for (const GuiWindowState& window : gui->windows) {

	    if (gui->focus.root == &window.root_info) {
		focus = &window.root_info;
		continue;
	    }

	    draw_root(window.root_info, cmd);
	}

	if (focus)
	    draw_root(*focus);

	imrend_flush(cmd);
    }
}
