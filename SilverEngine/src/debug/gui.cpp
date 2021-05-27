#include "debug/gui.h"

#include "core/renderer.h"

namespace sv {

    constexpr f32 GUI_DOCKING_SECTION_SIZE = 0.3f;
    constexpr f32 GUI_DOCKING_BUTTON_WIDTH = 0.05f;
    constexpr f32 GUI_DOCKING_BUTTON_HEIGHT = 0.03f;
    constexpr f32 GUI_SCROLL_SIZE = 5.f;

    enum GuiType : u32 {
		GuiType_f32,
		GuiType_v2_f32,
		GuiType_v3_f32,
		GuiType_v4_f32,
		GuiType_u32,
    };

    SV_INTERNAL constexpr size_t sizeof_type(GuiType type)
    {
		switch (type) {
			
		case GuiType_f32:
			return sizeof(f32);
			
		case GuiType_v2_f32:
			return sizeof(v2_f32);

		case GuiType_v3_f32:
			return sizeof(v3_f32);
		
		case GuiType_v4_f32:
			return sizeof(v4_f32);
			
		case GuiType_u32:
			return sizeof(u32);
			
		default:
			return 0;
		}
    }

    SV_INTERNAL constexpr u32 vectorof_type(GuiType type)
    {
		switch (type) {
			
		case GuiType_f32:
		case GuiType_u32:
			return 1u;

		case GuiType_v2_f32:
			return 2u;

		case GuiType_v3_f32:
			return 3u;

		case GuiType_v4_f32:
			return 4u;

		default:
			return 0;
		}
    }

    enum GuiWindowAction : u32 {
		GuiWindowAction_None,
		GuiWindowAction_Move,
		GuiWindowAction_ResizeRight,
		GuiWindowAction_ResizeLeft,
		GuiWindowAction_ResizeBottom,
		GuiWindowAction_ResizeBottomRight,
		GuiWindowAction_ResizeBottomLeft,
		GuiWindowAction_CloseButton,
		GuiWindowAction_SwapWindow,
    };

    enum GuiDockingLocation : u32 {
		GuiDockingLocation_Center,
		GuiDockingLocation_Left,
		GuiDockingLocation_Right,
		GuiDockingLocation_Top,
		GuiDockingLocation_Bottom,
		GuiDockingLocation_MaxEnum
    };

    enum GuiHeader : u32 {
	
		GuiHeader_BeginWindow,
		GuiHeader_EndWindow,
		GuiHeader_BeginPopup,
		GuiHeader_EndPopup,
	
		GuiHeader_Widget,

		GuiHeader_Separator,

		GuiHeader_BeginGrid,
		GuiHeader_EndGrid,
    };
    
    enum GuiWidgetType : u32 {
		GuiWidgetType_None,
		GuiWidgetType_Root, // Special value, is used to save the focus of the root
		GuiWidgetType_Button,
		GuiWidgetType_ImageButton,
		GuiWidgetType_Checkbox,
		GuiWidgetType_Drag,
		GuiWidgetType_Text,
		GuiWidgetType_TextField,
		GuiWidgetType_Collapse,
		GuiWidgetType_Image,
		GuiWidgetType_SelectFilepath,
		GuiWidgetType_AssetButton,
    };

    struct GuiWidget {

		v4_f32 bounds;
		GuiWidgetType type;
		u64 id;
		u32 flags;

		union Widget {

			// I hate u
			Widget() : button({}) {}

			struct {
				const char* text;
				bool pressed;
			} button;

			struct {
				const char* text;
				GPUImage* image;
				v4_f32 texcoord;
				bool pressed;
			} image_button;

			struct {
				const char* text;
				bool pressed;
				bool value;
			} checkbox;

			struct {
				const char* text;
				u8 adv_data[sizeof(f32)];
				u8 min_data[sizeof(f32)];
				u8 max_data[sizeof(f32)];
				u8 value_data[sizeof(v4_f32)];
				GuiType type;
				u32 current_vector = 0u;
			} drag;

			struct {
				const char* text;
			} text;

			struct {
				const char* text;
				size_t buff_size;
			} text_field;

			struct {
				const char* text;
				bool active;
			} collapse;

			struct {
				GPUImage* image;
				f32 height;
				v4_f32 texcoord;
			} image;

			struct {
				const char* text;
				GPUImage* image;
				bool pressed;
			} asset_button;

			struct {
				const char* filepath;
			} select_filepath;
	    
		} widget;

		GuiWidget() = default;

    };

    enum GuiRootType : u32 {
		GuiRootType_Screen,
		GuiRootType_Window,
		GuiRootType_Popup
    };

    enum GuiReadWidgetState : u32 {
		GuiReadWidgetState_Grid
    };

    struct GuiReadWidgetStateData {

		GuiReadWidgetState state;

		struct {
			f32 width;
			f32 padding;
			f32 xoff;
		};
    };

    struct GuiWindow;
    
    struct GuiRootInfo {

		List<GuiWidget> widgets;
		List<GuiReadWidgetStateData> read_widget_state_stack;
	
		// Thats where the widgets are projected.
		// the window has is own bounds
		v4_f32 widget_bounds = { 0.5f, 0.5f, 1.f, 1.f };
	
		f32 yoff = 0.f;
		f32 vertical_offset = 0.f;
		GuiRootType type = GuiRootType_Screen;
		GuiWindow* window = nullptr;
	
    };

    struct GuiWindowState {

		bool show;
		char title[GUI_WINDOW_NAME_SIZE + 1u];
		u64 hash;
		u32 flags;

    };

    struct GuiWindow {

		List<GuiWindowState*> states;
		u32 current_index = 0u;
		GuiRootInfo root = {};
		u32 priority = 0u;
		v4_f32 bounds = { 0.5f, 0.5f, 0.2f, 0.35f };

		struct {
			u32 selected_window_button = 0u;
			v2_f32 selection_point;
		} focus_data;
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

		bool popup_request = false;

		struct {
			u64 id;
			GuiWidgetType type;
			GuiRootInfo* root;
		} last_widget;
	
		// STATE

		GuiStyle style;
	
		GuiRootInfo root_info;
		List<GuiRootInfo*> root_stack;
		// TODO: Use indexed list
		GuiWindow windows[100u];
		u32 window_count = 0u;
		List<GuiWindow*> sorted_windows;

		u32 priority_count = 0u;

		struct {
			GuiRootInfo* root;
			GuiWidgetType type;
			u64 id;
			u32 action;
		} focus;

		GuiWidget* current_focus = nullptr;

		ThickHashTable<GuiWindowState, 50u> window_states;

		struct  {
			GuiRootInfo root;
			v2_f32 origin;
			u64 id = 0u;
		} popup;

		RawList text_field_buffer;
		u32 text_field_cursor = 0u;

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
    // TODO: This repeats operations
    SV_AUX v4_f32 compute_drag_slot(const GuiWidget& drag, u32 index)
    {
		const v4_f32& bounds = drag.bounds;
		u32 vector = vectorof_type(drag.widget.drag.type);

		f32 drag_width = bounds.z;
		f32 off = 0.f;

		if (drag.widget.drag.text) {
			drag_width *= 0.5f;
			off = drag_width;
		}

		f32 width = drag_width / f32(vector);
	
		f32 padding = 3.f / gui->resolution.x;

		v4_f32 b = bounds;
		b.z = (drag_width - padding * f32(vector - 1u)) / f32(vector);
		b.x = (bounds.x - bounds.z * 0.5f) + (width * f32(index)) + b.z * 0.5f;
		b.x += off;
		return b;
    }
    SV_AUX v4_f32 compute_drag_text(const GuiWidget& drag)
    {
		v4_f32 b = drag.bounds;
		b.x = b.x - b.z * 0.5f;
		b.z *= 0.5f;
		b.x += b.z * 0.5f;
		return b;
    }
    SV_AUX v4_f32 compute_window_decoration(const GuiRootInfo& root)
    {
		SV_ASSERT(root.window);
		v4_f32 b = root.window->bounds;
		v4_f32 bounds;
		bounds.x = b.x;
		bounds.z = b.z;
		bounds.w = 25.f / gui->resolution.y;
		bounds.y = b.y + b.w * 0.5f + bounds.w * 0.5f;
		return bounds;
    }
    SV_AUX v4_f32 compute_window_closebutton(const v4_f32& decoration)
    {
		v4_f32 bounds;
		bounds.w = decoration.w * 0.5f;
		bounds.z = bounds.w / gui->aspect;
		bounds.x = decoration.x + decoration.z * 0.5f - bounds.z * 1.5f;
		bounds.y = decoration.y;
		return bounds;
    }
    SV_AUX f32 compute_window_buttons_height(const GuiRootInfo& root)
    {
		return 20.f / gui->resolution.y;
    }
    SV_AUX v4_f32 compute_window_buttons(const GuiRootInfo& root)
    {
		v4_f32 b = root.widget_bounds;
		f32 height = compute_window_buttons_height(root);
		b.y += b.w * 0.5f + height * 0.5f;
		b.w = height;
		return b;
    }
    SV_AUX v4_f32 compute_window_button(const GuiRootInfo& root, v4_f32 buttons, u32 index)
    {
		f32 width = 50.f / gui->resolution.x;
		f32 padding = 3.f / gui->resolution.x;
	
		v4_f32 b;
		b.y = buttons.y;
		b.w = buttons.w;
		b.z = width;

		b.x = (buttons.x - buttons.z * 0.5f) + f32(index) * (width + padding) + width * 0.5f;
	
		return b;
    }

