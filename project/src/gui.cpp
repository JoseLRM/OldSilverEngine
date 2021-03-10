#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/FrameList.h"

#include "renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

namespace sv {

	/*
	  TODO LIST:
	  - Align memory using only one allocator
	  - Combobox
	  - Handle text dynamic memory
	  - Hot Label ????
	  - Draw first the container with the focus
	  - Reverse rendering
	*/

	enum GuiWidgetType : u32 {
		GuiWidgetType_None,
		GuiWidgetType_Container,
		GuiWidgetType_Window,
		GuiWidgetType_Popup,
		GuiWidgetType_Button,
		GuiWidgetType_Slider,
		GuiWidgetType_Label,
		GuiWidgetType_CheckBox,
		GuiWidgetType_Drag
	};

	struct GuiWidgetIndex {

		GuiWidgetType type = GuiWidgetType_None;
		u32 index = 0u;

	};

	struct GuiContainer {

		const GuiWidgetIndex parent_index;
		v4_f32 bounds;
		GuiContainerStyle style;

		GuiContainer(const GuiWidgetIndex& index) : parent_index(index) {}

	};

	struct GuiWindowState {

		v4_f32 bounds;
		bool show;

	};

	struct GuiWindow {

		const GuiWidgetIndex parent_index;
		GuiWindowState* state;
		GuiWindowStyle style;
		std::string title;

		GuiWindow(const GuiWidgetIndex& index) : parent_index(index) {}
		
	};

	struct GuiPopupState {

		bool active;
		v4_f32 bounds;
		u64 id;

	};

	struct GuiPopup {

		const GuiWidgetIndex parent_index;
		GuiPopupState* state;
		GuiPopupStyle style;

		GuiPopup(const GuiWidgetIndex& index) : parent_index(index) {}

	};

	struct GuiButton {

		v4_f32 bounds;
		GuiButtonStyle style;
		bool hot;
		bool pressed;
		std::string text;

	};

	struct GuiSlider {

		v4_f32 bounds;
		GuiSliderStyle style;
		f32 slider_value;
		
	};

	struct GuiLabel {

		v4_f32 bounds;
		GuiTextStyle style;
		std::string text;
		
	};

	struct GuiDrag {

		v4_f32 bounds;
		GuiDragStyle style;
		f32 value;
		
	};

	struct GuiCheckBoxState {
		bool active = false;
	};

	struct GuiCheckBox {
		
		v4_f32 bounds;
		GuiCheckBoxStyle style;
		bool active;

	};

	struct GUI_internal {

		size_t hashcode = 0u;
		
		FrameList<GuiContainer> containers;
		FrameList<GuiWindow>	windows;
		FrameList<GuiPopup>		popups;
		FrameList<GuiButton>	buttons;
		FrameList<GuiSlider>	sliders;
		FrameList<GuiLabel>		labels;
		FrameList<GuiCheckBox>	checkboxes;
		FrameList<GuiDrag>      drags;

		FrameList<GuiWidgetIndex> widgets;

		std::unordered_map<std::string, GuiWindowState> window_state;
		std::unordered_map<u64, GuiCheckBoxState> checkbox_state;
		std::unordered_map<u64, GuiPopupState> popup_state;
		GuiPopupState current_popup;

		GuiWidgetIndex parent_index;
		struct {
			u64 id = 0u;
			u32 last_index = u32_max;
			GuiWidgetType type = GuiWidgetType_None;
			u32 action = 0u;
		} focus;

		v2_f32 resolution;
		f32 aspect;
		v2_f32 mouse_position;
		bool input_unused;

		v2_f32 begin_position;

	};

	void gui_begin(GUI* gui_, f32 width, f32 height)
	{
		PARSE_GUI();

		gui.containers.reset();
		gui.windows.reset();
		gui.buttons.reset();
		gui.sliders.reset();
		gui.labels.reset();
		gui.checkboxes.reset();
		gui.drags.reset();

		gui.widgets.reset();

		gui.parent_index.type = GuiWidgetType_None;
		gui.resolution = { width, height };
		gui.aspect = width / height;
		gui.mouse_position = input.mouse_position + 0.5f;
		gui.focus.last_index = u32_max;
		gui.input_unused = input.unused;
	}

