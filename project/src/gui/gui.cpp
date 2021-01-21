#include "SilverEngine/core.h"

#include "gui_internal.h"

namespace sv {

	GUI* gui_create(u32 width, u32 height)
	{
		GUI_internal& gui = *new GUI_internal();
		gui.resolution = { f32(width), f32(height) };

		gui.debug.create();

		return reinterpret_cast<GUI*>(&gui);
	}

	void gui_destroy(GUI* gui_)
	{
		PARSE_GUI();

		gui.containers.clear();
		gui.buttons.clear();
		gui.sliders.clear();

		gui.windows.clear();

		gui.root.clear();

		gui.debug.destroy();

		delete& gui;
	}

	void gui_resize(GUI* gui_, u32 width, u32 height)
	{
		PARSE_GUI();
		gui.resolution = { f32(width), f32(height) };
	}

	// BOUNDS FUNCTIONS

	SV_INLINE static f32 compute_coord(const GUI_internal& gui, const GuiWidget::Coord& coord, f32 aspect_coord, f32 dimension, f32 resolution, f32 aspect)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (coord.constraint)
		{
		case GuiConstraint_Relative:
			res = coord.value;
			break;

		case GuiConstraint_Center:
			res = 0.5f;
			break;

		case GuiConstraint_Pixel:
			res = coord.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = aspect_coord * coord.value * aspect;
			break;

		}

		// Align coord
		if (coord.alignment >= 3u) {
			res = 1.f - res;
		}

		switch (coord.alignment)
		{
		case GuiCoordAlignment_Left:
		case GuiCoordAlignment_InverseLeft:
			res = res + dimension * 0.5f;
			break;

		case GuiCoordAlignment_Right:
		case GuiCoordAlignment_InverseRight:
			res = res - dimension * 0.5f;
			break;
		}