    SV_AUX bool root_has_scroll(const GuiRootInfo& root)
    {
		return &root != &gui->root_info && (root.yoff > root.widget_bounds.w * gui->resolution.y);
    }
    /*
	  SV_AUX void compute_docking_location(GuiDockingLocation& location, GuiRootInfo*& root)
	  {
	  root = &gui->root_info;

	  for (GuiRootInfo* r : gui->roots) {

	  if (r == root)
	  continue;
	    
	  if (mouse_in_bounds(r->bounds)) {
	  root = r;
	  break;
	  }
	  }

	  f32 CENTER_WIDTH = 0.5f * root->bounds.z;
	  f32 CENTER_HEIGHT = CENTER_WIDTH / gui->aspect;

	  v2_f32 point = gui->mouse_position * v2_f32(root->bounds.z, root->bounds.w) + v2_f32(root->bounds.x, root->bounds.y) - v2_f32(root->bounds.z, root->bounds.w) * 0.5f;
	  point -= v2_f32(root->bounds.x, root->bounds.y) * v2_f32(root->bounds.z, root->bounds.w);
	
	  if (abs(point.x) <= CENTER_WIDTH * 0.5f && abs(point.y) <= CENTER_HEIGHT * 0.5f) {
	  location = GuiDockingLocation_Center;
	  return;
	  }

	  if (abs(point.x) > abs(point.y)) {

	  if (point.x < 0.f) {

	  location = GuiDockingLocation_Left;
	  return;
	  }

	  location = GuiDockingLocation_Right;
	  return;
	  }
	  else {
	  if (point.y < 0.f) {

	  location = GuiDockingLocation_Bottom;
	  return;
	  }

	  location = GuiDockingLocation_Top;
	  return;
	  }
	  }
	  SV_AUX v4_f32 compute_docking_button(GuiDockingLocation location, GuiRootInfo* root, bool selected)
	  {
	  constexpr f32 SIZE = GUI_DOCKING_SECTION_SIZE;
	  constexpr f32 BUTTON_WIDTH = GUI_DOCKING_BUTTON_WIDTH;
	  constexpr f32 BUTTON_HEIGHT = GUI_DOCKING_BUTTON_HEIGHT;

	  v4_f32 b;
	  b.z = BUTTON_WIDTH;
	  b.w = BUTTON_HEIGHT * gui->aspect;

	  switch (location) {

	  case GuiDockingLocation_Center:
	  b.x = 0.5f;
	  b.y = 0.5f;
	  break;

	  case GuiDockingLocation_Left:
	  b.x = SIZE * 0.5f;
	  b.y = 0.5f;
	  break;

	  case GuiDockingLocation_Right:
	  b.x = 1.f - SIZE * 0.5f;
	  b.y = 0.5f;
	  break;

	  case GuiDockingLocation_Bottom:
	  b.y = SIZE * 0.5f;
	  b.x = 0.5f;
	  break;

	  case GuiDockingLocation_Top:
	  b.y = 1.f - SIZE * 0.5f;
	  b.x = 0.5f;
	  break;
	    
	  }

	  if (selected) {
	  b.z *= 1.3f;
	  b.w *= 1.3f;
	  }

	  const v4_f32& p = root->bounds;
	  b.x = (b.x * p.z) + p.x - p.z * 0.5f;
	  b.y = (b.y * p.w) + p.y - p.w * 0.5f;
	  b.z *= p.z;
	  b.w *= p.w;
	
	  return b;
	  }
	  SV_AUX v4_f32 compute_docking_section(GuiDockingLocation location, GuiRootInfo* root)
	  {
	  constexpr f32 SIZE = GUI_DOCKING_SECTION_SIZE;
	
	  v4_f32 b;

	  switch (location) {

	  case GuiDockingLocation_Center:
	  b.x = 0.5f;
	  b.y = 0.5f;
	  b.z = 1.f;
	  b.w = 1.f;
	  break;

	  case GuiDockingLocation_Left:
	  b.x = SIZE * 0.5f;
	  b.y = 0.5f;
	  b.z = SIZE;
	  b.w = 1.f;
	  break;

	  case GuiDockingLocation_Right:
	  b.x = 1.f - SIZE * 0.5f;
	  b.y = 0.5f;
	  b.z = SIZE;
	  b.w = 1.f;
	  break;

	  case GuiDockingLocation_Bottom:
	  b.y = SIZE * 0.5f;
	  b.x = 0.5f;
	  b.z = 1.f;
	  b.w = SIZE;
	  break;

	  case GuiDockingLocation_Top:
	  b.y = 1.f - SIZE * 0.5f;
	  b.x = 0.5f;
	  b.z = 1.f;
	  b.w = SIZE;
	  break;
	    
	  }

	  const v4_f32& p = root->bounds;
	  b.x = (b.x * p.z) + p.x - p.z * 0.5f;
	  b.y = (b.y * p.w) + p.y - p.w * 0.5f;
	  b.z *= p.z;
	  b.w *= p.w;
	
	  return b;
	  }*/
    
    bool _gui_initialize()
    {
		gui = SV_ALLOCATE_STRUCT(GUI);

		// Get last static state
		/*{
		  Deserializer d;
	    
		  bool res = bin_read(hash_string("GUI STATE"), d, true);
		  if (res) {

		  u32 window_count;
		  deserialize_u32(d, window_count);

		  foreach(i, window_count) {
		    
		  GuiWindowState s;
		  deserialize_bool(d, s.show);
		  deserialize_string(d, s.title, GUI_WINDOW_NAME_SIZE);
		  deserialize_v4_f32(d, s.root_info.bounds);

		  s.flags = 0u;
		  s.hash = hash_string(s.title);

		  GuiWindowState& state = gui->window_states[s.hash];
		  state = s;
		  state.root_info.type = GuiRootType_Window;
		  state.root_info.state = &state;
		  }

		  deserialize_end(d);
		  }
		  else {
		  SV_LOG_WARNING("Can't load the last gui static state");
		  }
		  }*/

		gui->popup.root.type = GuiRootType_Popup;

		return true;
    }