	SV_INLINE void free_focus(GUI_internal& gui)
	{
		gui.focus.type = GuiWidgetType_None;
	}

	SV_INLINE void set_focus(GUI_internal& gui, GuiWidgetType type, u32 index, u64 id, u32 action = 0u)
	{
		gui.focus.type = type;
		gui.focus.last_index = index;
		gui.focus.id = id;
		gui.focus.action = action;
	}

	SV_INLINE void update_focus(GUI_internal& gui, u32 index)
	{
		gui.focus.last_index = index;
	}

	void gui_end(GUI* gui_)
	{
		PARSE_GUI();

		if (gui.focus.last_index == u32_max)
			free_focus(gui);

		switch (gui.focus.type)
		{

		case GuiWidgetType_Window:
		{
			GuiWindow& window = gui.windows[gui.focus.last_index];
			
			if (input.mouse_buttons[MouseButton_Left] == InputState_None) {

				free_focus(gui);
			}
			else {

				input.unused = false;

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

	Result gui_create(u64 hashcode, GUI** pgui)
	{
		GUI_internal& gui = *new GUI_internal();

		gui.hashcode = hashcode;

		// Load state
		{
			u64 hashcode = hash_string("GUI SYSTEM") ^ gui.hashcode;

			Archive archive;

			Result res = bin_read(hashcode, archive);
			if (result_fail(res)) {
				SV_LOG_ERROR("Can't deserialize gui states");
			}
			else {
				
				Version engine_version;
				archive >> engine_version;

				// Load window states

				u32 count;
				archive >> count;

				foreach(i, count) {

					GuiWindowState state;
					std::string title;
				
					archive >> title;
					archive >> state.bounds;
					archive >> state.show;

					gui.window_state[title] = state;
				}
			}
		}
		
		*pgui = reinterpret_cast<GUI*>(&gui);
		return Result_Success;
	}

	Result gui_destroy(GUI* gui_)
	{
		PARSE_GUI();

		// Save state
		{
			u64 hashcode = hash_string("GUI SYSTEM") ^ gui.hashcode;

			Archive archive;
			archive << engine.version;

			// Save window states
			archive << u32(gui.window_state.size());

			for (auto& it : gui.window_state) {
				archive << it.first;
				archive << it.second.bounds;
				archive << it.second.show;
			}

			Result res = bin_write(hashcode, archive);
			if (result_fail(res)) {
				SV_LOG_ERROR("Can't serialize gui states");
			}
		}

		delete& gui;
		return Result_Success;
	}

	SV_INLINE static v4_f32 get_widget_bounds(const GUI_internal& gui, GuiWidgetIndex w)
	{
		switch (w.type)
		{

		case GuiWidgetType_Container:
			return gui.containers[w.index].bounds;

		case GuiWidgetType_Window:
			return gui.windows[w.index].state->bounds;

		case GuiWidgetType_Popup:
			return gui.popups[w.index].state->bounds;

		case GuiWidgetType_Button:
			return gui.buttons[w.index].bounds;

		case GuiWidgetType_Slider:
			return gui.sliders[w.index].bounds;

		case GuiWidgetType_Label:
			return gui.labels[w.index].bounds;

		case GuiWidgetType_CheckBox:
			return gui.checkboxes[w.index].bounds;

		case GuiWidgetType_Drag:
			return gui.drags[w.index].bounds;

		default:
			return {};
		}
	}

	SV_INLINE static v4_f32 get_parent_bounds(const GUI_internal& gui)
	{
		if (gui.parent_index.type == GuiWidgetType_None) return v4_f32{ 0.5f, 0.5f, 1.f, 1.f };

		switch (gui.parent_index.type)
		{
		case GuiWidgetType_Container:
			return gui.containers[gui.parent_index.index].bounds;
			break;

		case GuiWidgetType_Window:
			return gui.windows[gui.parent_index.index].state->bounds;
			break;

		case GuiWidgetType_Popup:
			return gui.popups[gui.parent_index.index].state->bounds;
			break;

		default:
			return v4_f32{ 0.f, 0.f, 0.f, 0.f };
		}
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

		case GuiConstraint_Center:
			res = 0.5f * parent_dimension;
			break;

		case GuiConstraint_Pixel:
		case GuiConstraint_InversePixel:
			res = coord.value / resolution;
			break;

		}

		// Inverse coord
		if (coord.constraint == GuiConstraint_InversePixel) {
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

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1)
	{
#ifndef SV_DEV
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

		v4_f32 parent_bounds = get_parent_bounds(gui);

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

	SV_INLINE static u32 create_widget(GUI_internal& gui, GuiWidgetType type)
	{
		u32 index;
		
		switch (type) {

		case GuiWidgetType_Container:
			index = u32(gui.containers.size());
			gui.containers.emplace_back(gui.parent_index);
			break;
			
		case GuiWidgetType_Window:
			index = u32(gui.windows.size());
			gui.windows.emplace_back(gui.parent_index);
			break;

		case GuiWidgetType_Popup:
			index = u32(gui.popups.size());
			gui.popups.emplace_back(gui.parent_index);
			break;
			
		case GuiWidgetType_Button:
			index = u32(gui.buttons.size());
			gui.buttons.emplace_back();
			break;

		case GuiWidgetType_Slider:
			index = u32(gui.sliders.size());
			gui.sliders.emplace_back();
			break;

		case GuiWidgetType_Label:
			index = u32(gui.labels.size());
			gui.labels.emplace_back();
			break;

		case GuiWidgetType_CheckBox:
			index = u32(gui.checkboxes.size());
			gui.checkboxes.emplace_back();
			break;

		case GuiWidgetType_Drag:
			index = u32(gui.drags.size());
			gui.drags.emplace_back();
			break;

		default:
			SV_ASSERT(!"Unknown widget type");
			
		}

		// Add widget index
		
		GuiWidgetIndex& i = gui.widgets.emplace_back();
		i.index = index;
		i.type = type;

		// New parent
		
		if (type == GuiWidgetType_Container || type == GuiWidgetType_Window || type == GuiWidgetType_Popup) {

			gui.parent_index.type = type;
			gui.parent_index.index = index;
		}
		
		return index;
	}

	static void end_parent(GUI_internal& gui)
	{
		SV_ASSERT(gui.parent_index.type != GuiWidgetType_None);

		switch (gui.parent_index.type)
		{
		case GuiWidgetType_Container:
			gui.parent_index = gui.containers[gui.parent_index.index].parent_index;
			break;

		case GuiWidgetType_Window:
			gui.parent_index = gui.windows[gui.parent_index.index].parent_index;
			break;

		case GuiWidgetType_Popup:
			gui.parent_index = gui.popups[gui.parent_index.index].parent_index;
			break;
		}
	}

	void gui_begin_container(GUI* gui_, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiContainerStyle& style)
	{
		PARSE_GUI();

		v4_f32 bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		GuiContainer& container = gui.containers[create_widget(gui, GuiWidgetType_Container)];
		container.style = style;
		container.bounds = bounds;

		if (input.unused && mouse_in_bounds(gui, bounds)) {
			gui.input_unused = true;
			input.unused = false;
		}
	}

	void gui_end_container(GUI* gui_)
	{
		PARSE_GUI();
		SV_ASSERT(gui.parent_index.type == GuiWidgetType_Container);
		end_parent(gui);
	}

	SV_INLINE static GuiWindowState* get_window_state(GUI_internal& gui, const char* title)
	{				
		auto it = gui.window_state.find(title);
		
		if (it == gui.window_state.end()) return nullptr;
		return &it->second;
	}

	bool gui_begin_window(GUI* gui_, const char* title, const GuiWindowStyle& style)
	{
		PARSE_GUI();
		SV_ASSERT(title);
		SV_ASSERT(gui.parent_index.type == GuiWidgetType_None);
		
		GuiWindowState* state;

		auto it = gui.window_state.find(title);
		if (it == gui.window_state.end()) {

			GuiWindowState s;
			s.bounds = { 0.5f, 0.5f, 0.3f, 0.5f };
			s.show = true;
			
			gui.window_state[title] = s;
			state = &gui.window_state[title];
		}
		else state = &it->second;

		if (!state->show) return false;

		u32 index = create_widget(gui, GuiWidgetType_Window);
		GuiWindow& window = gui.windows[index];
		window.style = style;
		window.state = state;
		window.title = title;

		if (input.unused && mouse_in_bounds(gui, state->bounds)) {
			gui.input_unused = true;
			input.unused = false;
		}

		// Select window
		if (gui.input_unused) {
			
			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

				v4_f32 decoration = compute_window_decoration_bounds(gui, window);

				if (mouse_in_bounds(gui, decoration)) {
				
					gui.begin_position = v2_f32(window.state->bounds.x, window.state->bounds.y) - gui.mouse_position;
					set_focus(gui, GuiWidgetType_Window, index, (u64)title, 0u);
					gui.input_unused = false;
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
						set_focus(gui, GuiWidgetType_Window, index, (u64)title, action);
						gui.input_unused = false;
					}
				}
			}
			else {

				if (gui.focus.type == GuiWidgetType_Window && gui.focus.id == (u64)title) {
					update_focus(gui, index);
					gui.input_unused = false;
				}
			}
		}

		return true;
	}

	void gui_end_window(GUI* gui_)
	{
		PARSE_GUI();
		end_parent(gui);
	}
	
	Result gui_show_window(GUI* gui_, const char* title)
	{
		PARSE_GUI();

		GuiWindowState* state = get_window_state(gui, title);
		if (state == nullptr) return Result_NotFound;
		
		state->show = true;
		return Result_Success;
	}
	
	Result gui_hide_window(GUI* gui_, const char* title)
	{
		PARSE_GUI();

		GuiWindowState* state = get_window_state(gui, title);
		if (state == nullptr) return Result_NotFound;
				
		state->show = false;
		return Result_Success;
	}

	bool gui_begin_popup(GUI* gui_, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, const GuiPopupStyle& style)
	{
		PARSE_GUI();

		bool open = false;

		if (input.mouse_buttons[mouse_button] == InputState_Released) {

			switch (trigger)
			{
			case GuiPopupTrigger_Parent:
			
			if (gui.parent_index.type != GuiWidgetType_None) {

				v4_f32 parent_bounds = get_parent_bounds(gui);

				if (mouse_in_bounds(gui, parent_bounds)) {

					open = true;
					u32 begin_index = gui.widgets.size() - 1u;

					while (gui.widgets[begin_index].type != gui.parent_index.type && gui.widgets[begin_index].index != gui.parent_index.index)
						--begin_index;

					for (u32 i = begin_index; i < gui.widgets.size(); ++i) {

						v4_f32 bounds = get_widget_bounds(gui, gui.widgets[i]);

						if (mouse_in_bounds(gui, bounds)) {
							open = false;
							break;
						}
					}
				}
			}
			break;

			case GuiPopupTrigger_LastWidget:
				if (gui.widgets.size()) {

					GuiWidgetIndex last = gui.widgets.back();
					v4_f32 bounds = get_widget_bounds(gui, last);

					if (mouse_in_bounds(gui, bounds)) {
						open = true;
					}
				}
				break;
			}
		}

		// Get state
		GuiPopupState* state;

		if (trigger == GuiPopupTrigger_Custom) {

			auto it = gui.popup_state.find(id);
			if (it == gui.popup_state.end()) {

				GuiPopupState s;
				s.active = false;
				s.bounds = { 0.5f, 0.5f, 0.1f, 0.1f };
				s.id = id;

				gui.popup_state[id] = s;
				state = &gui.popup_state[id];
			}
			else state = &it->second;
		}
		else {

			state = &gui.current_popup;

			if (open && state->id != id) {

				state->id = id;
			}
		}

		if (open) {
			state->active = true;
			state->bounds.z = 0.1f;
			state->bounds.w = 0.2f;
			state->bounds.x = gui.mouse_position.x + state->bounds.z * 0.5f;
			state->bounds.y = gui.mouse_position.y - state->bounds.w * 0.5f;
		}
		else {

			bool any_mouse_button = false;

			foreach(i, MouseButton_MaxEnum) {
				if (input.mouse_buttons[i] == InputState_Pressed) {
					any_mouse_button = true;
					break;
				}
			}

			if (any_mouse_button && !mouse_in_bounds(gui, state->bounds)) {
				state->active = false;
			}
		}

		if (!state->active || state->id != id) return false;

		u32 index = create_widget(gui, GuiWidgetType_Popup);
		GuiPopup& popup = gui.popups[index];
		popup.style = style;
		popup.state = state;

		if (input.unused && mouse_in_bounds(gui, state->bounds)) {
			gui.input_unused = true;
			input.unused = false;
		}

		return true;
	}
	
	void gui_end_popup(GUI* gui_)
	{
		PARSE_GUI();
		end_parent(gui);
	}

	void gui_open_popup(GUI* gui_, u64 id)
	{
		PARSE_GUI();

		GuiPopupState* state;
		
		auto it = gui.popup_state.find(id);
		if (it == gui.popup_state.end()) return;
		
		state->active = true;
	}

	bool gui_button(GUI* gui_, const char* text, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiButtonStyle& style)
	{
		PARSE_GUI();

		GuiButton& button = gui.buttons[create_widget(gui, GuiWidgetType_Button)];
		button.bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		button.style = style;
		button.hot = false;
		button.pressed = false;
		button.text = text;

		// Check state
		if (gui.input_unused) {

			// Check if is hot
			button.hot = mouse_in_bounds(gui, button.bounds);

			// Check if is presssed
			if (button.hot) {

				InputState input_state = input.mouse_buttons[MouseButton_Left];

				if (input_state == InputState_Released) {

					button.pressed = true;
					gui.input_unused = false;
				}
			}
		}

		return button.pressed;
	}

	bool gui_slider(GUI* gui_, f32* value, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiSliderStyle& style)
	{
		PARSE_GUI();

		u32 index = create_widget(gui, GuiWidgetType_Slider);
		GuiSlider& slider = gui.sliders[index];
		slider.bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		slider.style = style;
		slider.slider_value = (*value - min) / (max - min);

		bool focused = false;

		// Check state
		if (gui.input_unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			if (gui.focus.type == GuiWidgetType_Slider && gui.focus.id == id) {

				if (input_state == InputState_None || input_state == InputState_Released) {

					free_focus(gui);
				}
				else {

					f32 slider_button_width = slider.style.button_size.x;
					f32 slider_min_pos = slider.bounds.x - slider.bounds.z * 0.5f + slider_button_width * 0.5f;
					
					f32 mouse_value = std::min(std::max((gui.mouse_position.x - slider_min_pos) / (slider.bounds.z - slider_min_pos), 0.f), 1.f);
					slider.slider_value = mouse_value;
					*value = mouse_value * (max - min) + min;

					focused = true;
					update_focus(gui, index);
					gui.input_unused = false;
				}
			}
			else if (input_state == InputState_Pressed && mouse_in_bounds(gui, slider.bounds)) {

				set_focus(gui, GuiWidgetType_Slider, index, id);
				gui.input_unused = false;
			}
		}

		return focused;
	}

	void gui_text(GUI* gui_, const char* text, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiTextStyle& style)
	{
		PARSE_GUI();

		GuiLabel& label = gui.labels[create_widget(gui, GuiWidgetType_Label)];
		label.bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		label.style = style;
		label.text = text;
	}

	bool gui_checkbox(GUI* gui_, bool* value, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckBoxStyle& style)
	{
		PARSE_GUI();

		GuiCheckBox& cb = gui.checkboxes[create_widget(gui, GuiWidgetType_CheckBox)];
		cb.bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		cb.style = style;
		cb.active = *value;

		bool pressed = false;

		// Check state
		if (gui.input_unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			// Check if is presssed
			if (input_state == InputState_Released && mouse_in_bounds(gui, cb.bounds)) {

				*value = !*value;
				cb.active = *value;
				gui.input_unused = false;
				pressed = true;
			}
		}

		return pressed;
	}

	bool gui_checkbox_id(GUI* gui_, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckBoxStyle& style)
	{
		PARSE_GUI();

		GuiCheckBoxState* state;

		auto it = gui.checkbox_state.find(id);
		if (it == gui.checkbox_state.end()) {

			gui.checkbox_state[id] = {};
			state = &gui.checkbox_state[id];
		}
		else state = &it->second;

		gui_checkbox(gui_, &state->active, x0, x1, y0, y1, style);

		return state->active;
	}

	bool gui_drag_f32(GUI* gui_, f32* value, f32 adv, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style)
	{
		PARSE_GUI();

		u32 index = create_widget(gui, GuiWidgetType_Drag);
		GuiDrag& drag = gui.drags[index];
		drag.bounds = compute_widget_bounds(gui, x0, x1, y0, y1);
		drag.style = style;

		bool focused = false;

		// Check state
		if (gui.input_unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			if (gui.focus.type == GuiWidgetType_Drag && gui.focus.id == id) {

				if (input_state == InputState_None || input_state == InputState_Released) {

					free_focus(gui);
				}
				else {

					*value += input.mouse_dragged.x * gui.resolution.x * adv;
					
					focused = true;
					update_focus(gui, index);
					gui.input_unused = false;
				}
			}
			else if (input_state == InputState_Pressed && mouse_in_bounds(gui, drag.bounds)) {

				set_focus(gui, GuiWidgetType_Drag, index, id);
				gui.input_unused = false;
			}
		}

		// TODO: limit value
		drag.value = *value;

		return focused;
	}

	///////////////////////////////////// RENDERING ///////////////////////////////////////

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

	void gui_draw(GUI* gui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_GUI();

		for (GuiWidgetIndex& w : gui.widgets) {

			switch (w.type)
			{
			case GuiWidgetType_Button:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiButton& button = gui.buttons[w.index];
				pos = v2_f32{ button.bounds.x, button.bounds.y } * 2.f - 1.f;
				size = v2_f32{ button.bounds.z, button.bounds.w } * 2.f;
				color = button.hot ? button.style.hot_color : button.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);

				if (button.text.size()) {

					f32 font_size = size.y;

					draw_text(button.text.c_str(), button.text.size()
						, pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font_opensans.vertical_offset * font_size, 
						size.x, 1u, font_size, gui.aspect, TextAlignment_Center, &font_opensans, button.style.text_color, cmd);
				}					
			}
			break;

			case GuiWidgetType_Container:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;

				GuiContainer& container = gui.containers[w.index];
				pos = v2_f32{ container.bounds.x, container.bounds.y } *2.f - 1.f;
				size = v2_f32{ container.bounds.z, container.bounds.w } *2.f;

				draw_debug_quad(pos.getVec3(0.f), size, container.style.color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);
			}
			break;

			case GuiWidgetType_Popup:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;

				GuiPopup& popup = gui.popups[w.index];
				GuiPopupState& state = *popup.state;

				pos = v2_f32{ state.bounds.x, state.bounds.y } *2.f - 1.f;
				size = v2_f32{ state.bounds.z, state.bounds.w } *2.f;

				draw_debug_quad(pos.getVec3(0.f), size, popup.style.background_color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);
			}
			break;

			case GuiWidgetType_Window:
			{
				begin_debug_batch(cmd);

				GuiWindow& window = gui.windows[w.index];
				GuiWindowState& state = *window.state;
				GuiWindowStyle& style = window.style;

				v4_f32 decoration = compute_window_decoration_bounds(gui, window);

				v2_f32 outline_size;
				v2_f32 content_position;
				v2_f32 content_size;

				content_position = v2_f32{ state.bounds.x, state.bounds.y };
				content_size = v2_f32{ state.bounds.z, state.bounds.w };

				outline_size = content_size + v2_f32{ style.outline_size, style.outline_size * gui.aspect } * 2.f;

				// Normal to clip
				content_position = content_position * 2.f - 1.f;
				content_size = content_size * 2.f;
				outline_size = outline_size * 2.f;
				v2_f32 decoration_position = v2_f32(decoration.x, decoration.y) * 2.f - 1.f;
				v2_f32 decoration_size = v2_f32(decoration.z, decoration.w) * 2.f;

				draw_debug_quad(content_position.getVec3(0.f), outline_size, style.outline_color, cmd);
				draw_debug_quad(content_position.getVec3(0.f), content_size, style.color, cmd);
				draw_debug_quad(decoration_position.getVec3(0.f), decoration_size, style.decoration_color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);

				if (window.title.size()) {

					f32 font_size = decoration_size.y * 0.5f;

					draw_text(window.title.c_str(), window.title.size()
						, decoration_position.x - decoration_size.x * 0.5f + 0.01f, decoration_position.y +
						decoration_size.y * 0.25f - font_opensans.vertical_offset * font_size, decoration_size.x, 1u, font_size,
						gui.aspect, TextAlignment_Left, &font_opensans, cmd);
				}
			}
			break;

			case GuiWidgetType_Slider:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiSlider& slider = gui.sliders[w.index];
				pos = v2_f32{ slider.bounds.x, slider.bounds.y } *2.f - 1.f;
				size = v2_f32{ slider.bounds.z, slider.bounds.w } *2.f;
				color = slider.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);

				f32 slider_pos = slider.bounds.x + slider.style.button_size.x * 0.5f + slider.slider_value * (slider.bounds.z - slider.style.button_size.x);
				
				draw_debug_quad({ slider_pos, slider.bounds.y, 0.f }, slider.style.button_size, slider.style.button_color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);
			}
			break;

