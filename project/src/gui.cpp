#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"
#include "SilverEngine/utils/allocators/FrameList.h"

#include "renderer/renderer_internal.h"

#define PARSE_GUI() sv::GUI_internal& gui = *reinterpret_cast<sv::GUI_internal*>(gui_)

namespace sv {

	/*
	  TODO LIST:
	  - GuiDraw structure that defines the rendering of a widget or some component (PD: Should not contain inherited alpha)
	  - Align gui widgets
	  - Popups
	  - Combobox
	  - Not use depthstencil
	*/

	enum GuiWidgetType : u32 {
		GuiWidgetType_None,
		GuiWidgetType_Container,
		GuiWidgetType_Button,
		GuiWidgetType_Slider,
	};

	struct GuiWidgetIndex {

		GuiWidgetType type = GuiWidgetType_None;
		u32 index = 0u;

	};

	struct GuiFocus {
		u64 id = 0u;
		GuiWidgetType type = GuiWidgetType_None;
		bool updated = false;
	};

	struct GuiContainer {

		v4_f32 bounds;
		GuiContainerStyle style;
		GuiWidgetIndex parent_index;

	};

	struct GuiButton {

		v4_f32 bounds;
		GuiButtonStyle style;
		bool hot;
		bool pressed;

	};

	struct GuiSlider {

		v4_f32 bounds;
		GuiSliderStyle style;
		f32 slider_value;
		
	};

	struct GUI_internal {

		FrameList<GuiContainer> containers;
		FrameList<GuiButton> buttons;
		FrameList<GuiSlider> sliders;

		FrameList<GuiWidgetIndex> widgets;

		GuiWidgetIndex parent_index;
		GuiFocus focus;

		v2_f32 resolution;
		v2_f32 mouse_position;

	};

	void gui_begin(GUI* gui_, f32 width, f32 height)
	{
		PARSE_GUI();

		gui.containers.reset();
		gui.buttons.reset();
		gui.sliders.reset();

		gui.widgets.reset();

		gui.parent_index.type = GuiWidgetType_None;
		gui.resolution = { width, height };
		gui.mouse_position = input.mouse_position + 0.5f;
		gui.focus.updated = false;
	}

	void gui_end(GUI* gui_)
	{
		PARSE_GUI();

		if (!gui.focus.updated) 
			gui.focus.type = GuiWidgetType_None;
	}

	Result gui_create(GUI** pgui)
	{
		GUI_internal& gui = *new GUI_internal();

		*pgui = reinterpret_cast<GUI*>(&gui);
		return Result_Success;
	}

	Result gui_destroy(GUI* gui_)
	{
		PARSE_GUI();


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
			res = aspect_coord * coord.value * (vertical ? (gui.resolution.x / gui.resolution.y) : (gui.resolution.y / gui.resolution.x));
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
			res = inv_dimension * (vertical ? (gui.resolution.x / gui.resolution.y) : (gui.resolution.y / gui.resolution.x));
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

	SV_INLINE static void add_widget(GUI_internal& gui, u32 index, GuiWidgetType type)
	{
		GuiWidgetIndex& i = gui.widgets.emplace_back();
		i.index = index;
		i.type = type;

		if (type == GuiWidgetType_Container) {

			gui.parent_index.type = type;
			gui.parent_index.index = index;
		}
	}

	void gui_begin_container(GUI* gui_, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiContainerStyle& style)
	{
		PARSE_GUI();

		u32 index = u32(gui.containers.size());

		GuiContainer& container = gui.containers.emplace_back();
		container.style = style;
		container.bounds = compute_widget_bounds(gui, x, y, w, h);
		container.parent_index = gui.parent_index;

		add_widget(gui, index, GuiWidgetType_Container);
	}

	void gui_end_container(GUI* gui_)
	{
		PARSE_GUI();
		SV_ASSERT(gui.parent_index.type != GuiWidgetType_None);

		switch (gui.parent_index.type)
		{
		case GuiWidgetType_Container:
			gui.parent_index = gui.containers[gui.parent_index.index].parent_index;
			break;
		}
	}

	bool gui_button(GUI* gui_, const char* text, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiButtonStyle& style)
	{
		PARSE_GUI();

		u32 index = u32(gui.buttons.size());

		GuiButton& button = gui.buttons.emplace_back();
		button.bounds = compute_widget_bounds(gui, x, y, w, h);
		button.style = style;
		button.hot = false;
		button.pressed = false;
		
		add_widget(gui, index, GuiWidgetType_Button);

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

		u32 index = u32(gui.sliders.size());

		GuiSlider& slider = gui.sliders.emplace_back();
		slider.bounds = compute_widget_bounds(gui, x, y, w, h);
		slider.style = style;
		slider.slider_value = (*value - min) / (max - min);

		add_widget(gui, index, GuiWidgetType_Slider);

		bool focused = false;

		// Check state
		if (input.unused) {

			InputState input_state = input.mouse_buttons[MouseButton_Left];

			if (gui.focus.type == GuiWidgetType_Slider && gui.focus.id == id) {

				if (input_state == InputState_None || input_state == InputState_Released) {

					gui.focus.type = GuiWidgetType_None;
				}
				else {

					f32 slider_min_pos = slider.bounds.x - slider.bounds.z * 0.5f;
					
					f32 mouse_value = (gui.mouse_position.x - slider_min_pos) / slider.bounds.z;
					slider.slider_value = mouse_value;
					*value = mouse_value * (max - min) + min;

					focused = true;
					gui.focus.updated = true;
				}
			}
			else if (input_state == InputState_Pressed && mouse_in_bounds(gui, slider.bounds)) {

				gui.focus.id = id;
				gui.focus.type = GuiWidgetType_Slider;
				gui.focus.updated = true;
			}
		}

		return focused;
	}

	///////////////////////////////////// RENDERING ///////////////////////////////////////

	void gui_draw(GUI* gui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_GUI();

		begin_debug_batch(cmd);

		for (GuiWidgetIndex& w : gui.widgets) {

			switch (w.type)
			{
			case GuiWidgetType_Button:
			{
				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiButton& button = gui.buttons[w.index];
				pos = v2_f32{ button.bounds.x, button.bounds.y } * 2.f - 1.f;
				size = v2_f32{ button.bounds.z, button.bounds.w } * 2.f;
				color = button.hot ? button.style.hot_color : button.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);
			}
			break;

			case GuiWidgetType_Container:
			{
				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiContainer& container = gui.containers[w.index];
				pos = v2_f32{ container.bounds.x, container.bounds.y } *2.f - 1.f;
				size = v2_f32{ container.bounds.z, container.bounds.w } *2.f;
				color = container.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);
			}
			break;

			case GuiWidgetType_Slider:
			{
				v2_f32 pos;
				v2_f32 size;
				Color color;

				GuiSlider& slider = gui.sliders[w.index];
				pos = v2_f32{ slider.bounds.x, slider.bounds.y } *2.f - 1.f;
				size = v2_f32{ slider.bounds.z, slider.bounds.w } *2.f;
				color = slider.style.color;

				draw_debug_quad(pos.getVec3(0.f), size, color, cmd);
			}
			break;

			}
		}

		// TODO: move on
		GPUImage* depthstencil = engine.scene->depthstencil;

		end_debug_batch(offscreen, depthstencil, XMMatrixIdentity(), cmd);
	}

}