    bool _gui_close()
    {
		// Save static state
		/*{
		  Serializer s;

		  serialize_begin(s);

		  serialize_u32(s, u32(gui->window_states.size()));

		  for (const GuiWindowState& state : gui->window_states) {

		  serialize_bool(s, state.show);
		  serialize_string(s, state.title);
		  serialize_v4_f32(s, state.root_info.bounds);
		  }

		  bool res = bin_write(hash_string("GUI STATE"), s, true);

		  if (!res) {

		  SV_LOG_ERROR("Can't save the gui static state");
		  }
		  }*/

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

		gui->root_stack.reset();
		gui->root_stack.push_back(&gui->root_info);

		gui->last_widget.id = 0u;
		gui->last_widget.type = GuiWidgetType_None;
		gui->last_widget.root = nullptr;

		gui->popup_request = false;

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

    SV_AUX void gui_read_raw(void* dst, size_t size, u8*& it)
    {
		memcpy(dst, it, size);
		it += size;
    }

    SV_AUX void reset_root(GuiRootInfo& root)
    {
		root.widgets.reset();
		root.yoff = 7.f;
    }

    SV_AUX void set_focus(GuiRootInfo& root, GuiWidgetType type, u64 id, u32 action = 0u)
    {
		if (action != u32_max || type != GuiWidgetType_Root) {
	    
			gui->focus.root = &root;
			gui->focus.type = type;
			gui->focus.id = id;
			gui->focus.action = action;
		}

		if (root.type == GuiRootType_Window)
			root.window->priority = ++gui->priority_count;
    }

    SV_AUX void free_focus()
    {
		gui->focus.root = nullptr;
		gui->focus.type = GuiWidgetType_None;
		gui->focus.id = 0u;
		gui->focus.action = 0u;
    }

    SV_AUX void compute_widget_bounds(GuiRootInfo& root, GuiWidget& w)
    {
		GuiReadWidgetStateData* state_data = root.read_widget_state_stack.empty() ? nullptr : &root.read_widget_state_stack.back();

		if (state_data == nullptr) {
	    
			f32 height = 0.f;
			f32 width = 0.9f;

			switch (w.type) {
		    
			case GuiWidgetType_Button:
			case GuiWidgetType_Checkbox:		
			case GuiWidgetType_Drag:
			case GuiWidgetType_Text:
			case GuiWidgetType_TextField:
			case GuiWidgetType_SelectFilepath:
				height = 25.f;
				break;

			case GuiWidgetType_Collapse:
				height = 35.f;
				break;

			case GuiWidgetType_ImageButton:
				height = 40.f;
				break;

			case GuiWidgetType_Image:
			{
				if (w.flags & GuiImageFlag_Fullscreen) {

					root.yoff = 0.f;
					width = 1.f;
					height = root.widget_bounds.w * gui->resolution.y;
				}
				else {

					auto& image = w.widget.image;
		
					height = image.height;
					width = height / root.widget_bounds.z / gui->resolution.x;
				}
			}
			break;
		    
			}

			f32 separation = 5.f;
	
			w.bounds = { 0.5f, root.yoff + height * 0.5f, width, height };
			root.yoff += height + separation;
		}
		else if (state_data->state == GuiReadWidgetState_Grid) {

			auto& data = *state_data;

			f32 relative_space = 0.95f;

			f32 space = relative_space * root.widget_bounds.z * gui->resolution.x;

			f32 x0 = data.xoff;
			f32 x1 = x0 + data.width;

			if (x1 > space && data.xoff != 0.f) {

				data.xoff = 0.f;
				root.yoff += data.padding + data.width;

				x0 = 0.f;
				x1 = data.width;
			}

			data.xoff += (x1 - x0) + data.padding;

			x0 = x0 / gui->resolution.x / root.widget_bounds.z + (1.f - relative_space) * 0.5f;
			x1 = x1 / gui->resolution.x / root.widget_bounds.z + (1.f - relative_space) * 0.5f;

			w.bounds.z = x1 - x0;
			w.bounds.x = x0 + w.bounds.z * 0.5f;
			w.bounds.y = root.yoff + data.width * 0.5f;
			w.bounds.w = data.width;
		}
    }

    SV_AUX void read_widget(u8*& it)
    {
		GuiWidget& w = gui->root_stack.back()->widgets.emplace_back();

		w.bounds = {};
		w.type = gui_read<GuiWidgetType>(it);
		w.id = gui_read<u64>(it);
		w.flags = gui_read<u32>(it);

		GuiRootInfo& root = *gui->root_stack.back();
	
		switch (w.type) {
		    
		case GuiWidgetType_Button:
		{
			auto& button = w.widget.button;
			button.text = gui_read_text(it);

			button.pressed = false;
		}
		break;

		case GuiWidgetType_ImageButton:
		{
			auto& button = w.widget.image_button;
			button.text = gui_read_text(it);
			button.image = gui_read<GPUImage*>(it);
			button.texcoord = gui_read<v4_f32>(it);

			button.pressed = false;
		}
		break;

		case GuiWidgetType_Checkbox:
		{
			auto& checkbox = w.widget.checkbox;
			checkbox.text = gui_read_text(it);
			checkbox.value = gui_read<bool>(it);
			checkbox.pressed = false;
		}
		break;
		
		case GuiWidgetType_Drag:
		{
			auto& drag = w.widget.drag;

			bool has_text = gui_read<bool>(it);
			if (has_text)
				drag.text = gui_read_text(it);
			else
				drag.text = nullptr;
	    
			drag.type = gui_read<GuiType>(it);

			size_t size = sizeof_type(drag.type);
			size_t comp_size = size / vectorof_type(drag.type);
	    
			gui_read_raw(drag.adv_data, comp_size, it);
			gui_read_raw(drag.min_data, comp_size, it);
			gui_read_raw(drag.max_data, comp_size, it);
			gui_read_raw(drag.value_data, size, it);

			drag.current_vector = gui_read<u32>(it);
		}
		break;

		case GuiWidgetType_Text:
		{
			auto& text = w.widget.text;
			text.text = gui_read_text(it);
		}
		break;

		case GuiWidgetType_TextField:
		{
			auto& text = w.widget.text_field;
			text.text = gui_read_text(it);
			text.buff_size = gui_read<size_t>(it);
		}
		break;

		case GuiWidgetType_Collapse:
		{
			auto& collapse = w.widget.collapse;
			collapse.text = gui_read_text(it);
			collapse.active = gui_read<bool>(it);
		}
		break;

		case GuiWidgetType_Image:
		{
			auto& image = w.widget.image;
			image.height = gui_read<f32>(it);
			image.image = gui_read<GPUImage*>(it);
			image.texcoord = gui_read<v4_f32>(it);
		}
		break;

		case GuiWidgetType_SelectFilepath:
		{
			auto& select = w.widget.select_filepath;
			select.filepath = gui_read_text(it);
		}
		break;

		case GuiWidgetType_AssetButton:
		{
			auto& asset = w.widget.asset_button;
			asset.text = gui_read_text(it);
			asset.image = gui_read<GPUImage*>(it);
		}
		break;
		    
		}
	
		compute_widget_bounds(root, w);
    }

    SV_AUX void update_root_bounds(GuiRootInfo& root)
    {
		// Update the widget bounds if it was a window
		if (root.type == GuiRootType_Window) {

			GuiWindow& win = *root.window;
			v4_f32& b = root.widget_bounds;
	    
			b = win.bounds;

			if (win.states.size() > 1u) {

				f32 height = compute_window_buttons_height(root);

				b.y -= height * 0.5f;
				b.w -= height;
			}
		}
	
		v4_f32 b = root.widget_bounds;
	
		f32 scale = gui->style.scale;

		f32 inv_height = 1.f / gui->resolution.y * scale;
	
		root.yoff *= scale;

		f32 root_width = b.z;

		if (root_has_scroll(root)) {
			f32 scroll = GUI_SCROLL_SIZE / gui->resolution.x;
			root_width = SV_MAX(root_width - scroll, 0.f);
		}
		else root.vertical_offset = 0.f;

		for (GuiWidget& w : root.widgets) {

			w.bounds.x = (w.bounds.x * root_width) + b.x - (root_width * 0.5f);
			w.bounds.y = (1.f - w.bounds.y) * inv_height + b.y + b.w * 0.5f + root.vertical_offset;
			w.bounds.z *= root_width;
			w.bounds.w *= inv_height;
		}
    }

    SV_AUX void update_focus()
    {
		if (gui->focus.type == GuiWidgetType_None)
			return;
	
		if (gui->focus.type == GuiWidgetType_Root && gui->focus.root->type == GuiRootType_Window) {

			v4_f32& bounds = gui->focus.root->window->bounds;

			f32 min_width = 0.03f;
			f32 min_height = 0.0f;

			switch ((GuiWindowAction)gui->focus.action)
			{

			case GuiWindowAction_SwapWindow:
			{
				GuiWindow* w = gui->focus.root->window;
				u32 index = w->focus_data.selected_window_button;

				if (index < w->states.size()) {

					v2_f32& point = gui->focus.root->window->focus_data.selection_point;

					if ((point - gui->mouse_position).length() > 0.03f) {

						GuiWindow& old_win = *gui->focus.root->window;
						GuiWindow& new_win = gui->windows[gui->window_count++];

						new_win.root.type = GuiRootType_Window;
						new_win.root.window = &new_win;
						new_win.states.push_back(old_win.states[index]);
						new_win.focus_data.selection_point = { 0.f, -new_win.bounds.w * 0.5f };

						old_win.current_index = 0u;
						old_win.states.erase(index);
			
						set_focus(new_win.root, GuiWidgetType_Root, 0u, GuiWindowAction_Move);
					}
					else if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {
		    
						v4_f32 buttons_bounds = compute_window_buttons(*gui->focus.root);
						v4_f32 bounds = compute_window_button(*gui->focus.root, buttons_bounds, index);

						if (mouse_in_bounds(bounds)) {
							w->current_index = index;
						}
					}
				}
			}
			break;

			case GuiWindowAction_Move:
			{
				if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {

					for (GuiWindow* w : gui->sorted_windows) {

						GuiWindow* r = gui->focus.root->window;
						if (w == r)
							continue;

						if (mouse_in_bounds(w->root.widget_bounds)) {

							w->states.insert(r->states);
							r->states.reset();
							break;
						}
					}
				}
				else {
					v2_f32& point = gui->focus.root->window->focus_data.selection_point;
					bounds.x = gui->mouse_position.x + point.x;
					bounds.y = gui->mouse_position.y + point.y;
				}
			}
			break;

			case GuiWindowAction_ResizeRight:
			{
				f32 width = SV_MAX(gui->mouse_position.x - (bounds.x - bounds.z * 0.5f), min_width);
				bounds.x -= (bounds.z - width) * 0.5f;
				bounds.z = width;
			}
			break;

			case GuiWindowAction_ResizeLeft:
			{
				f32 width = SV_MAX((bounds.x + bounds.z * 0.5f) - gui->mouse_position.x, min_width);
				bounds.x += (bounds.z - width) * 0.5f;
				bounds.z = width;
			}
			break;

			case GuiWindowAction_ResizeBottom:
			{
				f32 height = SV_MAX((bounds.y + bounds.w * 0.5f) - gui->mouse_position.y, min_height);
				bounds.y += (bounds.w - height) * 0.5f;
				bounds.w = height;
			}
			break;

			case GuiWindowAction_ResizeBottomRight:
			{
				f32 width = SV_MAX(gui->mouse_position.x - (bounds.x - bounds.z * 0.5f), min_width);
				bounds.x -= (bounds.z - width) * 0.5f;
				bounds.z = width;
				f32 height = SV_MAX((bounds.y + bounds.w * 0.5f) - gui->mouse_position.y, min_height);
				bounds.y += (bounds.w - height) * 0.5f;
				bounds.w = height;
			}
			break;

			case GuiWindowAction_ResizeBottomLeft:
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

			InputState state = input.mouse_buttons[MouseButton_Left];

			if (state == InputState_Released || state == InputState_None) {

				if (gui->focus.action == GuiWindowAction_CloseButton) {

					v4_f32 decoration = compute_window_decoration(*gui->focus.root);
					v4_f32 closebutton = compute_window_closebutton(decoration);
		    
					if (mouse_in_bounds(closebutton)) {

						GuiWindow* win = gui->focus.root->window;
						SV_ASSERT(win);

						if (win) {

							foreach(i, win->states.size()) {

								GuiWindowState* state = win->states[i];

								if (!(state->flags & GuiWindowFlag_NoClose))
									gui_hide_window(state->title);
							}
						}
					}
				}
		
				free_focus();
			}
		
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

			case GuiWidgetType_TextField:
			{
				InputState state = input.mouse_buttons[MouseButton_Left];
				
				if (input.keys[Key_Enter] || (state && !mouse_in_bounds(w.bounds))) {
					free_focus();
				}
				else {

					auto& field = w.widget.text_field;

					if (field.buff_size != gui->text_field_buffer.size()) {
						gui->text_field_buffer.resize(field.buff_size);

						const char* text = field.text ? field.text : NULL;
						
						string_copy((char*)gui->text_field_buffer.data(), text, field.buff_size);
						gui->text_field_cursor = (u32)string_size(text);
					}
					
					string_modify((char*)gui->text_field_buffer.data(), field.buff_size, gui->text_field_cursor, NULL);
				}
			}
			break;

			case GuiWidgetType_ImageButton:
			{
				InputState state = input.mouse_buttons[MouseButton_Left];
		
				if (state == InputState_Released || state == InputState_None) {

					if (mouse_in_bounds(w.bounds))
						w.widget.image_button.pressed = true;
		    
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

					switch (drag.type) {

					case GuiType_f32:
					case GuiType_v2_f32:
					case GuiType_v3_f32:
					case GuiType_v4_f32:
					{
						f32* value = nullptr;
						f32 adv = *reinterpret_cast<f32*>(drag.adv_data);
						f32 min = *reinterpret_cast<f32*>(drag.min_data);
						f32 max = *reinterpret_cast<f32*>(drag.max_data);
			    
						if (drag.type == GuiType_f32) {
			    
							value = reinterpret_cast<f32*>(drag.value_data);
						}
						else if (drag.type == GuiType_v2_f32) {
			    
							v2_f32* value_vec = reinterpret_cast<v2_f32*>(drag.value_data);
							value = &((*value_vec)[drag.current_vector]);
						}
						else if (drag.type == GuiType_v3_f32) {
			    
							v3_f32* value_vec = reinterpret_cast<v3_f32*>(drag.value_data);
							value = &((*value_vec)[drag.current_vector]);
						}
						else if (drag.type == GuiType_v4_f32) {
			    
							v4_f32* value_vec = reinterpret_cast<v4_f32*>(drag.value_data);
							value = &((*value_vec)[drag.current_vector]);
						}

						if (value) {
			    
							*value += input.mouse_dragged.x * gui->resolution.x * adv;
							*value = math_clamp(min, *value, max);
						}
					}
					break;

					case GuiType_u32:
					{
						u32* value = nullptr;
						u32 adv = *reinterpret_cast<u32*>(drag.adv_data);
						u32 min = *reinterpret_cast<u32*>(drag.min_data);
						u32 max = *reinterpret_cast<u32*>(drag.max_data);
			    
						if (drag.type == GuiType_u32) {
			    
							value = reinterpret_cast<u32*>(drag.value_data);
						}

						if (value) {

							i32 n = i32(input.mouse_dragged.x * gui->resolution.x * adv);
							*value = (-n > (i32)*value) ? 0u : (*value + n);
							*value = math_clamp(min, *value, max);
						}
					}
					break;
		    
					}
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

			case GuiWidgetType_AssetButton:
			{
				InputState state = input.mouse_buttons[MouseButton_Left];
		
				if (state == InputState_Released || state == InputState_None) {

					if (mouse_in_bounds(w.bounds))
						w.widget.asset_button.pressed = true;
		    
					free_focus();
				}
			}
			break;
		
			}
		}

		input.unused = false;
    }

    SV_AUX void update_widget(GuiWidget& w, GuiRootInfo& root)
    {
		const v4_f32& bounds = w.bounds;

		switch (w.type) {

		case GuiWidgetType_Button:
		case GuiWidgetType_ImageButton:
		case GuiWidgetType_AssetButton:
		case GuiWidgetType_Collapse:
		case GuiWidgetType_TextField:
		{
			if (input.unused) {
			
				if (mouse_in_bounds(bounds)) {

					if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
						set_focus(root, w.type, w.id);
						input.unused = true;
						gui->text_field_cursor;
						gui->text_field_buffer.clear();
					}
				}
			}
		} break;

		case GuiWidgetType_Checkbox:
		{
			if (input.unused) {

				v4_f32 button_bounds = compute_checkbox_button(w);
			
				if (mouse_in_bounds(button_bounds)) {

					if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
						set_focus(root, w.type, w.id);
						input.unused = true;
					}
				}
			}
		} break;

		case GuiWidgetType_Drag:
		{
			if (input.unused) {

				auto& drag = w.widget.drag;

				if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
		    
					u32 vector = vectorof_type(drag.type);
					v4_f32 bounds;
		
					foreach(i, vector) {
			
						bounds = compute_drag_slot(w, i);

						if (mouse_in_bounds(bounds)) {
			    
							set_focus(root, w.type, w.id);
							drag.current_vector = i;
							input.unused = true;
							break;
						}
					}
				}
			}
		} break;

		case GuiWidgetType_SelectFilepath:
		{
			// TODO
			/*auto& select = w.widget.select_filepath;

			  const char* filepath = select.filepath ? select.filepath : "";
			  const char* it = filepath;

			  u32 folder_count = 0u;

			  while (*it) {

			  //v4_f32 b = compute_select_filepath_folder(it, folder_count, renderer_default_font());
			  ++folder_count;
			  }*/
		}
		break;
		
		}
    }