			case GuiWidgetType_Label:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;

				GuiLabel& label = gui.labels[w.index];
				pos = v2_f32{ label.bounds.x, label.bounds.y } * 2.f - 1.f;
				size = v2_f32{ label.bounds.z, label.bounds.w } * 2.f;

				draw_debug_quad(pos.getVec3(0.f), size, label.style.background_color, cmd);

				end_debug_batch(true, false, XMMatrixIdentity(), cmd);

				if (label.text.size()) {
				
					f32 font_size = size.y;

					draw_text(label.text.c_str(), label.text.size(), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - 
						font_opensans.vertical_offset * font_size, size.x, 1u, font_size, gui.aspect, label.style.text_alignment, 
						&font_opensans, label.style.text_color, cmd);
				}
			}
			break;

			case GuiWidgetType_CheckBox:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;

				GuiCheckBox& cb = gui.checkboxes[w.index];
				pos = v2_f32{ cb.bounds.x, cb.bounds.y } *2.f - 1.f;
				size = v2_f32{ cb.bounds.z, cb.bounds.w } *2.f;

				draw_debug_quad(pos.getVec3(0.f), size, cb.style.color, cmd);
				
				GuiBox* box;
				if (cb.active) box = &cb.style.active_box;
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

			}
		}
	}

}
