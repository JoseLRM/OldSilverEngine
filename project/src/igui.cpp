#include "SilverEngine/core.h"

#include "SilverEngine/gui.h"

namespace sv {

	struct IGUI_WindowInfo {

		v4_f32 bounds;
		f32 vertical_offset;
		GuiWindow* window;
		f32 yoff;

	};

	struct IGUI_WidgetInfo {

		GuiWidget* widget;

		union {

			struct {
				bool clicked;
			} button;

			struct {
				bool dragging;
				f32* value;
			} drag;

		};

	};

	struct IGUI_internal {

		GUI* gui;

		IGUI_WindowInfo* current_window = nullptr;

		std::unordered_map<const char*, IGUI_WindowInfo> window_info;
		std::unordered_map<const char*, IGUI_WidgetInfo> widget_info;

	};
#define PARSE_IGUI() sv::IGUI_internal& igui = *reinterpret_cast<sv::IGUI_internal*>(igui_);

	IGUI* igui_create()
	{
		IGUI_internal& igui = *new IGUI_internal();

		igui.gui = gui_create();

		return reinterpret_cast<IGUI*>(&igui);
	}

	void igui_destroy(IGUI* igui_)
	{
		PARSE_IGUI();

		gui_destroy(igui.gui);

		delete& igui;
	}

	void igui_begin(IGUI* igui_)
	{
		PARSE_IGUI();

		gui_clear(igui.gui);
	}

	void igui_end(IGUI* igui_, f32 width, f32 height)
	{
		PARSE_IGUI();
		
		gui_update(igui.gui, width, height);

		// Update windows
		for (auto& it : igui.window_info) {

			IGUI_WindowInfo& info = it.second;
			if (info.window) {
			
				info.bounds.x = info.window->container->x.value;
				info.bounds.y = info.window->container->y.value;
				info.bounds.z = info.window->container->w.value;
				info.bounds.w = info.window->container->h.value;
				info.vertical_offset = info.window->container->vertical_offset;

				info.window = nullptr;
			}
		}

		// Update focused widget
		GuiWidget* focus = gui_widget_focused(igui.gui);
		
		if (focus) {

			const char* id = (const char*)focus->user_id;

			auto it = igui.widget_info.find(id);
			if (it != igui.widget_info.end()) {

				IGUI_WidgetInfo& info = it->second;

				switch (info.widget->type)
				{
				case GuiWidgetType_Button:
				{
					if (input.mouse_buttons[MouseButton_Left] == InputState_Released) {
						info.button.clicked = true;
					}
				}
				break;

				case GuiWidgetType_Drag:
				{
					info.drag.dragging = true;
					*info.drag.value = reinterpret_cast<GuiDrag*>(info.widget)->value;
				}
				break;
				}
			}
		}
	}

	void igui_render(IGUI* igui_, GPUImage* offscreen, CommandList cmd)
	{
		PARSE_IGUI();

		gui_render(igui.gui, offscreen, cmd);
	}

	bool igui_begin_window(IGUI* igui_, const char* name)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window == nullptr);

		GuiWindow* window = gui_window_create(igui.gui);
		IGUI_WindowInfo* info;

		auto it = igui.window_info.find(name);
		if (it == igui.window_info.end()) {

			IGUI_WindowInfo i;
			i.bounds = { 0.5f, 0.5f, 0.1f, 0.2f };
			i.vertical_offset = 0.f;
			i.window = window;

			igui.window_info[name] = i;
			info = &igui.window_info[name];
		}
		else info = &it->second;

		info->window = window;
		info->yoff = 5.f;

		window->container->x = { info->bounds.x, GuiConstraint_Relative, GuiCoordAlignment_Center };
		window->container->y = { info->bounds.y, GuiConstraint_Relative, GuiCoordAlignment_Center };
		window->container->w = { info->bounds.z, GuiConstraint_Relative };
		window->container->h = { info->bounds.w, GuiConstraint_Relative };
		window->container->vertical_offset = info->vertical_offset;
		window->container->vertical_scroll = true;

		igui.current_window = info;

		return true;
	}

	void igui_end_window(IGUI* igui_)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		igui.current_window = nullptr;
	}

	SV_INLINE static bool get_widget_info(IGUI_internal& igui, const char* id, IGUI_WidgetInfo** pinfo)
	{
		auto it = igui.widget_info.find(id);
		if (it == igui.widget_info.end()) {

			igui.widget_info[id] = {};
			*pinfo = &igui.widget_info[id];

			return true;
		}
		else *pinfo = &it->second;

		return false;
	}

	bool igui_button(IGUI* igui_, const char* text)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		GuiButton* button = gui_button_create(igui.gui, igui.current_window->window->container);

		IGUI_WidgetInfo* info;
		if (get_widget_info(igui, text, &info)) {

			info->button.clicked = false;
		}

		info->widget = button;

		constexpr f32 BUTTON_HEIGHT = 20.f;

		button->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		button->y = { igui.current_window->yoff, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		button->w = { 0.6f, GuiConstraint_Relative };
		button->h = { BUTTON_HEIGHT, GuiConstraint_Pixel };
		button->color = Color::Red();
		button->user_id = (u64)text;
		button->text = text;

		igui.current_window->yoff += BUTTON_HEIGHT + 3.f;

		bool result = info->button.clicked;
		info->button.clicked = false;
		return result;
	}

	bool igui_drag(IGUI* igui_, const char* text, f32* n, f32 adv)
	{
		PARSE_IGUI();
		SV_ASSERT(igui.current_window);

		GuiDrag* drag = gui_drag_create(igui.gui, igui.current_window->window->container);

		IGUI_WidgetInfo* info;
		if (get_widget_info(igui, text, &info)) {

			info->drag.dragging = false;
		}

		info->widget = drag;
		info->drag.value = n;

		constexpr f32 DRAG_HEIGHT = 20.f;

		drag->x = { 0.f, GuiConstraint_Center, GuiCoordAlignment_Center };
		drag->y = { igui.current_window->yoff, GuiConstraint_Pixel, GuiCoordAlignment_InverseTop };
		drag->w = { 0.6f, GuiConstraint_Relative };
		drag->h = { DRAG_HEIGHT, GuiConstraint_Pixel };
		drag->color = Color::Red();
		drag->user_id = (u64)text;
		drag->value = *n;
		drag->advance = adv;

		igui.current_window->yoff += DRAG_HEIGHT + 3.f;

		bool result = info->drag.dragging;
		info->drag.dragging = false;
		return result;
	}

}