    SV_INTERNAL void update_root(GuiRootInfo& root)
    {
		if (mouse_in_bounds(root.widget_bounds)) {
	    
			for (GuiWidget& w : root.widgets) {
		
				update_widget(w, root);
			}
		}

		bool catch_input = false;

		switch (root.type) {

		case GuiRootType_Window:
		{
			if (input.unused) {
		
				v4_f32 decoration = compute_window_decoration(root);
				if (mouse_in_bounds(root.window->bounds) || mouse_in_bounds(decoration))
					catch_input = true;

				// Limit bounds
				{
					v4_f32& b = root.window->bounds;

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

						// TODO: !(window->current_state->flags & GuiWindowFlag_NoClose)
						if (mouse_in_bounds(compute_window_closebutton(decoration))) {
							set_focus(root, GuiWidgetType_Root, 0u, GuiWindowAction_CloseButton);
						}
						else {
			    
							set_focus(root, GuiWidgetType_Root, 0u, GuiWindowAction_Move);
			    
							v4_f32 b = root.window->bounds;
							v2_f32& point = gui->focus.root->window->focus_data.selection_point;
			    
							point = v2_f32{ b.x, b.y } - gui->mouse_position;
						}
		    
						input.unused = false;
					}

					if (input.unused) {

						v4_f32 buttons = compute_window_buttons(root);

						if (mouse_in_bounds(buttons)) {

							foreach(i, root.window->states.size()) {

								v4_f32 b = compute_window_button(root, buttons, i);
								if (mouse_in_bounds(b)) {

									set_focus(root, GuiWidgetType_Root, 0u, GuiWindowAction_SwapWindow);
									root.window->focus_data.selected_window_button = i;
									input.unused = false;
									gui->focus.root->window->focus_data.selection_point = gui->mouse_position;
									break;
								}
							}
						}
					}

					if (input.unused) {

						const v4_f32& content = root.window->bounds;

						constexpr f32 SELECTION_SIZE = 0.02f;

						bool right = mouse_in_bounds({ content.x + content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui->aspect });
						bool left = mouse_in_bounds({ content.x - content.z * 0.5f, content.y, SELECTION_SIZE, content.w + SELECTION_SIZE * 2.f * gui->aspect });
						bool bottom = mouse_in_bounds({ content.x, content.y - content.w * 0.5f, content.z + SELECTION_SIZE * 2.f, SELECTION_SIZE * gui->aspect });

						GuiWindowAction action = GuiWindowAction_None;

						if (right && bottom) {
							action = GuiWindowAction_ResizeBottomRight;
						}
						else if (left && bottom) {
							action = GuiWindowAction_ResizeBottomLeft;
						}
						else if (right) {
							action = GuiWindowAction_ResizeRight;
						}
						else if (left) {
							action = GuiWindowAction_ResizeLeft;
						}
						else if (bottom) {
							action = GuiWindowAction_ResizeBottom;
						}

						if (action != GuiWindowAction_None) {
							input.unused = false;
							set_focus(root, GuiWidgetType_Root, 0u, action);
						}
					}
				}
			}
		}
		break;

		case GuiRootType_Popup:
		{
			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

				if (!mouse_in_bounds(root.widget_bounds)) {
					gui->popup.id = 0u;
					input.unused = false;
				}
			}

			if (mouse_in_bounds(root.widget_bounds))
				catch_input = true;
		}
		break;
	    
		}

