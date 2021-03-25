#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"

#include "renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

namespace sv {

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
		GuiWidgetType_MaxEnum,
	};

	///////////////////////////////// RAW STRUCTS ///////////////////////////////////////

	struct Raw_Coords {
		GuiCoord x0;
		GuiCoord x1;
		GuiCoord y0;
		GuiCoord y1;
	};

	struct Raw_Button {
		const char* text;
		Raw_Coords coords;
		GuiButtonStyle style;
	};

	struct Raw_Drag {
		Raw_Coords coords;
		GuiDragStyle style;
		f32 value;
		f32 min;
		f32 max;
		f32 adv;
	};

	struct Raw_Checkbox {
		GuiCheckboxStyle style;
		Raw_Coords coords;
		bool value;
	};

	struct Raw_Label {
		GuiLabelStyle style;
		Raw_Coords coords;
		const char* text;
	};

	struct Raw_Container {
		Raw_Coords coords;
		GuiContainerStyle style;
	};

	struct Raw_Window {
		GuiWindowStyle style;
	};

	struct Raw_Popup {
		GuiPopupStyle style;
		v4_f32 bounds;
	};

	struct Raw_MenuItem {
		GuiMenuItemStyle style;
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
				f32 value;
				f32 min;
				f32 max;
				f32 adv;
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
		GuiParentUserData userdata;
	};

	struct GUI_internal {

		// WRITING DATA

		u8* buffer = nullptr;
		size_t buffer_size = 0u;
		size_t buffer_capacity = 0u;

		List<WritingParentInfo> parent_stack;

		GuiIndex current_focus;

		// STATE DATA

		List<GuiWidget> widgets;
		List<GuiIndex> indices;

		u32 root_menu_count;

		struct {
			GuiWidgetType type;
			u64 id;
			u32 action;
		} focus;

		struct {
			u64 id;
			GuiWidgetType type;
		}last;

		struct {

			std::unordered_map<u64, GuiWindowState> window;
			std::unordered_map<u64, bool> checkbox;

		} static_state;

		v2_f32 resolution;
		f32 aspect;
		v2_f32 mouse_position;
		List<u64> ids;
		u64 current_id;
		v2_f32 begin_position;
		u64 package_id;
		
		RawList package_data;
		
		// HIGH LEVEL

		struct {
			u32 element_count;
			u32 columns;
			f32 element_size;
			u32 element_index;
			f32 xoff;
			f32 padding;
		} grid;

	};

	Result gui_create(u64 hashcode, GUI** pgui)
	{
		GUI_internal& gui = *new GUI_internal();

		gui.buffer = (u8*)malloc(100u);
		gui.buffer_capacity = 100u;

		gui.package_id = 0u;

		// Get last static state
		{
			Archive archive;
			Result res = bin_read(hash_string("GUI STATE"), archive);
			if (result_okay(res)) {

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
				SV_LOG_WARNING("Can't load the last gui static state: %s", result_str(res));
			}
		}

		*pgui = (GUI*)& gui;

		return Result_Success;
	}

	Result gui_destroy(GUI* gui_)
	{
		PARSE_GUI();

		free(gui.buffer);

		// Save static state
		{
			Archive archive;

			archive << u32(gui.static_state.window.size());

			for (const auto& it : gui.static_state.window) {

				const GuiWindowState& s = it.second;
				archive << s.title << s.show << s.bounds;
			}

			Result res = bin_write(hash_string("GUI STATE"), archive);

			if (result_fail(res)) {

				SV_LOG_ERROR("Can't save the gui static state: %s", result_str(res));
			}
		}

		delete& gui;
		return Result_Success;
	}

	////////////////////////////////////////// UTILS ////////////////////////////////////////////////

	SV_INLINE static void write_buffer(GUI_internal& gui, const void* data, size_t size)
	{
		if (gui.buffer_size + size > gui.buffer_capacity) {

			size_t new_capacity = size_t(f64(gui.buffer_size + size) * 1.5);
			u8* new_buffer = nullptr;

			while (new_buffer == nullptr)
				new_buffer = (u8*)malloc(new_capacity);

			memcpy(new_buffer, gui.buffer, gui.buffer_size);
			free(gui.buffer);
			gui.buffer_capacity = new_capacity;
			gui.buffer = new_buffer;
		}

		memcpy(gui.buffer + gui.buffer_size, data, size);
		gui.buffer_size += size;
	}

	template<typename T>
	SV_INLINE static void write_buffer(GUI_internal& gui, const T& t)
	{
		write_buffer(gui, &t, sizeof(T));
	}

	SV_INLINE static void write_text(GUI_internal& gui, const char* text)
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

	SV_INLINE static void set_focus(GUI_internal& gui, GuiWidgetType type, u64 id, u32 action = 0u)
	{
		gui.focus.type = type;
		gui.focus.id = id;
		gui.focus.action = action;
	}

	SV_INLINE static void free_focus(GUI_internal& gui)
	{
		gui.focus.type = GuiWidgetType_None;
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

	SV_INLINE static GuiParentInfo* get_parent_info(GUI_internal& gui, GuiWidget& parent)
	{
		if (is_parent(parent)) {

			return reinterpret_cast<GuiParentInfo*>(&parent.widget);
		}
		else return nullptr;
	}
	SV_INLINE static const GuiParentInfo* get_parent_info(const GUI_internal& gui, const GuiWidget& parent)
	{
		if (is_parent(parent)) {

			return reinterpret_cast<const GuiParentInfo*>(&parent.widget);
		}
		else return nullptr;
	}

	SV_INLINE static GuiParentInfo* get_parent_info(GUI_internal& gui, const GuiIndex& index)
	{
		GuiWidget& w = gui.widgets[index.index];
		return get_parent_info(gui, w);
	}
	SV_INLINE static GuiParentInfo* get_parent_info(GUI_internal& gui, u32 index)
	{
		GuiWidget& w = gui.widgets[index];
		return get_parent_info(gui, w);
	}

	SV_INLINE static void write_widget(GUI_internal& gui, GuiWidgetType type, u64 id, void* data, GuiParentInfo* parent_info)
	{
		write_buffer(gui, type);
		write_buffer(gui, id);

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
			write_buffer(gui, raw->coords);
			write_buffer(gui, raw->style);
		}
		break;

		case GuiWidgetType_Slider:
			break;

		case GuiWidgetType_Label:
		{
			Raw_Label* raw = (Raw_Label*)data;

			write_text(gui, raw->text);
			write_buffer(gui, raw->coords);
			write_buffer(gui, raw->style);
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
			write_buffer(gui, raw->style);
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

		// Write parent raw data
		if (is_parent(type)) {
			if (parent_info) {

				write_buffer(gui, parent_info->vertical_offset);
				write_buffer(gui, parent_info->has_vertical_scroll);
			}
			else {
				write_buffer(gui, 0.f);
				write_buffer(gui, false);			
			}
		}

		gui.last.id = id;
		gui.last.type = type;
	}

	SV_INLINE static f32 compute_coord(const GUI_internal& gui, const GuiCoord& coord, f32 resolution, f32 parent_coord, f32 parent_dimension)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (coord.constraint)
		{
		case GuiConstraint_Relative:
			res = coord.value * parent_dimension;
			break;

		case GuiConstraint_Absolute:
		case GuiConstraint_InverseAbsolute:
			res = coord.value;
			break;

		case GuiConstraint_Center:
			res = 0.5f * parent_dimension;
			break;

		case GuiConstraint_Pixel:
		case GuiConstraint_InversePixel:
			res = coord.value / resolution;
			break;

		}

		// Inverse coord
		if (coord.constraint == GuiConstraint_InverseAbsolute || coord.constraint == GuiConstraint_InversePixel) {
			res = parent_dimension - res;
		}

		// Parent offset
		res += (parent_coord - parent_dimension * 0.5f);

		return res;
	}

	SV_INLINE bool mouse_in_bounds(const GUI_internal& gui, const v4_f32& bounds)
	{
		return abs(bounds.x - gui.mouse_position.x) <= bounds.z * 0.5f && abs(bounds.y - gui.mouse_position.y) <= bounds.w * 0.5f;
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, GuiWidget* parent)
	{
#ifdef SV_DEV
		{
			u32 aspect_count = 0u;
			if (x0.constraint == GuiConstraint_Aspect) ++aspect_count;
			if (x1.constraint == GuiConstraint_Aspect) ++aspect_count;
			if (y0.constraint == GuiConstraint_Aspect) ++aspect_count;
			if (y1.constraint == GuiConstraint_Aspect) ++aspect_count;

			SV_ASSERT(aspect_count <= 1u);
		}
#endif

		f32 _x0, _x1, _y0, _y1;

		v4_f32 parent_bounds;
		if (parent) {

			const GuiParentInfo* parent_info = get_parent_info(gui, *parent);
			parent_bounds = parent_info->child_bounds;
		}
		else parent_bounds = { 0.5f, 0.5f, 1.f, 1.f };

		if (x0.constraint == GuiConstraint_Aspect) {

			_x1 = compute_coord(gui, x1, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_y0 = compute_coord(gui, y0, gui.resolution.y, parent_bounds.y, parent_bounds.w);
			_y1 = compute_coord(gui, y1, gui.resolution.y, parent_bounds.y, parent_bounds.w);

			if (_y0 > _y1) std::swap(_y0, _y1);

			_x0 = _x1 - (_y1 - _y0) * x0.value * 1.f / gui.aspect;
		}
		else if (x1.constraint == GuiConstraint_Aspect) {

			_x0 = compute_coord(gui, x0, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_y0 = compute_coord(gui, y0, gui.resolution.y, parent_bounds.y, parent_bounds.w);
			_y1 = compute_coord(gui, y1, gui.resolution.y, parent_bounds.y, parent_bounds.w);

			if (_y0 > _y1) std::swap(_y0, _y1);

			_x1 = _x0 + (_y1 - _y0) * x1.value * 1.f / gui.aspect;
		}
		else if (y0.constraint == GuiConstraint_Aspect) {

			_x0 = compute_coord(gui, x0, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_x1 = compute_coord(gui, x1, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_y1 = compute_coord(gui, y1, gui.resolution.y, parent_bounds.y, parent_bounds.w);

			if (_x0 > _x1) std::swap(_x0, _x1);

			_y0 = _y1 - (_x1 - _x0) * y0.value * gui.aspect;
		}
		else if (y1.constraint == GuiConstraint_Aspect) {

			_x0 = compute_coord(gui, x0, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_x1 = compute_coord(gui, x1, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_y0 = compute_coord(gui, y0, gui.resolution.y, parent_bounds.y, parent_bounds.w);

			if (_x0 > _x1) std::swap(_x0, _x1);

			_y1 = _y0 + (_x1 - _x0) * y1.value * gui.aspect;
		}
		else {
			_x0 = compute_coord(gui, x0, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_x1 = compute_coord(gui, x1, gui.resolution.x, parent_bounds.x, parent_bounds.z);
			_y0 = compute_coord(gui, y0, gui.resolution.y, parent_bounds.y, parent_bounds.w);
			_y1 = compute_coord(gui, y1, gui.resolution.y, parent_bounds.y, parent_bounds.w);

			if (_x0 > _x1) std::swap(_x0, _x1);
			if (_y0 > _y1) std::swap(_y0, _y1);
		}



		f32 w = _x1 - _x0;
		f32 h = _y1 - _y0;
		f32 x = _x0 + w * 0.5f;
		f32 y = _y0 + h * 0.5f;

		return { x, y, abs(w), abs(h) };
	}

	SV_INLINE static v4_f32 compute_window_decoration_bounds(const GUI_internal& gui, const GuiWidget& w)
	{
		auto& window = w.widget.window;

		v4_f32 bounds;
		bounds.x = w.bounds.x;
		bounds.y = w.bounds.y + w.bounds.w * 0.5f + window.style.decoration_height * 0.5f + window.style.outline_size * gui.aspect;
		bounds.z = w.bounds.z + window.style.outline_size * 2.f;
		bounds.w = window.style.decoration_height;
		return bounds;
	}

	SV_INLINE static v4_f32 compute_window_closebutton_bounds(const GUI_internal& gui, const GuiWidget& w, const v4_f32 decoration_bounds)
	{
		v4_f32 bounds;
		bounds.x = decoration_bounds.x + decoration_bounds.z * 0.5f - decoration_bounds.w * 0.5f / gui.aspect;
		bounds.y = decoration_bounds.y;
		bounds.z = 0.006f;
		bounds.w = 0.006f * gui.aspect;
		
		return bounds;
	}

	static void update_widget(GUI_internal& gui, const GuiIndex& index)
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
		if (input.unused && parent_info) {

			input.unused = !mouse_in_bounds(gui, w.bounds);

			if (!input.unused) {

				v4_f32 scrollable_bounds = parent_info->child_bounds;
				parent_info->child_bounds.y -= parent_info->vertical_offset;

				if (input.mouse_wheel != 0.f && mouse_in_bounds(gui, scrollable_bounds)) {
						
					parent_info->vertical_offset += input.mouse_wheel;
				}
			}
		}

		// Limit scroll offsets
		if (parent_info->has_vertical_scroll) {
			parent_info->vertical_offset = 0.f;
		}
		else {
			parent_info->vertical_offset = std::max(std::min(parent_info->vertical_offset, parent_info->y_max), parent_info->y_min);
		}
	}

	static void update_focus(GUI_internal& gui, const GuiIndex& index)
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

				drag.value += input.mouse_dragged.x * gui.resolution.x * drag.adv;
				drag.value = std::max(std::min(drag.value, drag.max), drag.min);
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

	void gui_begin(GUI* gui_, f32 width, f32 height)
	{
		PARSE_GUI();

		gui.resolution = { width, height };
		gui.aspect = width / height;

		gui.ids.reset();

		gui.mouse_position = input.mouse_position + 0.5f;

		gui.parent_stack.resize(1u);
		gui.parent_stack.back().userdata.xoff = 0.f;
		gui.parent_stack.back().userdata.yoff = 0.f;

		gui.last.id = 0u;
		gui.last.type = GuiWidgetType_None;

		// Reset buffer
		gui.buffer_size = 0u;
		// TODO: free memory if there is no using

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

	SV_INLINE static void read_widget(GUI_internal& gui, GuiWidgetType type, u64 id, u32& current_parent, u8*& it)
	{
		// Add widget
		GuiWidget& w = gui.widgets.emplace_back();
		w.type = type;
		w.id = id;
		w.index = gui.widgets.size() - 1u;
		gui.indices.emplace_back(w.index, id);

		// Set focus
		if (gui.focus.type == w.type && gui.focus.id == id) {
			gui.current_focus = { w.index, id };
		}

		// Get parent
		GuiWidget* parent = (current_parent == u32_max) ? nullptr : &gui.widgets[current_parent];

		// Read raw data
		switch (type)
		{
		case GuiWidgetType_MenuContainer:
			break;

		case GuiWidgetType_Container:
		{
			Raw_Container raw = _read<Raw_Container>(it);
			w.widget.container.style = raw.style;
			w.raw_coords = raw.coords;
		}
		break;

		case GuiWidgetType_Popup:
		{
			Raw_Popup raw = _read<Raw_Popup>(it);
			w.widget.popup.style = raw.style;
			w.widget.popup.close_request = false;
			w.bounds = raw.bounds;
		}
		break;

		case GuiWidgetType_Window:
		{
			Raw_Window raw = _read<Raw_Window>(it);
			w.widget.window.style = raw.style;

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

			w.raw_coords = _read<Raw_Coords>(it);
			button.style = _read<GuiButtonStyle>(it);
		}
		break;

		case GuiWidgetType_Label:
		{
			auto& label = w.widget.label;

			label.text = (const char*)it;
			it += strlen(label.text) + 1u;
			if (*label.text == '\0')
				label.text = nullptr;

			w.raw_coords = _read<Raw_Coords>(it);
			label.style = _read<GuiLabelStyle>(it);
		}
		break;

		case GuiWidgetType_Slider:
			break;

		case GuiWidgetType_Checkbox:
		{
			auto& cb = w.widget.checkbox;
			
			Raw_Checkbox raw = _read<Raw_Checkbox>(it);
			cb.value = raw.value;
			cb.style = raw.style;
			w.raw_coords = raw.coords;
		}
		break;

		case GuiWidgetType_Drag:
		{
			auto& drag = w.widget.drag;

			Raw_Drag raw = _read<Raw_Drag>(it);
			drag.value = raw.value;
			drag.min = raw.min;
			drag.max = raw.max;
			drag.adv = raw.adv;
			drag.style = raw.style;
			w.raw_coords = raw.coords;
		}
		break;

		case GuiWidgetType_MenuItem:
		{
			auto& menu = w.widget.menu_item;

			menu.text = (const char*)it;
			it += strlen(menu.text) + 1u;
			if (*menu.text == '\0')
				menu.text = nullptr;

			menu.style = _read<GuiMenuItemStyle>(it);
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

		if (is_parent(w)) {

			GuiParentInfo* parent_info = get_parent_info(gui, w);
			parent_info->parent_index = current_parent;
			current_parent = w.index;
			parent_info->widget_offset = u32(gui.indices.size());
			parent_info->menu_item_count = 0u;
			parent_info->y_min = 0.f;
			parent_info->y_max = 0.f;
			
			parent_info->vertical_offset = _read<f32>(it);
			parent_info->has_vertical_scroll = _read<bool>(it);
		}
	}

	// TEMP
	constexpr f32 MENU_HEIGHT = 0.02f;
	constexpr f32 MENU_WIDTH = 0.04f;

	constexpr f32 MENU_CONTAINER_HEIGHT = 0.1f;
	constexpr f32 MENU_CONTAINER_WIDTH = 0.1f;

	constexpr f32 SCROLL_SIZE = 0.02f;
	constexpr f32 SCROLL_BUTTON_SIZE_MULT = 0.2f;
	
	static void update_bounds(GUI_internal& gui, GuiWidget* w, GuiWidget* parent, u32& menu_index)
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
			w->bounds = compute_widget_bounds(gui, w->raw_coords.x0, w->raw_coords.x1, w->raw_coords.y0, w->raw_coords.y1, parent);
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
				parent_info->child_bounds.y += parent_info->vertical_offset;
			}

			// Update childs
			GuiIndex* it = gui.indices.data() + parent_info->widget_offset;
			GuiIndex* end = it + parent_info->widget_count;

			u32 _menu_index = 0u;

			while (it != end) {

				GuiWidget* widget = &gui.widgets[it->index];
				update_bounds(gui, widget, w, _menu_index);

				parent_info->y_min = std::min(widget->bounds.y - parent_info->vertical_offset - widget->bounds.w * 0.5f, parent_info->y_min);
				parent_info->y_max = std::max(widget->bounds.y - parent_info->vertical_offset + widget->bounds.w * 0.5f, parent_info->y_max);

				GuiParentInfo* p = get_parent_info(gui, *widget);
				if (p) {

					it += p->widget_count;
				}

				++it;
			}

			// Enable - Disable scroll bars (this bool is saved in raw data and used in the next frame
			f32 height = parent_info->y_max - parent_info->y_min;
			parent_info->has_vertical_scroll = height > parent_info->child_bounds.w;
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
		SV_ASSERT(gui.parent_stack.size() == 1u);

		// Read widgets from raw data
		{
			u8* it = gui.buffer;
			u8* end = gui.buffer + gui.buffer_size;

			u32 current_parent = u32_max;

			while (it != end) {

				GuiWidgetType type = _read<GuiWidgetType>(it);
				u64 id;

				if (type != GuiWidgetType_None)
					id = _read<u64>(it);

				if (type == GuiWidgetType_None) {

					SV_ASSERT(current_parent != u32_max);

					GuiParentInfo* parent_info = get_parent_info(gui, current_parent);
					SV_ASSERT(parent_info);
					parent_info->widget_count = u32(gui.indices.size()) - parent_info->widget_offset;
					current_parent = parent_info->parent_index;
				}
				else if (type < GuiWidgetType_MaxEnum) {

					read_widget(gui, type, id, current_parent, it);
				}
			}
		}

		// Compute widget bounds
		{
			GuiWidget* current_parent = nullptr;

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
	}

	SV_INLINE static void update_id(GUI_internal& gui)
	{
		gui.current_id = 0U;
		for (u64 id : gui.ids)
			hash_combine(gui.current_id, id);
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

	///////////////////////////////////////////// WIDGETS ///////////////////////////////////////////

	SV_INLINE static GuiWidget* find_widget(GUI_internal& gui, u64 id, GuiWidgetType type)
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

	SV_INLINE void begin_parent(GUI_internal& gui, GuiWidget* parent, u64 id)
	{
		WritingParentInfo& info = gui.parent_stack.emplace_back();
		info.userdata.xoff = 0.f;
		info.userdata.yoff = 0.f;

		if (parent) {
			SV_ASSERT(is_parent(parent->type));
			info.parent = parent;
		}
		gui_push_id((GUI*)& gui, id);
	}

	SV_INLINE void end_parent(GUI_internal& gui)
	{
		gui_pop_id((GUI*)& gui);
		gui.parent_stack.pop_back();
	}

	void gui_begin_container(GUI* gui_, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiContainerStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);

		{
			Raw_Container raw;
			raw.coords.x0 = x0;
			raw.coords.x1 = x1;
			raw.coords.y0 = y0;
			raw.coords.y1 = y1;
			raw.style = style;

			write_widget(gui, GuiWidgetType_Container, id, &raw);
		}

		GuiWidget* w = find_widget(gui, id, GuiWidgetType_Container);
		begin_parent(gui, w, id);
	}

	void gui_end_container(GUI* gui_)
	{
		PARSE_GUI();
		write_buffer(gui, GuiWidgetType_None);

		end_parent(gui);
	}

	bool gui_begin_popup(GUI* gui_, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, const GuiPopupStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);

		GuiWidget* w = find_widget(gui, id, GuiWidgetType_Popup);

		if (w) {

			auto& popup = w->widget.popup;

			if (popup.close_request)
				return false;

			Raw_Popup raw;
			raw.style = style;
			raw.bounds = w->bounds;

			write_widget(gui, GuiWidgetType_Popup, w->id, &raw);
			begin_parent(gui, w, id);
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
				raw.style = style;
				raw.bounds.z = 0.1f;
				raw.bounds.w = 0.2f;
				raw.bounds.x = gui.mouse_position.x + raw.bounds.z * 0.5f;
				raw.bounds.y = gui.mouse_position.y - raw.bounds.w * 0.5f;

				write_widget(gui, GuiWidgetType_Popup, id, &raw);
				begin_parent(gui, w, id);
				return true;
			}
		}

		return false;
	}

	void gui_end_popup(GUI* gui_)
	{
		PARSE_GUI();
		write_buffer(gui, GuiWidgetType_None);

		end_parent(gui);
	}

	/////////////////////////////////////// WINDOW ////////////////////////////////////////////////

	bool gui_begin_window(GUI* gui_, const char* title, const GuiWindowStyle& style)
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
			raw.style = style;

			write_widget(gui, GuiWidgetType_Window, id, &raw);

			GuiWidget* w = find_widget(gui, id, GuiWidgetType_Window);
			begin_parent(gui, w, id);
		}

		return state->show;
	}

	void gui_end_window(GUI* gui_)
	{
		PARSE_GUI();
		write_buffer(gui, GuiWidgetType_None);

		end_parent(gui);
	}

	SV_INLINE static GuiWindowState* get_window_state(GUI_internal& gui, u64 id)
	{
		auto it = gui.static_state.window.find(id);
		if (it == gui.static_state.window.end()) return nullptr;
		return &it->second;
	}

	Result gui_show_window(GUI* gui_, const char* title)
	{
		PARSE_GUI();
		u64 id = hash_string(title);
		GuiWindowState* state = get_window_state(gui, id);

		if (state) {

			state->show = true;
			return Result_Success;
		}

		return Result_NotFound;
	}

	Result gui_hide_window(GUI* gui_, const char* title)
	{
		PARSE_GUI();
		u64 id = hash_string(title);
		GuiWindowState* state = get_window_state(gui, id);

		if (state) {

			state->show = false;
			return Result_Success;
		}

		return Result_NotFound;
	}

	/////////////////////////////////////// MENU ITEM /////////////////////////////////////////

	bool gui_begin_menu_item(GUI* gui_, const char* text, u64 id, const GuiMenuItemStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);

		GuiWidget* w = find_widget(gui, id, GuiWidgetType_MenuItem);

		{
			Raw_MenuItem raw;
			raw.style = style;
			raw.text = text;
			raw.active = w ? w->widget.menu_item.active : false;

			write_widget(gui, GuiWidgetType_MenuItem, id, &raw);
		}

		if (w && w->widget.menu_item.active) {
			
			u64 container_id = id;
			hash_combine(container_id, 0x32d9f32ac);

			write_widget(gui, GuiWidgetType_MenuContainer, container_id, nullptr);

			GuiWidget* c = find_widget(gui, container_id, GuiWidgetType_MenuContainer);
			begin_parent(gui, c, container_id);

			return true;
		}

		return false;
	}

	void gui_end_menu_item(GUI* gui_)
	{
		PARSE_GUI();
		write_buffer(gui, GuiWidgetType_None);

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
		
		write_widget(gui, GuiWidgetType_Package, id, &raw);
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

				write_widget(gui, GuiWidgetType_Reciver, id, &raw);
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

	bool gui_button(GUI* gui_, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiButtonStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);

		{
			Raw_Button raw;
			raw.text = text;
			raw.coords.x0 = x0;
			raw.coords.x1 = x1;
			raw.coords.y0 = y0;
			raw.coords.y1 = y1;
			raw.style = style;

			write_widget(gui, GuiWidgetType_Button, id, &raw);
		}

		GuiWidget* w = find_widget(gui, id, GuiWidgetType_Button);

		if (w) {
			return w->widget.button.pressed;
		}
		else {
			return false;
		}
	}

	bool gui_drag_f32(GUI* gui_, f32* value, f32 adv, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style)
	{
		PARSE_GUI();
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
			raw.coords.x0 = x0;
			raw.coords.x1 = x1;
			raw.coords.y0 = y0;
			raw.coords.y1 = y1;
			raw.style = style;
			raw.value = *value;
			raw.min = min;
			raw.max = max;
			raw.adv = adv;

			write_widget(gui, GuiWidgetType_Drag, id, &raw);
		}

		return modified;
	}

	bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiSliderStyle& style)
	{
		return false;
	}

	void gui_text(GUI* gui_, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiLabelStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);
		
		Raw_Label raw;
		raw.text = text;
		raw.coords.x0 = x0;
		raw.coords.x1 = x1;
		raw.coords.y0 = y0;
		raw.coords.y1 = y1;
		raw.style = style;

		write_widget(gui, GuiWidgetType_Label, id, &raw);		
	}

	bool gui_checkbox(GUI* gui_, bool* value, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckboxStyle& style)
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
			raw.coords.x0 = x0;
			raw.coords.x1 = x1;
			raw.coords.y0 = y0;
			raw.coords.y1 = y1;
			raw.style = style;
			raw.value = *value;

			write_widget(gui, GuiWidgetType_Checkbox, id, &raw);
		}

		return modified;
	}

	bool gui_checkbox(GUI* gui_, u64 user_id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckboxStyle& style)
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
		
		gui_checkbox(gui_, value, user_id, x0, x1, y0, y1, style);
		return *value;
	}

	///////////////////////////////////////////// GETTERS ///////////////////////////////////////////

	v4_f32 gui_parent_bounds(GUI* gui_)
	{
		PARSE_GUI();
		return (gui.parent_stack.size() && gui.parent_stack.back().parent) ? gui.parent_stack.back().parent->bounds : v4_f32{};
	}

	GuiParentUserData& gui_parent_userdata(GUI* gui_)
	{
		PARSE_GUI();
		return gui.parent_stack.back().userdata;
	}

	///////////////////////////////////////////// RENDERING ///////////////////////////////////////////

	constexpr u32 MAX_FLOAT_CHARS = 20u;

	static void float_to_string(f32 n, char* str, u32* size, const u32 buffer_size)
	{
		bool negative = n < 0.f;

		constexpr f32 DECIMAL_MULT = 10000000.f;

		n = abs(n);
		u32 n0 = u32(n);
		u32 n1 = u32((n - i32(n)) * DECIMAL_MULT + DECIMAL_MULT);

		// Add integer values

		char* it = str;
		char* end = str + buffer_size;

		if (negative) {
			*it = '-';
			++it;
		}

		char* valid_ptr = it;

		if (n0 == 0u) {

			*it = '0';
			++it;
		}
		else {

			while (it != end) {

				if (n0 == 0u) {
					break;
				}

				u32 number = n0 % 10u;
				n0 /= 10u;

				char c = number + '0';
				*it = c;
				++it;

				if (n == 0u) {
					if (it == end) break;

					*it = '.';
					++it;
				}
			}
		}

		// Swap values
		char* it0 = it - 1U;

		while (valid_ptr < it0) {

			char aux = *it0;
			*it0 = *valid_ptr;
			*valid_ptr = aux;
			++valid_ptr;
			--it0;
		}

		// Add decimal values

		if (n1 != u32(DECIMAL_MULT) && it != end) {

			*it = '.';
			++it;

			valid_ptr = it;

			while (it != end) {

				if (n1 == 1u) {
					break;
				}

				u32 number = n1 % 10u;
				n1 /= 10u;

				char c = number + '0';
				*it = c;
				++it;
			}
		}

		// Swap values
		it0 = it - 1U;

		while (valid_ptr < it0) {

			char aux = *it0;
			*it0 = *valid_ptr;
			*valid_ptr = aux;
			++valid_ptr;
			--it0;
		}

		// Remove unnecesary 0
		--it;
		while (*it == '0' && it > (str + 1u)) --it;
		++it;

		*size = u32(it - str);
	}


	static void draw_widget(GUI_internal& gui, const GuiWidget& w, CommandList cmd)
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

			char strbuff[MAX_FLOAT_CHARS + 1u];
			u32 strsize;
			float_to_string(drag.value, strbuff, &strsize, MAX_FLOAT_CHARS);

			draw_text(strbuff, strsize
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

			draw_debug_quad(pos.getVec3(0.f), size, cb.style.color, cmd);

			const GuiBox* box;
			if (cb.value) box = &cb.style.active_box;
			else box = &cb.style.inactive_box;

			size *= box->mult;

			switch (box->type)
			{
			case GuiBoxType_Quad:
				draw_debug_quad(pos.getVec3(0.f), size * 0.7f, box->quad.color, cmd);
				break;

			case GuiBoxType_Triangle:
				if (box->triangle.down) {
					draw_debug_triangle(
						{ pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
						{ pos.x + size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
						{ pos.x, pos.y - size.y * 0.5f, 0.f }
						, box->triangle.color, cmd);
				}
				else {
					draw_debug_triangle(
						{ pos.x - size.x * 0.5f, pos.y + size.y * 0.5f, 0.f },
						{ pos.x - size.x * 0.5f, pos.y - size.y * 0.5f, 0.f },
						{ pos.x + size.x * 0.5f, pos.y, 0.f }
						, box->triangle.color, cmd);
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
			auto& container = w.widget.menu_container;

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
			draw_debug_quad(pos.getVec3(), size, popup.style.background_color, cmd);
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

					f32 norm = (parent_info->vertical_offset - parent_info->y_min) / (parent_info->y_max - parent_info->y_min);

					v4_f32 scroll_bounds = parent_info->child_bounds;
					scroll_bounds.y -= parent_info->vertical_offset;

					scroll_bounds.x += scroll_bounds.z * 0.5f - SCROLL_SIZE * 0.5f;
					scroll_bounds.z = SCROLL_SIZE;

					f32 button_height = scroll_bounds.w * SCROLL_BUTTON_SIZE_MULT;
					f32 button_space = scroll_bounds.w - button_height;
					f32 button_y = (scroll_bounds.y + scroll_bounds.w * 0.5f) + (button_height * 0.5f) + (norm * button_space);
					
					begin_debug_batch(cmd);
					
					draw_debug_quad((v2_f32(scroll_bounds.x, scroll_bounds.y) * 2.f - 1.f).getVec3(0.f), v2_f32(scroll_bounds.z, scroll_bounds.w) * 2.f, Color::Red(), cmd);
					draw_debug_quad((v2_f32(scroll_bounds.x, button_y) * 2.f - 1.f).getVec3(0.f), v2_f32(scroll_bounds.z, button_height) * 2.f, Color::Green(), cmd);
					
					
					end_debug_batch(true, false, XMMatrixIdentity(), cmd);

				}
				
				// Draw childs
				
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

	void gui_begin_grid(GUI* gui_, u32 element_count, f32 element_size, f32 padding, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1)
	{
		PARSE_GUI();
		auto& grid = gui.grid;

		v4_f32 parent_bounds = gui_parent_bounds(gui_);

		f32 width = parent_bounds.z * gui.resolution.x;

		grid.element_count = element_count;
		grid.element_index = 0u;
		grid.columns = std::max(u32((width + padding) / (element_size + padding)), 1u);
		grid.element_size = element_size;
		grid.xoff = std::max(width - grid.element_size * f32(grid.columns) - padding * f32(grid.columns - 1u), 0.f) * 0.5f;
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

		gui_begin_container(gui_, id, GuiCoord::Pixel(x0), GuiCoord::Pixel(x1), GuiCoord::IPixel(y0), GuiCoord::IPixel(y1));
	}

	void gui_end_grid_element(GUI* gui_)
	{
		PARSE_GUI();
		auto& grid = gui.grid;

		gui_end_container(gui_);
	}

	void gui_end_grid(GUI* gui_)
	{
		PARSE_GUI();
		auto& grid = gui.grid;
	}

}
