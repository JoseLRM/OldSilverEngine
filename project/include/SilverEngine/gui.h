#pragma once

#include "SilverEngine/graphics.h"

namespace sv {

	// ENUMS

	enum GuiConstraint : u32 {
		GuiConstraint_Relative,
		GuiConstraint_Center,
		GuiConstraint_Pixel,
		GuiConstraint_Aspect,
	};

	enum GuiWidgetType : u32 {
		GuiWidgetType_Container,
		GuiWidgetType_Button,
		GuiWidgetType_Slider,
		GuiWidgetType_TextField,
		GuiWidgetType_Checkbox,
	};

	enum GuiCoordAlignment : u32 {
		GuiCoordAlignment_Left = 0u,
		GuiCoordAlignment_Bottom = 0u,
		GuiCoordAlignment_Center = 1u,
		GuiCoordAlignment_Right = 2u,
		GuiCoordAlignment_Top = 2u,

		GuiCoordAlignment_InverseLeft = 3u,
		GuiCoordAlignment_InverseBottom = 3u,
		GuiCoordAlignment_InverseCenter = 4u,
		GuiCoordAlignment_InverseRight = 5u,
		GuiCoordAlignment_InverseTop = 5u,
	};

	enum HoverState : u32 {
		HoverState_None,
		HoverState_Enter,
		HoverState_Hover,
		HoverState_Leave,
	};

	// WIDGETS

	struct GuiContainer;

	struct GuiWidget {

		struct Coord {
			f32					value = 0.f;
			GuiConstraint		constraint = GuiConstraint_Relative;
			GuiCoordAlignment	alignment = GuiCoordAlignment_Center;
		};

		struct Dimension {
			f32				value = 0.1f;
			GuiConstraint	constraint = GuiConstraint_Relative;
		};
		
		Coord		x;
		Coord		y;
		Dimension	w;
		Dimension	h;

		u64					user_id = 0u;
		GuiContainer*		parent = nullptr;
		f32					inherited_alpha = 1.f;
		Color				color = Color::White();
		f32					rounded_radius = 0.f;

		const GuiWidgetType	type;

		GuiWidget(GuiWidgetType type) : type(type) {}
	};

	struct GuiContainer : public GuiWidget {

		std::vector<GuiWidget*> sons;

		GuiContainer() : GuiWidget(GuiWidgetType_Container) {}

	};

	struct GuiButton : public GuiWidget {

		HoverState hover_state = HoverState_None;
		std::string text;

		GuiButton() : GuiWidget(GuiWidgetType_Button) {}

	};

	struct GuiSlider : public GuiWidget {

		Color button_color = Color::Gray(100u);

		f32 min = 0.f;
		f32 max = 1.f;
		f32 value = 0.5f;

		GuiSlider() : GuiWidget(GuiWidgetType_Slider) {}

	};

	struct GuiTextField : public GuiWidget {

		std::string text;

		GuiTextField() : GuiWidget(GuiWidgetType_TextField) {}

	};

	struct GuiCheckbox : public GuiWidget {

		bool active = false;
		Color color_check = Color::Gray(200u);

		GuiCheckbox() : GuiWidget(GuiWidgetType_Checkbox) {}

	};

	// WINDOW

	struct GuiWindow {

		GuiContainer*	container;
		f32				decoration_height	= 0.025f;
		f32				outline_size		= 0.006f;
		Color			decoration_color	= Color::Gray(200u);
		Color			color				= Color::Gray(150u);

	};

	// INPUT

	struct GuiLockedInput {

		bool mouse_click = false;
		bool mouse_move = false;
		bool mouse_wheel = false;
		bool keyboard = false;

	};

	// FUNCTIONS

	SV_DEFINE_HANDLE(GUI);

	GUI*							gui_create();
	void							gui_destroy(GUI* gui);
	void							gui_update(GUI* gui, f32 width, f32 height);
	void							gui_render(GUI* gui, CommandList cmd);
	const std::vector<GuiWidget*>&	gui_root_get(GUI* gui);
	const GuiLockedInput&			gui_locked_input(GUI* gui);

	GuiWidget*	gui_widget_create(GUI* gui, GuiWidgetType widget_type, GuiContainer* container = nullptr);
	void		gui_widget_destroy(GUI* gui, GuiWidget* widget);
	GuiWidget*	gui_widget_focused(GUI* gui);

	GuiWindow*	gui_window_create(GUI* gui);
	void		gui_window_destroy(GUI* gui, GuiWindow* window);

	// HELPER FUNCTIONS

	SV_INLINE GuiContainer* gui_container_create(GUI* gui, GuiContainer* container = nullptr)
	{
		return reinterpret_cast<GuiContainer*>(gui_widget_create(gui, GuiWidgetType_Container, container));
	}

	SV_INLINE GuiButton* gui_button_create(GUI* gui, GuiContainer* container = nullptr)
	{
		return reinterpret_cast<GuiButton*>(gui_widget_create(gui, GuiWidgetType_Button, container));
	}

	SV_INLINE GuiSlider* gui_slider_create(GUI* gui, GuiContainer* container = nullptr)
	{
		return reinterpret_cast<GuiSlider*>(gui_widget_create(gui, GuiWidgetType_Slider, container));
	}

	SV_INLINE GuiTextField* gui_textfield_create(GUI* gui, GuiContainer* container = nullptr)
	{
		return reinterpret_cast<GuiTextField*>(gui_widget_create(gui, GuiWidgetType_TextField, container));
	}

	SV_INLINE GuiButton* gui_button_clicked(GUI* gui)
	{
		GuiWidget* widget = gui_widget_focused(gui);
		return (widget->type == GuiWidgetType_Button) ? reinterpret_cast<GuiButton*>(widget) : nullptr;
	}

}