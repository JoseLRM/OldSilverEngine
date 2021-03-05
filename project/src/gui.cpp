#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/FrameList.h"

#include "renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

namespace sv {

	/*
	  TODO LIST:
	  - Align memory using only one allocator
	  - Popups
	  - Combobox
	  - Not use depthstencil (right now)
	  - Handle text dynamic memory
	  - Hot Label ????
	  - Save states in bin file
	  - Draw first the container with the focus
	  - GuiDrag rendering
	  - Remove GuiDim and use only coords
	  - Reverse rendering
	*/

	enum GuiWidgetType : u32 {
		GuiWidgetType_None,
		GuiWidgetType_Container,
		GuiWidgetType_Window,
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
		FrameList<GuiButton>	buttons;
		FrameList<GuiSlider>	sliders;
		FrameList<GuiLabel>	labels;
		FrameList<GuiCheckBox>	checkboxes;
		FrameList<GuiDrag>      drags;

		FrameList<GuiWidgetIndex> widgets;

		std::unordered_map<std::string, GuiWindowState> window_state;
		std::unordered_map<u64, GuiCheckBoxState> checkbox_state;

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

			ArchiveI archive;

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

			ArchiveO archive;
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

		default:
			return v4_f32{ 0.f, 0.f, 0.f, 0.f };
		}
	}

	SV_INLINE static f32 compute_coord(const GUI_internal& gui, const GuiCoord& coord, f32 aspect_coord, f32 dimension, f32 resolution, bool vertical, f32 parent_coord, f32 parent_dimension)
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
			res = coord.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = aspect_coord * coord.value * (vertical ? gui.aspect : (1.f / gui.aspect));
			break;

		}

		// Inverse coord
		if (coord.alignment >= 3u) {
			res = parent_dimension - res;
		}

		// Parent offset
		res += (parent_coord - parent_dimension * 0.5f);

		// Alignment
		switch (coord.alignment)
		{
		case GuiAlignment_Left:
		case GuiAlignment_InverseLeft:
			res = res + dimension * 0.5f;
			break;

		case GuiAlignment_Right:
		case GuiAlignment_InverseRight:
			res = res - dimension * 0.5f;
			break;
		}

		return res;
	}

	SV_INLINE static f32 compute_dimension(const GUI_internal& gui, const GuiDim& dimension, f32 inv_dimension, f32 resolution, bool vertical, f32 parent_dimension)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (dimension.constraint)
		{
		case GuiConstraint_Relative:
			res = dimension.value * parent_dimension;
			break;

		case GuiConstraint_Center:
			res = 0.5f * parent_dimension;
			break;

		case GuiConstraint_Pixel:
			res = dimension.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = inv_dimension * (vertical ? gui.aspect : (1.f / gui.aspect));
			break;
		}

		return res;
	}

	SV_INLINE bool mouse_in_bounds(const GUI_internal& gui, const v4_f32& bounds)
	{
		return abs(bounds.x - gui.mouse_position.x) <= bounds.z * 0.5f && abs(bounds.y - gui.mouse_position.y) <= bounds.w * 0.5f;
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiCoord& coord_x, GuiCoord& coord_y, GuiDim& dim_w, GuiDim& dim_h)
	{
#ifndef SV_DIST
		if (coord_x.constraint == GuiConstraint_Aspect && coord_x.constraint == coord_y.constraint) {
			SV_LOG_WARNING("Gui coords has both aspect constraint, that's not possible...");
		}
		if (dim_w.constraint == GuiConstraint_Aspect && dim_w.constraint == dim_h.constraint) {
			SV_LOG_WARNING("Gui dimensions has both aspect constraint, that's not possible...");
		}
#endif

		// Compute local bounds
		f32 w;
		f32 h;
		f32 x;
		f32 y;

		v4_f32 parent_bounds = get_parent_bounds(gui);

		if (dim_w.constraint == GuiConstraint_Aspect) {
			h = compute_dimension(gui, dim_h, 0.5f, gui.resolution.y, true, parent_bounds.w);
			w = compute_dimension(gui, dim_w, h, gui.resolution.x, false, parent_bounds.z);
		}
		else {
			w = compute_dimension(gui, dim_w, 0.5f, gui.resolution.x, false, parent_bounds.z);
			h = compute_dimension(gui, dim_h, w, gui.resolution.y, true, parent_bounds.w);
		}

		if (coord_x.constraint == GuiConstraint_Aspect) {
			y = compute_coord(gui, coord_y, 0.5f, h, gui.resolution.y, true, parent_bounds.y, parent_bounds.w);
			x = compute_coord(gui, coord_x, y, w, gui.resolution.x, false, parent_bounds.x, parent_bounds.z);
		}
		else {
			x = compute_coord(gui, coord_x, 0.5f, w, gui.resolution.x, false, parent_bounds.x, parent_bounds.z);
			y = compute_coord(gui, coord_y, x, h, gui.resolution.y, true, parent_bounds.y, parent_bounds.w);
		}

		return { x, y, w, h };
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

		case GuiWidgetType_Checkbox:
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
		
		if (type == GuiWidgetType_Container || type == GuiWidgetType_Window) {

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
		}
	}

	void gui_begin_container(GUI* gui_, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiContainerStyle& style)
	{
		PARSE_GUI();

		GuiContainer& container = gui.containers[create_widget(gui, GuiWidgetType_Container)];
		container.style = style;
		container.bounds = compute_widget_bounds(gui, x, y, w, h);
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

		// Select window
		if (input.unused) {
			
			if (input.mouse_buttons[MouseButton_Left] == InputState_Pressed) {

				v4_f32 decoration = compute_window_decoration_bounds(gui, window);

				if (mouse_in_bounds(gui, decoration)) {
				
					gui.begin_position = v2_f32(window.state->bounds.x, window.state->bounds.y) - gui.mouse_position;
					set_focus(gui, GuiWidgetType_Window, index, (u64)title, 0u);
					input.unused = false;
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
						input.unused = false;
					}
				}
			}
			else {

				if (gui.focus.type == GuiWidgetType_Window && gui.focus.id == (u64)title) {
					update_focus(gui, index);
					input.unused = false;
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

	bool gui_button(GUI* gui_, const char* text, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiButtonStyle& style)
	{
		PARSE_GUI();

		GuiButton& button = gui.buttons[create_widget(gui, GuiWidgetType_Button)];
		button.bounds = compute_widget_bounds(gui, x, y, w, h);
		button.style = style;
		button.hot = false;
		button.pressed = false;
		button.text = text;

		// Check state
		if (input.unused) {

			// Check if is hot
			button.hot = mouse_in_bounds(gui, button.bounds);

			// Check if is presssed
			if (button.hot) {

				InputState input_state = input.mouse_buttons[MouseButton_Left];

				if (input_state == InputState_Released) {

					button.pressed = true;
					input.unused = false;
				}
			}
		}

		return button.pressed;
	}

	bool gui_slider(GUI* gui_, f32* value, f32 min, f32 max, u64 id, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiSliderStyle& style)
	{
		PARSE_GUI();

		u32 index = create_widget(gui, GuiWidgetType_Slider);
		GuiSlider& slider = gui.sliders[index];
		slider.bounds = compute_widget_bounds(gui, x, y, w, h);
		slider.style = style;
		slider.slider_value = (*value - min) / (max - min);

		bool focused = false;

		// Check state
		if (input.unused) {

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
					input.unused = false;
				}
			}
			else if (input_state == InputState_Pressed && mouse_in_bounds(gui, slider.bounds)) {

				set_focus(gui, GuiWidgetType_Slider, index, id);
				input.unused = false;
			}
		}

		return focused;
	}

	void gui_text(GUI* gui_, const char* text, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiTextStyle& style)
	{
		PARSE_GUI();

		GuiLabel& label = gui.labels[create_widget(gui, GuiWidgetType_Label)];
		label.bounds = compute_widget_bounds(gui, x, y, w, h);
		label.style = style;
		label.text = text;
	}

	bool gui_checkbox(GUI* gui_, bool* value, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiCheckBoxStyle& style)
	{
		PARSE_GUI();

		GuiCheckBox& cb = gui.checkboxes[create_widget(gui, GuiWidgetType_Checkbox)];
		cb.bounds = compute_widget_bounds(gui, x, y, w, h);
		cb.style = style;
		cb.active = *value;

		bool pressed = false;

		// Check state
		if (input.unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			// Check if is presssed
			if (input_state == InputState_Released && mouse_in_bounds(gui, cb.bounds)) {

				*value = !*value;
				cb.active = *value;
				input.unused = false;
				pressed = true;
			}
		}

		return pressed;
	}

	bool gui_checkbox_id(GUI* gui_, u64 id, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiCheckBoxStyle& style)
	{
		PARSE_GUI();

		GuiCheckBoxState* state;

		auto it = gui.checkbox_state.find(id);
		if (it == gui.checkbox_state.end()) {

			gui.checkbox_state[id] = {};
			state = &gui.checkbox_state[id];
		}
		else state = &it->second;

		gui_checkbox(gui_, &state->active, x, y, w, h, style);

		return state->active;
	}

	bool gui_drag_f32(GUI* gui_, f32* value, f32 adv, u64 id, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiDragStyle& style)
	{
		PARSE_GUI();

		u32 index = create_widget(gui, GuiWidgetType_Drag);
		GuiDrag& drag = gui.drags[index];
		drag.bounds = compute_widget_bounds(gui, x, y, w, h);
		drag.style = style;
		drag.value = *value;

		bool focused = false;

		// Check state
		if (input.unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			if (gui.focus.type == GuiWidgetType_Drag && gui.focus.id == id) {

				if (input_state == InputState_None || input_state == InputState_Released) {

					free_focus(gui);
				}
				else {

					*value += input.mouse_dragged.x * gui.resolution.x * adv;
					
					focused = true;
					update_focus(gui, index);
					input.unused = false;
				}
			}
			else if (input_state == InputState_Pressed && mouse_in_bounds(gui, slider.bounds)) {

				set_focus(gui, GuiWidgetType_Drag, index, id);
				input.unused = false;
			}
		}

		// TODO: limit value
		drag.value = *value;

		return focused;
	}

	///////////////////////////////////// RENDERING ///////////////////////////////////////

	void gui_draw(GUI* gui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_GUI();

		// TODO: move on
		GPUImage* depthstencil = engine.scene->depthstencil;

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

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);

				if (button.text.size()) {

					f32 font_size = size.y;

					draw_text(offscreen, button.text.c_str(), button.text.size()
						, pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - font_opensans.vertical_offset * font_size, size.x, 1u, font_size, gui.aspect, TextAlignment_Center, &font_opensans, cmd);
				}					
			}
			break;

			case GuiWidgetType_Container:
			{
				begin_debug_batch(cmd);

				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiContainer& container = gui.containers[w.index];
				pos = v2_f32{ container.bounds.x, container.bounds.y } *2.f - 1.f;
				size = v2_f32{ container.bounds.z, container.bounds.w } *2.f;
				color = container.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);
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

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);

				if (window.title.size()) {

					f32 font_size = decoration_size.y * 0.5f;

					draw_text(offscreen, window.title.c_str(), window.title.size()
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

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);
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

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);

				if (label.text.size()) {
				
					f32 font_size = size.y;

					draw_text(offscreen, label.text.c_str(), label.text.size(), pos.x - size.x * 0.5f, pos.y + size.y * 0.5f - 
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

				end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);
			}
			break;

			}
		}
	}

}