		return res;
	}

	SV_INLINE static f32 compute_dimension(const GUI_internal& gui, const GuiWidget::Dimension& dimension, f32 inv_dimension, f32 resolution, f32 aspect)
	{
		f32 res = 0.5f;

		// Centred coord
		switch (dimension.constraint)
		{
		case GuiConstraint_Relative:
			res = dimension.value;
			break;

		case GuiConstraint_Center:
			res = 0.5f;
			break;

		case GuiConstraint_Pixel:
			res = dimension.value / resolution;
			break;

		case GuiConstraint_Aspect:
			res = inv_dimension * dimension.value * aspect;
			break;
		}

		return res;
	}

	SV_INLINE bool mouse_in_bounds(const GUI_internal& gui, const v4_f32& bounds)
	{
		return abs(bounds.x - gui.mouse_position.x) <= bounds.z * 0.5f && abs(bounds.y - gui.mouse_position.y) <= bounds.w * 0.5f;
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiWidget& widget, const v4_f32& parent_bounds)
	{
#ifndef SV_DIST
		if (widget.x.constraint == GuiConstraint_Aspect && widget.x.constraint == widget.y.constraint) {
			SV_LOG_WARNING("Gui coords has both aspect constraint, that's not possible...");
		}
		if (widget.w.constraint == GuiConstraint_Aspect && widget.w.constraint == widget.h.constraint) {
			SV_LOG_WARNING("Gui dimensions has both aspect constraint, that's not possible...");
		}
#endif

		// Compute local bounds
		f32 w;
		f32 h;
		f32 x;
		f32 y;

		f32 vaspect = (gui.resolution.x * parent_bounds.z) / (gui.resolution.y * parent_bounds.w);
		f32 haspect = 1.f / vaspect;

		if (widget.w.constraint == GuiConstraint_Aspect) {
			h = compute_dimension(gui, widget.h, 0.5f, gui.resolution.y, vaspect);
			w = compute_dimension(gui, widget.w, h, gui.resolution.x, haspect);
		}
		else {
			w = compute_dimension(gui, widget.w, 0.5f, gui.resolution.x, haspect);
			h = compute_dimension(gui, widget.h, w, gui.resolution.y, vaspect);
		}

		if (widget.x.constraint == GuiConstraint_Aspect) {
			y = compute_coord(gui, widget.y, 0.5f, h, gui.resolution.y, vaspect);
			x = compute_coord(gui, widget.x, y, w, gui.resolution.x, haspect);
		}
		else {
			x = compute_coord(gui, widget.x, 0.5f, w, gui.resolution.x, haspect);
			y = compute_coord(gui, widget.y, x, h, gui.resolution.y, vaspect);
		}

		// Adjust to parent bounds
		if (widget.parent) {

			x = (parent_bounds.x - parent_bounds.z * 0.5f) + (x * parent_bounds.z);
			y = (parent_bounds.y - parent_bounds.w * 0.5f) + (y * parent_bounds.w);
			w *= parent_bounds.z;
			h *= parent_bounds.w;
		}

		return { x, y, w, h };
	}

	static v4_f32 compute_widget_bounds(const GUI_internal& gui, GuiWidget& widget)
	{
		v4_f32 parent_bounds = { 0.f, 0.f, 1.f, 1.f };

		if (widget.parent != nullptr) {
			parent_bounds = compute_widget_bounds(gui, *widget.parent);
		}

		return compute_widget_bounds(gui, widget, parent_bounds);
	}

	SV_INLINE static v4_f32 compute_window_decoration_bounds(const GUI_internal& gui, GuiWindow& window, const v4_f32& container_bounds)
	{
		f32 aspect = gui.resolution.x / gui.resolution.y;

		v4_f32 decoration_bounds;
		decoration_bounds.x = container_bounds.x;
		decoration_bounds.y = container_bounds.y + container_bounds.w * 0.5f + (window.outline_size + window.decoration_height * 0.5f) * aspect;
		decoration_bounds.z = container_bounds.z + window.outline_size * 2.f;
		decoration_bounds.w = window.decoration_height * aspect;
		return decoration_bounds;
	}

	SV_INLINE static v4_f32 compute_window_bounds(const GUI_internal& gui, GuiWindow& window, const v4_f32& container_bounds)
	{
		f32 aspect = gui.resolution.x / gui.resolution.y;

		v4_f32 bounds;
		bounds.x = container_bounds.x;
		bounds.y = container_bounds.y;
		bounds.z = container_bounds.z + window.outline_size * 2.f;
		bounds.w = container_bounds.w + (window.outline_size * 2.f) * aspect;
		return bounds;
	}

	// UPDATE

	static void update_window(GUI_internal& gui, GuiWindow& window)
	{
		v4_f32 container_bounds = compute_widget_bounds(gui, *window.container);
		v4_f32 decoration_bounds = compute_window_decoration_bounds(gui, window, container_bounds);
		
		if (mouse_in_bounds(gui, decoration_bounds)) {

			gui.widget_hovered = window.container;

			if (gui.mouse_clicked) {

				gui.widget_clicked = window.container;

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Pressed) {

					gui.widget_focused = window.container;
					gui.dragged_begin_pos = gui.mouse_position - v2_f32{ container_bounds.x, container_bounds.y };
					gui.dragged_action_id = 0u;
				}
			}
		}
		else {

			v4_f32 bounds = compute_window_bounds(gui, window, container_bounds);
			if (mouse_in_bounds(gui, bounds) && !mouse_in_bounds(gui, container_bounds)) {

				gui.widget_clicked = window.container;

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Pressed) {

					gui.widget_focused = window.container;

					bool vertical = abs(gui.mouse_position.x - container_bounds.x) < container_bounds.z * 0.5f;
					bool horizontal = abs(gui.mouse_position.y - container_bounds.y) < container_bounds.w * 0.5f;

					bool top = gui.mouse_position.y > container_bounds.y + container_bounds.w * 0.5f;
					bool right = gui.mouse_position.x > container_bounds.x + container_bounds.z * 0.5f;

					if (vertical) {
						if (top) {
							gui.dragged_begin_pos.y = container_bounds.y - container_bounds.w * 0.5f;
							gui.dragged_action_id = 1u;
						}
						else {
							gui.dragged_begin_pos.y = container_bounds.y + container_bounds.w * 0.5f;
							gui.dragged_action_id = 2u;
						}
					}
					else if (horizontal) {
						if (right) {
							gui.dragged_begin_pos.x = container_bounds.x - container_bounds.z * 0.5f;
							gui.dragged_action_id = 3u;
						}
						else {
							gui.dragged_begin_pos.x = container_bounds.x + container_bounds.z * 0.5f;
							gui.dragged_action_id = 4u;
						}
					}
					else {

						if (top) {
							gui.dragged_begin_pos.y = container_bounds.y - container_bounds.w * 0.5f;
						}
						else {
							gui.dragged_begin_pos.y = container_bounds.y + container_bounds.w * 0.5f;
						}

						if (right) {
							gui.dragged_begin_pos.x = container_bounds.x - container_bounds.z * 0.5f;
						}
						else {
							gui.dragged_begin_pos.x = container_bounds.x + container_bounds.z * 0.5f;
						}

						if (top && !right) {
							gui.dragged_action_id = 5u;
						}
						else if (top && right) {
							gui.dragged_action_id = 6u;
						}
						else if (!top && !right) {
							gui.dragged_action_id = 7u;
						}
						else if (!top && right) {
							gui.dragged_action_id = 8u;
						}
					}
				}
			}
		}
	}

	static void update_widget(GUI_internal& gui, GuiWidget& widget, const v4_f32& parent_bounds)
	{
		switch (widget.type)
		{
		case GuiWidgetType_Container:
		{
			GuiContainer& container = *reinterpret_cast<GuiContainer*>(&widget);
			v4_f32 bounds = compute_widget_bounds(gui, container, parent_bounds);

			for (GuiWidget* son : container.sons) {
				update_widget(gui, *son, bounds);
			}
		} 
		break;

		case GuiWidgetType_Button:
		{
			GuiButton& button = *reinterpret_cast<GuiButton*>(&widget);
			v4_f32 bounds = compute_widget_bounds(gui, button, parent_bounds);

			// Mouse in button
			if (mouse_in_bounds(gui, bounds)) {
				
				switch (button.hover_state)
				{
				case HoverState_None:
				case HoverState_Leave:
					button.hover_state = HoverState_Enter;
					break;

				case HoverState_Enter:
					button.hover_state = HoverState_Hover;
					break;

				}

				gui.widget_hovered = &widget;

				if (gui.mouse_clicked)
					gui.widget_clicked = &widget;
			}
			else {

				if (button.hover_state == HoverState_Enter || button.hover_state == HoverState_Hover) {

					button.hover_state = HoverState_Leave;
				}
			}
		}
		break;

		case GuiWidgetType_Slider:
		{
			GuiSlider& slider = *reinterpret_cast<GuiSlider*>(&widget);
			v4_f32 bounds = compute_widget_bounds(gui, slider, parent_bounds);

			// Mouse in button
			if (mouse_in_bounds(gui, bounds)) {

				gui.widget_hovered = &widget;

				if (gui.mouse_clicked) {

					gui.widget_clicked = &widget;

					InputState mouse_state = input.mouse_buttons[MouseButton_Left];

					if (mouse_state == InputState_Pressed) {

						gui.widget_focused = &widget;
					}
				}
			}
		}
		break;

		}
	}

	void gui_update(GUI* gui_)
	{
		PARSE_GUI();

		// Init update
		gui.mouse_position = input.mouse_position + 0.5f;
		gui.widget_clicked = nullptr;
		gui.widget_hovered = nullptr;
		gui.mouse_clicked = false;
		for (MouseButton b = MouseButton(0); b < MouseButton_MaxEnum; ++(u32&)b) 
			if (input.mouse_buttons[b] != InputState_None) {
				gui.mouse_clicked = true;
				break;
			}

		// Update windows and widgets
		for (auto& pool : gui.windows) {
			for (GuiWindow& window : pool) {
				update_window(gui, window);
			}
		}
		for (GuiWidget* root : gui.root) {
			update_widget(gui, *root, { 0.f, 0.f, 1.f, 1.f });
		}

		// Save the last hovered to know if it is leaved
		{
			GuiWidget* temp = gui.widget_last_hovered;
			gui.widget_last_hovered = gui.widget_hovered;
			if (temp) {
				gui.widget_hovered = temp;
			}
		}

		// Set the locked data and update focused widget
		gui.locked.keyboard = false;

		if (gui.widget_focused != nullptr) {

			gui.locked.keyboard = false;

			switch (gui.widget_focused->type)
			{

				// A container can't be focused, so it should be a window
			case GuiWidgetType_Container:
			{
				GuiContainer& container = *reinterpret_cast<GuiContainer*>(gui.widget_focused);

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Released || mouse_state == InputState_None) {
					gui.widget_focused = nullptr;
				}
				else {

					v2_f32 move = gui.mouse_position - gui.dragged_begin_pos;

					container.x.constraint = GuiConstraint_Relative;
					container.x.alignment = GuiCoordAlignment_Center;
					container.y.constraint = GuiConstraint_Relative;
					container.y.alignment = GuiCoordAlignment_Center;

					switch (gui.dragged_action_id)
					{
					case 0:
						container.x.value = move.x;
						container.y.value = move.y;
						break;

					// RESIZE SIDES
					case 1: // top
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
					}
					break;

					case 2: // bottom
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
					}
					break;

					case 3: // right
					{
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					case 4: // left
					{
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					// RESIZE CORNERS
					case 5: // top - left
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					case 6: // top - right
					{
						move.y = std::max(0.0f, move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y + move.y * 0.5f;
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					case 7: // bottom - left
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
						move.x = std::max(0.05f, -move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x - move.x * 0.5f;
					}
					break;

					case 8: // bottom - right
					{
						move.y = std::max(0.0f, -move.y);
						container.h.value = move.y;
						container.y.value = gui.dragged_begin_pos.y - move.y * 0.5f;
						move.x = std::max(0.05f, move.x);
						container.w.value = move.x;
						container.x.value = gui.dragged_begin_pos.x + move.x * 0.5f;
					}
					break;

					}
				}
			}
			break;

			case GuiWidgetType_Slider:
			{
				GuiSlider& slider = *reinterpret_cast<GuiSlider*>(gui.widget_focused);

				InputState mouse_state = input.mouse_buttons[MouseButton_Left];

				if (mouse_state == InputState_Released || mouse_state == InputState_None) {
					gui.widget_focused = nullptr;
				}
				else {

					v4_f32 bounds = compute_widget_bounds(gui, slider);
					f32 min_pos = bounds.x - bounds.z * 0.5f;

					f32 normalized_value = (gui.mouse_position.x - min_pos) / bounds.z;
					
					slider.value = slider.min + normalized_value * (slider.max - slider.min);
					slider.value = std::max(slider.min, std::min(slider.value, slider.max));
				}
			}
			break;

			}
		}


		if (gui.mouse_clicked) {
			gui.locked.mouse_click = gui.widget_clicked != nullptr;
		}
	}

	void draw_widget(GUI_internal& gui, GuiWidget& widget, const v4_f32& parent_bounds)
	{
		v4_f32 bounds = compute_widget_bounds(gui, widget, parent_bounds);

		v2_f32 pos = v2_f32{ bounds.x, bounds.y } * 2.f - 1.f;
		v2_f32 size = v2_f32{ bounds.z, bounds.w } * 2.f;

		switch (widget.type)
		{

		case GuiWidgetType_Container:
		{
			GuiContainer& container = *reinterpret_cast<GuiContainer*>(&widget);
			gui.debug.drawQuad(pos.getVec3(), size, widget.color);

			for (GuiWidget* son : container.sons) {
				draw_widget(gui, *son, bounds);
			}
		}
		break;

		case GuiWidgetType_Slider:
		{
			GuiSlider& slider = *reinterpret_cast<GuiSlider*>(&widget);
			
			f32 normalized_value = (slider.value - slider.min) / (slider.max - slider.min);
			f32 subpos = pos.x - size.x * 0.5f + normalized_value * size.x;

			gui.debug.drawQuad(pos.getVec3(), size, widget.color);
			gui.debug.drawQuad({ subpos, pos.y, 0.f }, { size.x * 0.05f, size.y * 1.1f }, slider.button_color);
		}
		break;

		default:
			gui.debug.drawQuad(pos.getVec3(), size, widget.color);
			break;
		}
	}

	void gui_render(GUI* gui_, GPUImage* rendertarget, CommandList cmd)
	{
		PARSE_GUI();

		gui.debug.reset();

		// Draw windows
		for (auto& pool : gui.windows) {
			for (GuiWindow& window : pool) {

				v4_f32 container_bounds = compute_widget_bounds(gui, *window.container);
				v4_f32 bounds = compute_window_bounds(gui, window, container_bounds);
				v4_f32 decoration_bounds = compute_window_decoration_bounds(gui, window, container_bounds);

				v2_f32 pos = v2_f32{ decoration_bounds.x, decoration_bounds.y };
				pos = pos * 2.f - 1.f;

				v2_f32 size = v2_f32{ decoration_bounds.z, decoration_bounds.w };
				size = size * 2.f;

				gui.debug.drawQuad(pos.getVec3(), size, window.decoration_color);

				pos = v2_f32{ bounds.x, bounds.y };
				pos = pos * 2.f - 1.f;

				size = v2_f32{ bounds.z, bounds.w };
				size = size * 2.f;

				gui.debug.drawQuad(pos.getVec3(), size, window.color);
			}
		}

		// Draw widgets

		for (GuiWidget* root : gui.root) {
			draw_widget(gui, *root, { 0.f, 0.f, 1.f, 1.f });
		}

		gui.debug.render(rendertarget, XMMatrixIdentity(), cmd);
	}

	GuiWidget* gui_widget_create(GUI* gui_, GuiWidgetType widget_type, GuiContainer* container)
	{
		PARSE_GUI();

		GuiWidget* widget;

		// Allocate widget
		switch (widget_type)
		{
		case GuiWidgetType_Container:
			widget = &gui.containers.create();
			break;

		case GuiWidgetType_Button:
			widget = &gui.buttons.create();
			break;

		case GuiWidgetType_Slider:
			widget = &gui.sliders.create();
			break;

		default:
			SV_LOG_ERROR("Unknown widget type: %u", widget_type);
			return nullptr;
		}

		// Set parent
		if (container)
			container->sons.push_back(widget);
		else
			gui.root.push_back(widget);
		widget->parent = container;


		return widget;
	}

	void gui_widget_destroy(GUI* gui_, GuiWidget* widget)
	{
		if (widget == nullptr) return;
		PARSE_GUI();

		// Remove son reference from the parent
		if (widget->parent) {

			GuiContainer& parent = *widget->parent;
			
			for (auto it = parent.sons.begin(); it != parent.sons.end(); ++it) {
				if (*it == widget) {
					parent.sons.erase(it);
					break;
				}
			}
		}

		// Deallocate widget
		switch (widget->type)
		{
		case GuiWidgetType_Container:
			gui.containers.destroy(*reinterpret_cast<GuiContainer*>(widget));
			break;

		case GuiWidgetType_Button:
			gui.buttons.destroy(*reinterpret_cast<GuiButton*>(widget));
			break;

		case GuiWidgetType_Slider:
			gui.sliders.destroy(*reinterpret_cast<GuiSlider*>(widget));
			break;
		}
	}

	GuiWidget* gui_widget_clicked(GUI* gui_)
	{
		PARSE_GUI();
		return gui.widget_clicked;
	}

	GuiWidget* gui_widget_hovered(GUI* gui_)
	{
		PARSE_GUI();
		return gui.widget_hovered;
	}

	GuiWidget* gui_widget_focused(GUI* gui_)
	{
		PARSE_GUI();
		return gui.widget_focused;
	}

	GuiWindow* gui_window_create(GUI* gui_)
	{
		PARSE_GUI();
		GuiWindow& window = gui.windows.create();

		window.container = &gui_container_create(gui_);

		window.container->x.value = 0.f;
		window.container->x.constraint = GuiConstraint_Relative;
		window.container->x.alignment = GuiCoordAlignment_Center;
		window.container->y.value = 0.f;
		window.container->y.constraint = GuiConstraint_Relative;
		window.container->y.alignment = GuiCoordAlignment_Center;
		window.container->w.value = 0.2f;
		window.container->w.constraint = GuiConstraint_Relative;
		window.container->h.value = 0.4f;
		window.container->h.constraint = GuiConstraint_Relative;

		return &window;
	}

	void gui_window_destroy(GUI* gui_, GuiWindow* window)
	{
		PARSE_GUI();

		gui_widget_destroy(gui_, window->container);
		gui.windows.destroy(*window);
	}

	const std::vector<GuiWidget*>& gui_root_get(GUI* gui_)
	{
		PARSE_GUI();
		return gui.root;
	}

	const GuiLockedInput& gui_locked_input(GUI* gui_)
	{
		PARSE_GUI();
		return gui.locked;
	}

}