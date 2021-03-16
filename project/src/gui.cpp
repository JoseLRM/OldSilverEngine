#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/FrameList.h"

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
		GuiWidgetType_Drag
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

	///////////////////////////////// WIDGET STRUCTS ///////////////////////////////////

	struct GuiWidget {
		GuiWidgetType type;
		u32 index;
		u64 id;

		GuiWidget() = default;
		GuiWidget(GuiWidgetType type, u32 index, u64 id) : type(type), index(index), id(id) {}
	};

	struct GuiParent {

		u32 widget_offset;
		u32 widget_count;

		GuiWidget parent;

		v4_f32 bounds;

	};

	struct GuiContainer : public GuiParent {

		GuiContainerStyle style;

	};

	struct GuiWindowState {

		v4_f32 bounds;
		bool show;
		std::string title;
		u64 id;

	};

	struct GuiWindow : public GuiParent {

		GuiWindowStyle style;
		GuiWindowState* state;

	};

	struct GuiPopup : public GuiParent {

		GuiPopupStyle style;
		u64 id;
		bool close_request;

	};

	struct GuiButton {

		const char* text;
		GuiButtonStyle style;
		bool hot;
		bool pressed;
		v4_f32 bounds;

	};

	struct GuiDrag {

		f32 value;
		v4_f32 bounds;
		f32 adv;
		GuiDragStyle style;

	};

	struct GuiCheckbox {

		bool value;
		v4_f32 bounds;
		GuiCheckboxStyle style;
		
	};

	struct GuiLabel {

		const char* text;
		GuiLabelStyle style;
		v4_f32 bounds;
		
	};

	struct GUI_internal {

		u8* buffer = nullptr;
		size_t buffer_size = 0u;
		size_t buffer_capacity = 0u;

		GuiParent* current_parent;

		FrameList<GuiContainer> containers;
		FrameList<GuiPopup> popups;
		FrameList<GuiWindow> windows;
		FrameList<GuiButton> buttons;
		FrameList<GuiDrag> drags;
		FrameList<GuiLabel> labels;
		FrameList<GuiCheckbox> checkboxes;

		FrameList<GuiWidget> widgets;

		struct {
			GuiWidgetType type;
			u64 id;
			u32 action;
		} focus;

		struct {
			u64 id;
			GuiWidgetType type;
		} last;

		GuiWidget current_focus;

		struct {

			std::unordered_map<u64, GuiWindowState> window;
			std::unordered_map<u64, bool> checkbox;

		} static_state;

		v2_f32 resolution;
		f32 aspect;
		v2_f32 mouse_position;
		FrameList<u64> ids;
		u64 current_id;
		v2_f32 begin_position;

	};

	Result gui_create(u64 hashcode, GUI** pgui)
	{
		GUI_internal& gui = *new GUI_internal();

		gui.buffer = (u8*)malloc(100u);
		gui.buffer_capacity = 100u;

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
		case GuiWidgetType_Container:
		case GuiWidgetType_Window:
		case GuiWidgetType_Popup:
			return true;

		default:
			return false;
		}
	}

	SV_INLINE static GuiParent* get_parent(GUI_internal& gui, const GuiWidget& w)
	{
		switch (w.type)
		{
		case GuiWidgetType_Container:
			return &gui.containers[w.index];

		case GuiWidgetType_Popup:
			return &gui.popups[w.index];

		case GuiWidgetType_Window:
			return &gui.windows[w.index];

		default:
			return nullptr;
		}
	}

	SV_INLINE static v4_f32 get_widget_bounds(GUI_internal& gui, const GuiWidget& w)
	{
		switch (w.type)
		{

		case GuiWidgetType_Container:
			return gui.containers[w.index].bounds;

		case GuiWidgetType_Window:
			return gui.windows[w.index].bounds;

		case GuiWidgetType_Popup:
			return gui.popups[w.index].bounds;

		case GuiWidgetType_Button:
			return gui.buttons[w.index].bounds;

			//case GuiWidgetType_Slider:
			//	return gui.sli[w.index].bounds;

		case GuiWidgetType_Label:
			return gui.labels[w.index].bounds;
		
		case GuiWidgetType_Checkbox:
			return gui.checkboxes[w.index].bounds;

		case GuiWidgetType_Drag:
			return gui.drags[w.index].bounds;

		default:
			return { 0.f, 0.f, 0.f, 0.f };
		}
	}

	SV_INLINE static void write_widget(GUI_internal& gui, GuiWidgetType type, u64 id, void* data)
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

		default:
			break;
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

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, GuiParent* parent)
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
			parent_bounds = parent->bounds;
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

	SV_INLINE static v4_f32 compute_window_decoration_bounds(const GUI_internal& gui, const GuiWindow& window)
	{
		v4_f32 bounds;
		bounds.x = window.state->bounds.x;
		bounds.y = window.state->bounds.y + window.state->bounds.w * 0.5f + window.style.decoration_height * 0.5f + window.style.outline_size * gui.aspect;
		bounds.z = window.state->bounds.z + window.style.outline_size * 2.f;
		bounds.w = window.style.decoration_height;
		return bounds;
	}

	static void update_widget(GUI_internal& gui, const GuiWidget& w)
	{
		GuiParent* parent = get_parent(gui, w);

		// If it is a parent, update childs
		if (parent) {

			GuiWidget* it = gui.widgets.data() + parent->widget_offset;
			GuiWidget* end = it + parent->widget_count;

			// Update childs parents
			while (it != end) {

				GuiParent* p = get_parent(gui, *it);
				if (p) {

					update_widget(gui, *it);
					it += p->widget_count;
				}

				++it;
			}

			// Update normal widgets
			it = gui.widgets.data() + parent->widget_offset;

			while (it != end) {

				GuiParent* p = get_parent(gui, *it);
				if (p) {

					it += p->widget_count;
				}
				else {

					update_widget(gui, *it);
				}

				++it;
			}
		}

		switch (w.type)
		{

		case GuiWidgetType_Button:
		{
			GuiButton& button = gui.buttons[w.index];
			if (input.unused && mouse_in_bounds(gui, button.bounds)) {

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
		{
			if (input.unused) {
				GuiDrag& drag = gui.drags[w.index];
				if (mouse_in_bounds(gui, drag.bounds)) {

					if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

						set_focus(gui, GuiWidgetType_Drag, w.id);
						input.unused = false;
					}
				}
			}
		}
		break;

		case GuiWidgetType_Checkbox:
		{
			if (input.unused) {
				GuiCheckbox& cb = gui.checkboxes[w.index];
				if (mouse_in_bounds(gui, cb.bounds)) {

					if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

						set_focus(gui, GuiWidgetType_Checkbox, w.id);
						input.unused = false;
					}
				}
			}
		}
		break;

		case GuiWidgetType_Popup:
		{
			GuiPopup& popup = gui.popups[w.index];

			bool any = false;

			foreach(i, MouseButton_MaxEnum) {
				if (input.mouse_buttons[i] == InputState_Pressed) {
					any = true;
					break;
				}
			}

			if (any && !mouse_in_bounds(gui, popup.bounds)) {

				popup.close_request = true;
			}
		}
		break;

		case GuiWidgetType_Window:
		{
			GuiWindow& window = gui.windows[w.index];
			GuiWindowState& state = *window.state;

			if (input.unused) {

				InputState button = input.mouse_buttons[MouseButton_Left];

				v4_f32 decoration = compute_window_decoration_bounds(gui, window);

				if (mouse_in_bounds(gui, decoration)) {

					input.unused = false;

					if (button == InputState_Pressed) {
						gui.begin_position = v2_f32(window.state->bounds.x, window.state->bounds.y) - gui.mouse_position;
						set_focus(gui, GuiWidgetType_Window, state.id, 0u);
					}
				}
				else {
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


		// Catch the input if the mouse is inside the parent
		if (input.unused && parent) {

			input.unused = !mouse_in_bounds(gui, parent->bounds);
		}
	}

	static void update_focus(GUI_internal& gui, const GuiWidget& w)
	{
		switch (w.type)
		{

		case GuiWidgetType_Drag:
		{
			GuiDrag& drag = gui.drags[w.index];

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {
				free_focus(gui);
			}
			else {

				drag.value += input.mouse_dragged.x * gui.resolution.x * drag.adv;
				drag.value += input.mouse_dragged.x * gui.resolution.x * drag.adv;
			}
		}
		break;

		case GuiWidgetType_Checkbox:
		{
			GuiCheckbox& cb = gui.checkboxes[w.index];

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

				free_focus(gui);
				cb.value = !cb.value;
			}
		}
		break;

		case GuiWidgetType_Window:
		{
			GuiWindow& window = gui.windows[w.index];

			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

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
		gui.current_parent = nullptr;

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

	SV_INLINE static void add_widget(GUI_internal& gui, GuiWidgetType type, u32 index, u64 id)
	{
		gui.widgets.emplace_back(type, index, id);

		if (gui.focus.type == type && gui.focus.id == id) {
			gui.current_focus = { type, index, id };
		}
	}

	SV_INLINE static void add_widget_parent(GUI_internal& gui, GuiParent* parent, GuiWidget& current_parent, GuiWidgetType type, u32 index, u64 id)
	{
		add_widget(gui, type, index, id);

		parent->parent = current_parent;
		current_parent = { type, index, id };
		parent->widget_offset = u32(gui.widgets.size());
	}

	void gui_end(GUI* gui_)
	{
		PARSE_GUI();

		// Reset last data
		gui.containers.reset();
		gui.popups.reset();
		gui.windows.reset();
		gui.buttons.reset();
		gui.drags.reset();
		gui.labels.reset();
		gui.checkboxes.reset();
		gui.widgets.reset();

		gui.current_focus.type = GuiWidgetType_None;

		SV_ASSERT(gui.ids.empty());

		// Create widgets from raw data

		u8* it = gui.buffer;
		u8* end = gui.buffer + gui.buffer_size;

		GuiWidget current_parent;
		current_parent.type = GuiWidgetType_None;

		while (it != end) {

			GuiWidgetType type = _read<GuiWidgetType>(it);
			u64 id;

			if (type != GuiWidgetType_None)
				id = _read<u64>(it);

			switch (type)
			{

			case GuiWidgetType_None:
			{
				SV_ASSERT(current_parent.type != GuiWidgetType_None);

				GuiParent* parent = get_parent(gui, current_parent);
				SV_ASSERT(parent);
				parent->widget_count = u32(gui.widgets.size()) - parent->widget_offset;
				current_parent = parent->parent;
			}
			break;

			case GuiWidgetType_Container:
			{
				GuiContainer& container = gui.containers.emplace_back();

				Raw_Container raw = _read<Raw_Container>(it);
				container.style = raw.style;

				container.bounds = compute_widget_bounds(gui, raw.coords.x0, raw.coords.x1, raw.coords.y0, raw.coords.y1, get_parent(gui, current_parent));

				add_widget_parent(gui, &container, current_parent, GuiWidgetType_Container, gui.containers.size() - 1u, id);
			}
			break;

			case GuiWidgetType_Popup:
			{
				GuiPopup& popup = gui.popups.emplace_back();

				Raw_Popup raw = _read<Raw_Popup>(it);
				popup.style = raw.style;
				popup.bounds = raw.bounds;
				popup.id = id;
				popup.close_request = false;

				add_widget_parent(gui, &popup, current_parent, GuiWidgetType_Popup, gui.popups.size() - 1u, id);
			}
			break;

			case GuiWidgetType_Window:
			{
				GuiWindow& window = gui.windows.emplace_back();

				Raw_Window raw = _read<Raw_Window>(it);
				window.style = raw.style;

				GuiWindowState& state = gui.static_state.window[id];
				window.bounds = state.bounds;
				window.state = &state;

				add_widget_parent(gui, &window, current_parent, GuiWidgetType_Window, gui.windows.size() - 1u, id);
			}
			break;

			case GuiWidgetType_Button:
			{
				GuiButton& button = gui.buttons.emplace_back();

				button.text = (const char*)it;
				it += strlen(button.text) + 1u;
				if (*button.text == '\0')
					button.text = nullptr;

				Raw_Coords coords = _read<Raw_Coords>(it);
				button.style = _read<GuiButtonStyle>(it);

				button.bounds = compute_widget_bounds(gui, coords.x0, coords.x1, coords.y0, coords.y1, get_parent(gui, current_parent));

				add_widget(gui, GuiWidgetType_Button, u32(gui.buttons.size()) - 1u, id);
			}
			break;

			case GuiWidgetType_Label:
			{
				GuiLabel& label = gui.labels.emplace_back();

				label.text = (const char*)it;
				it += strlen(label.text) + 1u;
				if (*label.text == '\0')
					label.text = nullptr;

				Raw_Coords coords = _read<Raw_Coords>(it);
				label.style = _read<GuiLabelStyle>(it);

				label.bounds = compute_widget_bounds(gui, coords.x0, coords.x1, coords.y0, coords.y1, get_parent(gui, current_parent));

				add_widget(gui, GuiWidgetType_Label, u32(gui.labels.size()) - 1u, id);
			}
			break;

			case GuiWidgetType_Slider:
				break;

			case GuiWidgetType_Checkbox:
			{
				Raw_Checkbox raw = _read<Raw_Checkbox>(it);

				GuiCheckbox& cb = gui.checkboxes.emplace_back();
				cb.value = raw.value;
				cb.style = raw.style;
				cb.bounds = compute_widget_bounds(gui, raw.coords.x0, raw.coords.x1, raw.coords.y0, raw.coords.y1, get_parent(gui, current_parent));

				add_widget(gui, GuiWidgetType_Checkbox, u32(gui.checkboxes.size()) - 1u, id);
			}
			break;

			case GuiWidgetType_Drag:
			{
				Raw_Drag raw = _read<Raw_Drag>(it);

				GuiDrag& drag = gui.drags.emplace_back();
				drag.value = raw.value;
				drag.adv = raw.adv;
				drag.style = raw.style;
				drag.bounds = compute_widget_bounds(gui, raw.coords.x0, raw.coords.x1, raw.coords.y0, raw.coords.y1, get_parent(gui, current_parent));

				add_widget(gui, GuiWidgetType_Drag, u32(gui.drags.size()) - 1u, id);
			}
			break;

			}
		}

		// Update focus
		if (gui.current_focus.type != GuiWidgetType_None) {

			update_focus(gui, gui.current_focus);
			input.unused = false;
		}

		// Update widgets
		foreach(i, gui.widgets.size()) {

			const GuiWidget& w = gui.widgets[i];

			update_widget(gui, w);

			GuiParent* parent = get_parent(gui, w);
			if (parent) {

				i += parent->widget_count;
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

	void gui_pop_id(GUI* gui_)
	{
		PARSE_GUI();
		SV_ASSERT(gui.ids.size());
		gui.ids.pop_back();
		update_id(gui);
	}

	///////////////////////////////////////////// WIDGETS ///////////////////////////////////////////

	SV_INLINE static GuiWidget find_widget(GUI_internal& gui, GuiWidgetType type, u64 id)
	{
		GuiWidget res;
		res.type = GuiWidgetType_None;

		if (gui.current_parent == nullptr) {

			foreach(i, gui.widgets.size()) {

				const GuiWidget& w = gui.widgets[i];

				if (w.type == type && w.id == id) {

					res = w;
					break;
				}
				else {

					GuiParent* parent = get_parent(gui, w);
					if (parent) i += parent->widget_count;
				}
			}
		}
		else {

			GuiWidget* it = gui.widgets.data() + gui.current_parent->widget_offset;
			GuiWidget* end = it + gui.current_parent->widget_count;

			while (it != end) {

				if (it->type == type && it->id == id) {

					res = *it;
					break;
				}
				else {

					GuiParent* parent = get_parent(gui, *it);
					if (parent) it += parent->widget_count;
				}

				++it;
			}
		}

		return res;
	}

	SV_INLINE static void* find_widget_ptr(GUI_internal& gui, GuiWidgetType type, u64 id)
	{
		GuiWidget res = find_widget(gui, type, id);

		switch (res.type)
		{

		case GuiWidgetType_Container:
			return &gui.containers[res.index];

		case GuiWidgetType_Popup:
			return &gui.popups[res.index];

		case GuiWidgetType_Window:
			return &gui.windows[res.index];

		case GuiWidgetType_Button:
			return &gui.buttons[res.index];

		case GuiWidgetType_Drag:
			return &gui.drags[res.index];

		default:
			return nullptr;
		}
	}

	SV_INLINE void begin_parent(GUI_internal& gui, GuiParent* parent, u64 id)
	{
		if (parent)
			gui.current_parent = parent;
		gui_push_id((GUI*)& gui, id);
	}

	SV_INLINE void end_parent(GUI_internal& gui)
	{
		gui_pop_id((GUI*)& gui);

		if (gui.current_parent)
			gui.current_parent = get_parent(gui, gui.current_parent->parent);
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

		GuiContainer* c = (GuiContainer*)find_widget_ptr(gui, GuiWidgetType_Container, id);
		begin_parent(gui, c, id);
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

		GuiPopup* popup = nullptr;

		for (GuiPopup& p : gui.popups) {
			if (p.id == id) {
				popup = &p;
				break;
			}
		}

		if (popup) {

			if (popup->close_request)
				return false;

			Raw_Popup raw;
			raw.style = style;
			raw.bounds = popup->bounds;

			write_widget(gui, GuiWidgetType_Popup, id, &raw);
			begin_parent(gui, popup, id);
			return true;
		}
		else {

			bool open = false;

			if (input.mouse_buttons[mouse_button] == InputState_Released) {

				switch (trigger)
				{

				case GuiPopupTrigger_Parent:
				{
					if (gui.current_parent != nullptr) {

						const GuiParent& parent = *gui.current_parent;

						if (mouse_in_bounds(gui, parent.bounds)) {

							open = true;

							foreach(i, parent.widget_count) {

								const GuiWidget& w = gui.widgets[parent.widget_offset + i];

								v4_f32 bounds = get_widget_bounds(gui, w);

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

						GuiWidget w = find_widget(gui, gui.last.type, gui.last.id);
						v4_f32 bounds = get_widget_bounds(gui, w);

						if (mouse_in_bounds(gui, bounds)) {
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
				begin_parent(gui, popup, id);
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

			GuiWindow* wnd = (GuiWindow*)find_widget_ptr(gui, GuiWidgetType_Window, id);
			begin_parent(gui, wnd, id);
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

		GuiButton* state = (GuiButton*)find_widget_ptr(gui, GuiWidgetType_Button, id);

		if (state) {
			return state->pressed;
		}
		else {
			return false;
		}
	}

	bool gui_drag_f32(GUI* gui_, f32* value, f32 adv, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style)
	{
		PARSE_GUI();
		hash_combine(id, gui.current_id);

		bool modified;

		if (gui.current_focus.type == GuiWidgetType_Drag && gui.current_focus.id == id) {

			GuiDrag& drag = gui.drags[gui.current_focus.index];
			*value = drag.value;
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

		if (gui.current_focus.type == GuiWidgetType_Checkbox && gui.current_focus.id == id) {

			GuiCheckbox& cb = gui.checkboxes[gui.current_focus.index];
			*value = cb.value;
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
			const GuiButton& button = gui.buttons[w.index];

			v2_f32 pos = v2_f32(button.bounds.x, button.bounds.y) * 2.f - 1.f;
			v2_f32 size = v2_f32(button.bounds.z, button.bounds.w) * 2.f;

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

			GuiDrag& drag = gui.drags[w.index];
			pos = v2_f32{ drag.bounds.x, drag.bounds.y } *2.f - 1.f;
			size = v2_f32{ drag.bounds.z, drag.bounds.w } *2.f;

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

		case GuiWidgetType_Label:
		{
			begin_debug_batch(cmd);

			v2_f32 pos;
			v2_f32 size;

			GuiLabel& label = gui.labels[w.index];
			pos = v2_f32{ label.bounds.x, label.bounds.y } *2.f - 1.f;
			size = v2_f32{ label.bounds.z, label.bounds.w } *2.f;

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

			GuiCheckbox& cb = gui.checkboxes[w.index];
			pos = v2_f32{ cb.bounds.x, cb.bounds.y } *2.f - 1.f;
			size = v2_f32{ cb.bounds.z, cb.bounds.w } *2.f;

			draw_debug_quad(pos.getVec3(0.f), size, cb.style.color, cmd);

			GuiBox* box;
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
			const GuiContainer& container = gui.containers[w.index];

			v2_f32 pos = v2_f32(container.bounds.x, container.bounds.y) * 2.f - 1.f;
			v2_f32 size = v2_f32(container.bounds.z, container.bounds.w) * 2.f;

			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, container.style.color, cmd);
			end_debug_batch(true, false, XMMatrixIdentity(), cmd);
		}
		break;

		case GuiWidgetType_Popup:
		{
			const GuiPopup& popup = gui.popups[w.index];

			v2_f32 pos = v2_f32(popup.bounds.x, popup.bounds.y) * 2.f - 1.f;
			v2_f32 size = v2_f32(popup.bounds.z, popup.bounds.w) * 2.f;

			begin_debug_batch(cmd);
			draw_debug_quad(pos.getVec3(), size, popup.style.background_color, cmd);
			end_debug_batch(true, false, XMMatrixIdentity(), cmd);
		}
		break;

		case GuiWidgetType_Window:
		{
			const GuiWindow& window = gui.windows[w.index];
			const GuiWindowState& state = *window.state;
			const GuiWindowStyle& style = window.style;

			v4_f32 decoration = compute_window_decoration_bounds(gui, window);

			v2_f32 outline_size;
			v2_f32 content_position;
			v2_f32 content_size;

			content_position = v2_f32{ state.bounds.x, state.bounds.y };
			content_size = v2_f32{ state.bounds.z, state.bounds.w };

			outline_size = content_size + v2_f32{ style.outline_size, style.outline_size * gui.aspect } *2.f;

			// Normal to clip
			content_position = content_position * 2.f - 1.f;
			content_size = content_size * 2.f;
			outline_size = outline_size * 2.f;
			v2_f32 decoration_position = v2_f32(decoration.x, decoration.y) * 2.f - 1.f;
			v2_f32 decoration_size = v2_f32(decoration.z, decoration.w) * 2.f;

			begin_debug_batch(cmd);

			draw_debug_quad(content_position.getVec3(0.f), outline_size, style.outline_color, cmd);
			draw_debug_quad(content_position.getVec3(0.f), content_size, style.color, cmd);
			draw_debug_quad(decoration_position.getVec3(0.f), decoration_size, style.decoration_color, cmd);

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

		// If it is a parent, draw childs
		{
			GuiParent* parent = get_parent(gui, w);
			if (parent) {

				GuiWidget* it = gui.widgets.data() + parent->widget_offset;
				GuiWidget* end = it + parent->widget_count;

				// Draw normal widgets
				while (it != end) {

					GuiParent* p = get_parent(gui, *it);
					if (p) {

						it += p->widget_count;
					}
					else draw_widget(gui, *it, cmd);

					++it;
				}

				// Draw parents
				it = gui.widgets.data() + parent->widget_offset;

				while (it != end) {

					GuiParent* p = get_parent(gui, *it);
					if (p) {

						draw_widget(gui, *it, cmd);
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

		foreach(i, gui.widgets.size()) {

			const GuiWidget& w = gui.widgets[i];

			draw_widget(gui, w, cmd);

			GuiParent* parent = get_parent(gui, w);
			if (parent) {

				i += parent->widget_count;
			}
		}
	}


}