		if (catch_input) {
	    
			if (input.unused) {

				input.unused = false;
		
				if (gui->focus.type == GuiWidgetType_None && input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {
					set_focus(root, GuiWidgetType_Root, 0u, u32_max);
				}

				// Scroll
				if (root_has_scroll(root)) {
					root.vertical_offset -= input.mouse_wheel / gui->resolution.y * 8.f;

					f32 max = root.yoff / gui->resolution.y - root.widget_bounds.w;
					root.vertical_offset = math_clamp(0.f, root.vertical_offset, max);
				}
			}
		}
    }

    SV_AUX GuiWindow* find_window_from_state(GuiWindowState* state)
    {
		foreach(i, gui->window_count) {

			for (GuiWindowState* s : gui->windows[i].states) {
		
				if (s == state) {
					return &gui->windows[i];
				}
			}
		}

		return nullptr;
    }

    void _gui_end()
    {
		// Reset roots

		{
			u32 i = 0u;
			while (i < gui->window_count) {

				GuiWindow& win = gui->windows[i];

				for (auto it = win.states.begin(); it != win.states.end();) {

					GuiWindowState* state = *it;
					if (!state->show)
						it = win.states.erase(it);
					else
						++it;
				}
		
				if (win.states.empty()) {

					win = {};

					if (i != gui->window_count - 1u) {

						win = gui->windows[gui->window_count - 1u];
						win.root.window = &win;
						gui->windows[gui->window_count - 1u] = {};
					}

					--gui->window_count;
				}
				else ++i;
			}
		}

		gui->sorted_windows.reset();

		reset_root(gui->root_info);
		gui->current_focus = nullptr;

		reset_root(gui->popup.root);

		foreach(i, gui->window_count) {
			reset_root(gui->windows[i].root);
		}

		// Read buffer
	
		u8* it = gui->buffer.data();
		u8* end = it + gui->buffer.size();
	
		gui->root_stack.reset();
		gui->root_stack.push_back(&gui->root_info);

		while (it != end) {

			GuiHeader header = gui_read<GuiHeader>(it);

			switch (header) {

			case GuiHeader_BeginWindow:
			{
				SV_ASSERT(gui->root_stack.back() == &gui->root_info);

				u64 hash = gui_read<u64>(it);
				bool show = gui_read<bool>(it);

				if (show) {
		    
					GuiWindowState* state = gui->window_states.find(hash);
					SV_ASSERT(state);

					if (state) {

						GuiWindow* win = find_window_from_state(state);

						if (win) {
		    
							gui->root_stack.push_back(&win->root);

							if (win->current_index < win->states.size() && win->states[win->current_index] == state)
								gui->sorted_windows.push_back(win);
						}
					}
				}
			}
			break;

			case GuiHeader_EndWindow:
			{
				SV_ASSERT(gui->root_stack.back()->type == GuiRootType_Window);

				update_root_bounds(*gui->root_stack.back());

				// Set new root
				gui->root_stack.pop_back();
			}
			break;

			case GuiHeader_BeginPopup:
			{
				u64 id = gui_read<u64>(it);
				v2_f32 origin = gui_read<v2_f32>(it);

				if (id != gui->popup.id) {
					gui->popup.origin = origin;
					gui->popup.id = id;
				}
		
				gui->root_stack.push_back(&gui->popup.root);
			}
			break;

			case GuiHeader_EndPopup:
			{
				SV_ASSERT(gui->root_stack.back()->type == GuiRootType_Popup);

				f32 scale = gui->style.scale;
		
				// Compute bounds
				{
					v2_f32 size;
					size.x = 200.f / gui->resolution.x * scale;
					size.y = (gui->popup.root.yoff + 7.f) / gui->resolution.y * scale;

					v2_f32 pos = gui->popup.origin;
					// TODO: Check if the popup is outside the screen
					pos += v2_f32(size.x, -size.y) * 0.5f;

					gui->popup.root.widget_bounds = { pos.x, pos.y, size.x, size.y };
				}
		
				update_root_bounds(*gui->root_stack.back());

				// Set new root
				gui->root_stack.pop_back();
			}
			break;

			case GuiHeader_Widget:
			{
				read_widget(it);
			}
			break;

			case GuiHeader_Separator:
			{
				f32 separation = gui_read<f32>(it);
				gui->root_stack.back()->yoff += separation;
			}
			break;

			case GuiHeader_BeginGrid:
			{
				GuiRootInfo& root = *gui->root_stack.back();
				GuiReadWidgetStateData& data = root.read_widget_state_stack.emplace_back();
				data.state = GuiReadWidgetState_Grid;
				data.width = gui_read<f32>(it);
				data.padding = gui_read<f32>(it);
				data.xoff = 0.f;
			}
			break;

			case GuiHeader_EndGrid:
			{
				GuiRootInfo& root = *gui->root_stack.back();
				auto& stack = root.read_widget_state_stack;
		
				if (stack.size() && stack.back().state == GuiReadWidgetState_Grid) {

					auto& data = stack.back();
		    
					if (data.xoff != 0.f) {

						root.yoff += data.width + data.padding;
					}
		    
					stack.pop_back();
				}
				else SV_ASSERT(0);
			}
			break;
		
			}
		}

		update_root_bounds(gui->root_info);

		// Sort roots list
		std::sort(gui->sorted_windows.data(), gui->sorted_windows.data() + gui->sorted_windows.size(), [] (GuiWindow* w0, GuiWindow* w1) {
				return w0->priority > w1->priority;
			});

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

		// In update focus the windows are docked onto others and if one window is empty the app should crash
		u32 i = 0u;
		while (i != gui->sorted_windows.size()) {

			GuiWindow* w = gui->sorted_windows[i];
			if (w->states.empty()) {

				gui->sorted_windows.erase(i);
			}
			else ++i;
		}

		// Update popup
		if (gui->popup.id != 0u)
			update_root(gui->popup.root);

		// Update windows
		for (GuiWindow* w : gui->sorted_windows) {

			update_root(w->root);
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

    SV_AUX void gui_write_raw(const void* ptr, size_t size)
    {
		gui->buffer.write_back(ptr, size);
    }

    SV_AUX GuiWidget* find_widget(GuiWidgetType type, u64 id, GuiRootInfo* root = nullptr)
    {
		// TODO: Optimize

		if (root == nullptr)
			root = gui->root_stack.back();

		for (GuiWidget& w : root->widgets) {

			if (w.type == type && w.id == id)
				return &w;
		}

		return nullptr;
    }

    bool gui_begin_window(const char* title, u32 flags)
    {
		u64 hash = hash_string(title);

		GuiWindowState* state = gui->window_states.find(hash);

		if (state == nullptr) {

			size_t title_size = strlen(title);
			if (title_size > GUI_WINDOW_NAME_SIZE || title_size == 0u) {
				SV_LOG_ERROR("Invalid window name '%s'", title);
				return false;
			}

			GuiWindowState& s = gui->window_states[hash];
			strcpy(s.title, title);
			s.hash = hash;
			s.show = true;
			state = &s;
		}

		state->flags = flags;

		if (state->flags & GuiWindowFlag_NoClose)
			state->show = true;

		bool show = state->show;
	
		if (state->show) {

			gui_write(GuiHeader_BeginWindow);
			gui_write(hash);

			GuiWindow* win = find_window_from_state(state);

			if (win == nullptr) {

				u32 index = gui->window_count++;
				win = &gui->windows[index];
				win->states.push_back(state);
				win->current_index = 0u;
				win->root.type = GuiRootType_Window;
				win->root.window = win;
			}

			if (win->current_index >= win->states.size() || win->states[win->current_index] != state) {
				show = false;
			}
			else {
				gui->root_stack.push_back(&win->root);
				gui_push_id(state->hash);
			}

			gui_write(show);
		}

		return show;
    }

    void gui_end_window()
    {
		gui_write(GuiHeader_EndWindow);
		gui->root_stack.pop_back();
		gui_pop_id();
    }

    bool gui_begin_popup(GuiPopupTrigger trigger)
    {
		// Compute id
		u64 id = gui->current_id;
	
		{	    
			switch (trigger) {

			case GuiPopupTrigger_Root:
				hash_combine(id, 0x38B0F3);
				break;

			case GuiPopupTrigger_LastWidget:
			{
				if (gui->last_widget.type != GuiWidgetType_None) {
					hash_combine(id, gui->last_widget.id);
				}
				else return false;
			}
			break;
		
			}
		}

		bool show = false;
	
		// Check if should show the popup

		if (gui->popup.id == id && !gui->popup_request) {

			// Is showing right now
			show = true;
		}
		else if (input.mouse_buttons[MouseButton_Right] == InputState_Pressed) {

			switch (trigger) {

			case GuiPopupTrigger_Root:
			{
				GuiRootInfo* root = gui->root_stack.back();
		
				if (mouse_in_bounds(root->widget_bounds)) {

					show = true;

					for (GuiWidget& w : root->widgets) {

						if (mouse_in_bounds(w.bounds)) {
							show = false;
							break;
						}
					}
				}
			}
			break;

			case GuiPopupTrigger_LastWidget:
			{    
				GuiWidget* widget = find_widget(gui->last_widget.type, gui->last_widget.id, gui->last_widget.root);
		
				if (mouse_in_bounds(widget->bounds)) {
					show = true;
				}	
			}
			break;
		
			}
		}

		if (show) {

			gui_write(GuiHeader_BeginPopup);
			gui_write(id);
			gui_write(gui->mouse_position); // origin

			gui->root_stack.push_back(&gui->popup.root);
	    
			gui_push_id(id);

			gui->popup_request = true;
		}

		return show;
    }
    
    void gui_end_popup()
    {
		gui_write(GuiHeader_EndPopup);
		gui->root_stack.pop_back();
		gui_pop_id();
    }

    bool gui_show_window(const char* title)
    {
		u64 hash = hash_string(title);
		GuiWindowState* state = gui->window_states.find(hash);

		if (state == nullptr) return false;

		state->show = true;
		return true;
    }

    bool gui_hide_window(const char* title)
    {
		u64 hash = hash_string(title);
		GuiWindowState* state = gui->window_states.find(hash);

		if (state == nullptr) return false;

		state->show = false;
		return true;
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

    SV_AUX void write_widget(GuiWidgetType type, u64 id, u32 flags)
    {
		gui_write(GuiHeader_Widget);
		gui_write(type);
		gui_write(id);
		gui_write(flags);

		gui->last_widget.type = type;
		gui->last_widget.id = id;
		gui->last_widget.root = gui->root_stack.back();
    }

    bool gui_button(const char* text, u64 id)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_Button, id, 0u);
		gui_write_text(text);
	
		GuiWidget* button = find_widget(GuiWidgetType_Button, id);

		bool pressed = false;

		if (button) {

			pressed = button->widget.button.pressed;
		}
	
		return pressed;
    }

	bool gui_image_button(const char* text, GPUImage* image, v4_f32 texcoord, u64 id)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_ImageButton, id, 0u);
		gui_write_text(text);
		gui_write(image);
		gui_write(texcoord);
	
		GuiWidget* button = find_widget(GuiWidgetType_ImageButton, id);

		bool pressed = false;

		if (button) {

			pressed = button->widget.image_button.pressed;
		}
	
		return pressed;
    }

    bool gui_checkbox(const char* text, bool& value, u64 id)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_Checkbox, id, 0u);
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
	
		write_widget(GuiWidgetType_Checkbox, id, 0u);
		gui_write_text(text);
	
		GuiWidget* checkbox = find_widget(GuiWidgetType_Checkbox, id);

		bool value = checkbox && checkbox->widget.checkbox.value;
	
		gui_write(value);
	
		return value;
    }

    SV_AUX bool gui_drag(const char* text, void* value, void* adv, void* min, void* max, GuiType type, u64 id, u32 flags)
    {
		size_t size = sizeof_type(type);
		size_t comp_size = size / vectorof_type(type);

		compute_id(id);

		write_widget(GuiWidgetType_Drag, id, flags);

		if (text) {
			gui_write(true);
			gui_write_text(text);
		}
		else gui_write(false);
	
		gui_write(type);
		gui_write_raw(adv, comp_size);
		gui_write_raw(min, comp_size);
		gui_write_raw(max, comp_size);
	
		GuiWidget* drag = find_widget(GuiWidgetType_Drag, id);

		bool pressed = false;

		if (drag && gui->current_focus == drag) {

			pressed = true;
		}

		if (pressed) {
			memcpy(value, drag->widget.drag.value_data, size);
		}

		gui_write_raw(value, size);
		gui_write(drag ? drag->widget.drag.current_vector : 0u);
	
		return pressed;
    }

    bool gui_drag_f32(const char* text, f32& value, f32 adv, f32 min, f32 max, u64 id, u32 flags)
    {
		return gui_drag(text, &value, &adv, &min, &max, GuiType_f32, id, flags);
    }
    bool gui_drag_v2_f32(const char* text, v2_f32& value, f32 adv, f32 min, f32 max, u64 id, u32 flags)
    {
		return gui_drag(text, &value, &adv, &min, &max, GuiType_v2_f32, id, flags);
    }
    bool gui_drag_v3_f32(const char* text, v3_f32& value, f32 adv, f32 min, f32 max, u64 id, u32 flags)
    {
		return gui_drag(text, &value, &adv, &min, &max, GuiType_v3_f32, id, flags);
    }
    bool gui_drag_v4_f32(const char* text, v4_f32& value, f32 adv, f32 min, f32 max, u64 id, u32 flags)
    {
		return gui_drag(text, &value, &adv, &min, &max, GuiType_v4_f32, id, flags);
    }
    bool gui_drag_u32(const char* text, u32& value, u32 adv, u32 min, u32 max, u64 id, u32 flags)
    {
		return gui_drag(text, &value, &adv, &min, &max, GuiType_u32, id, flags);
    }

    void gui_text(const char* text, u64 id)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_Text, id, 0u);
		gui_write_text(text);
    }

	bool gui_text_field(char* buff, size_t buff_size, u64 id, u32 flags)
	{
		compute_id(id);
	
		write_widget(GuiWidgetType_TextField, id, flags);
		gui_write_text(buff);
		gui_write(buff_size);

		bool modified = false;

		if (gui->focus.type == GuiWidgetType_TextField && gui->focus.id == id) {

			if (gui->text_field_buffer.size() == buff_size) {

				string_copy(buff, (char*)gui->text_field_buffer.data(), buff_size);
				modified = true;
			}
		}

		return modified;
	}

    bool gui_collapse(const char* text, u64 id)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_Collapse, id, 0u);
		gui_write_text(text);
	
		GuiWidget* collapse = find_widget(GuiWidgetType_Collapse, id);

		bool active = collapse && collapse->widget.collapse.active;

		gui_write(active);
	
		return active;
    }

    void gui_image(GPUImage* image, f32 height, v4_f32 texcoord, u64 id, u32 flags)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_Image, id, flags);
		gui_write(height);
		gui_write(image);
		gui_write(texcoord);
    }

    void gui_separator(f32 separation)
    {
		gui_write(GuiHeader_Separator);
		gui_write(separation);
    }

    void gui_begin_grid(f32 width, f32 padding, u64 id)
    {
		gui_push_id(id);
		gui_write(GuiHeader_BeginGrid);
		gui_write(width);
		gui_write(padding);
    }
    
    void gui_end_grid()
    {
		gui_pop_id();
		gui_write(GuiHeader_EndGrid);
    }

    bool gui_select_filepath(const char* filepath, char* out, u64 id, u32 flags)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_SelectFilepath, id, flags);
		gui_write_text(filepath);

		//GuiWidget* asset = find_widget(GuiWidgetType_AssetButton, id);
		//bool pressed = asset ? asset->widget.asset_button.pressed : false;
		//return pressed;

		return false;
    }

    bool gui_asset_button(const char* text, GPUImage* image, u64 id, u32 flags)
    {
		compute_id(id);
	
		write_widget(GuiWidgetType_AssetButton, id, flags);
		gui_write_text(text);
		gui_write(image);

		GuiWidget* asset = find_widget(GuiWidgetType_AssetButton, id);

		bool pressed = asset ? asset->widget.asset_button.pressed : false;

		return pressed;
    }

    SV_AUX void draw_root(const GuiRootInfo& root, CommandList cmd)
    {
		const GuiStyle& style = gui->style;
	
		// Draw root
		switch (root.type) {

		case GuiRootType_Window:
		{
			const GuiWindow* window = root.window;

			const v4_f32& b = window->bounds;
			v4_f32 decoration = compute_window_decoration(root);
			v4_f32 closebutton = compute_window_closebutton(decoration);
	    
			GuiWindow* last_win = gui->sorted_windows.size() ? gui->sorted_windows.front() : nullptr;

			Color background_color = (last_win == window) ? style.root_focused_background_color : style.root_background_color;
			Color decoration_color = (last_win == window) ? style.window_focused_decoration_color : style.window_decoration_color;
			Color closebutton_color = (last_win == window) ? Color::Red() : Color::Gray(200u);
	    
			imrend_draw_quad({ b.x, b.y, 0.f }, { b.z, b.w }, background_color, cmd);
			imrend_draw_quad({ decoration.x, decoration.y, 0.f }, { decoration.z, decoration.w }, decoration_color, cmd);
			// TODO: !(window->current_state->flags & GuiWindowFlag_NoClose)) 
			imrend_draw_quad({ closebutton.x, closebutton.y, 0.f }, { closebutton.z, closebutton.w }, closebutton_color, cmd);

			if (window->states.size() > 1u) {

				v4_f32 window_buttons = compute_window_buttons(root);
				imrend_draw_quad({ window_buttons.x, window_buttons.y, 0.f }, { window_buttons.z, window_buttons.w }, Color::Red(), cmd);

				foreach (i, window->states.size()) {

					v4_f32 button = compute_window_button(root, window_buttons, i);
					imrend_draw_quad({ button.x, button.y, 0.f }, { button.z, button.w }, Color::Blue(), cmd);
				}
			}
		}
		break;

		case GuiRootType_Popup:
		{
			const v4_f32& b = root.widget_bounds;
			Color background_color = style.root_focused_background_color;;
			imrend_draw_quad({ b.x, b.y, 0.f }, { b.z, b.w }, background_color, cmd);
		}
		break;
	
		}

		// Draw scroll
		if (root_has_scroll(root)) {

			f32 width = GUI_SCROLL_SIZE / gui->resolution.x;

			Color color = Color::Green();
			Color button_color = Color::Blue();

			f32 x = root.widget_bounds.x + root.widget_bounds.z * 0.5f - width * 0.5f;

			f32 min = root.widget_bounds.y - root.widget_bounds.w * 0.5f;
			f32 max = root.widget_bounds.y + root.widget_bounds.w * 0.5f;

			f32 norm_value = 1.f - root.vertical_offset / ((root.yoff / gui->resolution.y) - root.widget_bounds.w);
	    
			f32 height = max - min;
			f32 button_height = 0.2f * height;
	    
			f32 y = (min + button_height * 0.5f) + norm_value * (height - button_height);
	    
			imrend_draw_quad({ x, root.widget_bounds.y, 0.f }, { width, root.widget_bounds.w }, color, cmd);
			imrend_draw_quad({ x, y, 0.f }, { width, button_height }, button_color, cmd);
		}

		// Push widget scissor
		{
			const v4_f32& b = root.widget_bounds;
			imrend_push_scissor(b.x, b.y, b.z, b.w, false, cmd);
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

				Color color = style.widget_primary_color;

				if (gui->current_focus == &w)
					color = style.widget_focused_color;
				else if (mouse_in_bounds(w.bounds))
					color = style.widget_highlighted_color;
		    
				imrend_draw_quad(pos.getVec3(), size, color, cmd);

				if (button.text) {

					Font& font = renderer_default_font();
			
					f32 font_size = size.y;

					imrend_draw_text(button.text, strlen(button.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, style.widget_text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_ImageButton:
			{
				auto& button = w.widget.image_button;

				const v4_f32& b = w.bounds;

				v2_f32 pos = { b.x, b.y };
				v2_f32 size = { b.z, b.w };

				Color color = style.widget_primary_color;

				if (gui->current_focus == &w)
					color = style.widget_focused_color;
				else if (mouse_in_bounds(w.bounds))
					color = style.widget_highlighted_color;
		    
				imrend_draw_quad(pos.getVec3(), size, color, cmd);

				constexpr f32 RELATIVE_HEIGHT = 0.95f;

				v4_f32 image_bounds = w.bounds;
				image_bounds.w *= RELATIVE_HEIGHT;
				image_bounds.z = image_bounds.w / gui->aspect;
				image_bounds.x = (image_bounds.x - w.bounds.z * 0.5f) + ((1.f - RELATIVE_HEIGHT) * image_bounds.w) / gui->aspect + image_bounds.z * 0.5f;

				size = { image_bounds.z, image_bounds.w };
				pos = { image_bounds.x, image_bounds.y };

				imrend_draw_sprite(pos.getVec3(), size, Color::White(), button.image ? button.image : renderer_white_image(), GPUImageLayout_ShaderResource, button.texcoord, cmd);

				if (button.text) {

					size.y = w.bounds.w;
					size.x = w.bounds.z - image_bounds.z - ((1.f - RELATIVE_HEIGHT) * image_bounds.w) / gui->aspect;
					pos.x = image_bounds.x + image_bounds.z * 0.5f + size.x * 0.5f;
					pos.x += size.x * 0.5f * 0.03f;
					size.x *= 0.97f;
					size.y *= 0.6f;

					Font& font = renderer_default_font();
			
					f32 font_size = size.y;

					imrend_draw_text(button.text, strlen(button.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, style.widget_text_color, cmd);
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

				imrend_draw_quad(pos.getVec3(0.f), size, style.widget_primary_color, cmd);

				// Check size
				v2_f32 s = size * 0.7f;
	    
				if (cb.value)
					imrend_draw_quad(pos.getVec3(0.f), s, style.check_color, cmd);
		
				size.x = w.bounds.z - size.x;
				pos.x = w.bounds.x + w.bounds.z * 0.5f - size.x * 0.5f;
	    
				if (cb.text) {

					size.x -= 0.01f; // Minus some margin
					f32 font_size = size.y;

					Font& font = renderer_default_font();

					imrend_draw_text(cb.text, strlen(cb.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, style.widget_text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_Drag:
			{
				v2_f32 pos;
				v2_f32 size;

				auto& drag = w.widget.drag;

				Font& font = renderer_default_font();
				v4_f32 bounds;
				u32 vector = vectorof_type(drag.type);
				char strbuff[100u];

				foreach(i, vector) {
		    
					bounds = compute_drag_slot(w, i);
					pos = v2_f32{ bounds.x, bounds.y };
					size = v2_f32{ bounds.z, bounds.w };

					Color color = style.widget_primary_color;

					if (w.flags & (GuiDragFlag_Position | GuiDragFlag_Scale | GuiDragFlag_Rotation)) {

						switch (i) {

						case 0:
							color = Color{ 229u, 25u, 25u, 255u };
							break;
						case 1:
							color = Color{ 51u, 204u, 51u, 255u };
							break;
						case 2:
							color = Color{ 13u, 25u, 229u, 255u };
							break;
						default:
							color = Color::Gray(150u);
						}
					}
		    
					imrend_draw_quad(pos.getVec3(0.f), size, color, cmd);

					f32 font_size = size.y;

					strcpy(strbuff, "\0");

					switch (drag.type) {

					case GuiType_f32:
					case GuiType_v2_f32:
					case GuiType_v3_f32:
					case GuiType_v4_f32:
					{
						f32 value = 0u;

						if (drag.type == GuiType_f32)
							value = *reinterpret_cast<const f32*>(drag.value_data);
						else if (drag.type == GuiType_v2_f32)
							value = (*reinterpret_cast<const v2_f32*>(drag.value_data))[i];
						else if (drag.type == GuiType_v3_f32)
							value = (*reinterpret_cast<const v3_f32*>(drag.value_data))[i];
						else if (drag.type == GuiType_v4_f32)
							value = (*reinterpret_cast<const v4_f32*>(drag.value_data))[i];
			
						sprintf(strbuff, "%f", value);
					}
					break;

					case GuiType_u32:
					{
						u32 value = 0u;

						if (drag.type == GuiType_u32)
							value = *reinterpret_cast<const u32*>(drag.value_data);
			
						sprintf(strbuff, "%u", value);
					}
					break;
		    
					}

					imrend_draw_text(strbuff, strlen(strbuff), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, style.widget_text_color, cmd);

					if (drag.text) {

						v4_f32 text_bounds = compute_drag_text(w);
						v2_f32 size = { text_bounds.z, text_bounds.w };
						v2_f32 pos = { text_bounds.x, text_bounds.y };

						size.x -= 0.01f; // Minus some margin
						f32 font_size = size.y;

						Font& font = renderer_default_font();

						imrend_draw_text(drag.text, strlen(drag.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, style.widget_text_color, cmd);
					}
				}
			}
			break;
			
			case GuiWidgetType_Text:
			{
				v2_f32 pos;
				v2_f32 size;

				auto& text = w.widget.text;
				pos = v2_f32{ w.bounds.x, w.bounds.y };
				size = v2_f32{ w.bounds.z, w.bounds.w };
	    
				if (text.text) {

					Font& font = renderer_default_font();
					f32 font_size = size.y + size.y * font.vertical_offset;

					imrend_draw_text(text.text, strlen(text.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, style.widget_text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_TextField:
			{
				v2_f32 pos;
				v2_f32 size;

				auto& text = w.widget.text_field;
				pos = v2_f32{ w.bounds.x, w.bounds.y };
				size = v2_f32{ w.bounds.z, w.bounds.w };

				Color color;

				if (gui->focus.type == GuiWidgetType_TextField && gui->focus.id == w.id)
					color = style.widget_highlighted_color;
				else
					color = style.widget_secondary_color;

				imrend_draw_quad(pos.getVec3(), size, color, cmd);
	    
				if (text.text) {

					Font& font = renderer_default_font();
					f32 font_size = size.y + size.y * font.vertical_offset;

					imrend_draw_text(text.text, strlen(text.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, style.widget_text_color, cmd);
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

				imrend_draw_quad(pos.getVec3(0.f), size, style.widget_primary_color, cmd);

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

					imrend_draw_text(collapse.text, strlen(collapse.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font.vertical_offset * font_size, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, style.widget_text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_Image:
			{
				auto& image = w.widget.image;

				v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y);
				v2_f32 size = v2_f32(w.bounds.z, w.bounds.w);
	    
				imrend_draw_sprite(pos.getVec3(), size, Color::White(), image.image ? image.image : renderer_white_image(), GPUImageLayout_ShaderResource, image.texcoord, cmd);
			}
			break;

			case GuiWidgetType_AssetButton:
			{
				auto& asset = w.widget.asset_button;

				v2_f32 pos = v2_f32(w.bounds.x, w.bounds.y);
				v2_f32 size = v2_f32(w.bounds.z, w.bounds.w);

				Color color = Color::White();

				if (mouse_in_bounds(w.bounds))
					color = Color::Gray(100);
	    
				imrend_draw_sprite(pos.getVec3(), size, color, asset.image ? asset.image : renderer_white_image(), GPUImageLayout_ShaderResource, { 0.f, 0.f, 1.f, 1.f }, cmd);

				if (asset.text) {

					pos.y -= size.y * 0.35f;
					size.y *= 0.35f;

					Font& font = renderer_default_font();
					f32 font_size = size.y + size.y * font.vertical_offset;

					imrend_draw_text(asset.text, strlen(asset.text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, size.x, 1u, font_size, gui->aspect, TextAlignment_Center, &font, style.widget_text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_SelectFilepath:
			{
				v2_f32 pos;
				v2_f32 size;

				auto& select = w.widget.select_filepath;
				pos = v2_f32{ w.bounds.x, w.bounds.y };
				size = v2_f32{ w.bounds.z, w.bounds.w };

				const char* text = select.filepath;

				if (text == nullptr)
					text = "";

				Font& font = renderer_default_font();
				f32 font_size = size.y + size.y * font.vertical_offset;

				imrend_draw_text(text, strlen(text), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, size.x, 1u, font_size, gui->aspect, TextAlignment_Left, &font, style.widget_text_color, cmd);
			}
			break;
		    
			}
		
		}

		imrend_pop_scissor(cmd);
    }
    
    void _gui_draw(CommandList cmd)
    {
		//const GuiStyle& style = gui->style;
		imrend_begin_batch(cmd);

		imrend_camera(ImRendCamera_Normal, cmd);

		// Draw screen
		draw_root(gui->root_info, cmd);

		// Draw windows
		for (i32 i = (i32)gui->sorted_windows.size() - 1; i >= 0; --i)
			draw_root(gui->sorted_windows[i]->root, cmd);

		// Draw popup
		if (gui->popup.id != 0u)
			draw_root(gui->popup.root, cmd);

		// Docking effects
		/*{
		  if (gui->focus.type == GuiWidgetType_Root && gui->focus.root->type == GuiRootType_Window && gui->focus.action == GuiWindowAction_Move) {

		  //GuiWindowState& window = *(GuiWindowState*)gui->focus.root->state;

		  constexpr Color BUTTON_COLOR = Color::LightSalmon(200u);
		  constexpr Color BUTTON_SELECTED_COLOR = Color::Red();
		  constexpr Color BACKGROUND_COLOR = Color::Gray(150u, 40u);

		  GuiRootInfo* docked_into;
		  GuiDockingLocation location;
		  compute_docking_location(location, docked_into);

		  v4_f32 section = compute_docking_section(location, docked_into);
		  imrend_draw_quad({ section.x, section.y, 0.f }, { section.z, section.w }, BACKGROUND_COLOR, cmd);

		  foreach(i, GuiDockingLocation_MaxEnum) {

		  v4_f32 b = compute_docking_button(GuiDockingLocation(i), docked_into, i == location);

		  Color color = (i == location) ? BUTTON_SELECTED_COLOR : BUTTON_COLOR;

		  imrend_draw_quad({ b.x, b.y, 0.f }, v2_f32{ b.z, b.w } * style.scale, color, cmd);
		  }
		  }
		  }*/

		imrend_flush(cmd);
    }
}
