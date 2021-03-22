#pragma once

#include "SilverEngine/graphics.h"
#include "SilverEngine/font.h"

namespace sv {

	enum GuiConstraint : u8 {
		GuiConstraint_Relative,
		GuiConstraint_Absolute,
		GuiConstraint_InverseAbsolute,
		GuiConstraint_Center,
		GuiConstraint_Pixel,
		GuiConstraint_InversePixel,
		GuiConstraint_Aspect,
	};

	struct GuiCoord {

		f32 value;
		GuiConstraint constraint;

		SV_INLINE static GuiCoord Center() { return { 0.f, GuiConstraint_Center }; }
		SV_INLINE static GuiCoord Relative(f32 n) { return { n, GuiConstraint_Relative }; }
		SV_INLINE static GuiCoord Absolute(f32 n) { return { n, GuiConstraint_Absolute }; }
		SV_INLINE static GuiCoord IRelative(f32 n) { return { 1.f - n, GuiConstraint_Relative }; }
		SV_INLINE static GuiCoord IAbsolute(f32 n) { return { n, GuiConstraint_InverseAbsolute }; }
		SV_INLINE static GuiCoord Pixel(f32 n) { return { n, GuiConstraint_Pixel }; }
		SV_INLINE static GuiCoord IPixel(f32 n) { return { n, GuiConstraint_InversePixel }; }
		SV_INLINE static GuiCoord Aspect(f32 n = 1.f) { return { n, GuiConstraint_Aspect }; }
		
	};


	enum GuiBoxType : u32 {
		GuiBoxType_Quad,
		GuiBoxType_Triangle,
	};

	struct GuiBox {

		GuiBoxType type;
		f32 mult;

		union {

			struct {
				Color color;
			} quad;

			struct {
				Color color;
				bool down;
			} triangle;

		};

		SV_INLINE static GuiBox Quad(Color color, f32 mult = 0.7f) 
		{ 
			GuiBox box; 
			box.type = GuiBoxType_Quad; 
			box.mult = mult;
			box.quad.color = color; 
			return box; 
		}

		SV_INLINE static GuiBox Triangle(Color color, bool down = false, f32 mult = 0.7f)
		{
			GuiBox box;
			box.type = GuiBoxType_Triangle;
			box.mult = mult;
			box.triangle.color = color;
			box.triangle.down = down;
			return box;
		}

	};

	enum GuiPopupTrigger : u32 {
		GuiPopupTrigger_Custom,
		GuiPopupTrigger_Parent,
		GuiPopupTrigger_LastWidget,
	};

	SV_DEFINE_HANDLE(GUI);

	struct GuiContainerStyle {
		Color color = Color::Gray(100u);
	};

	struct GuiWindowStyle {
		Color color = Color::Gray(170u, 20u);
		Color decoration_color = Color::Gray(100u, 200u);
		Color outline_color = Color::Gray(50u, 200u);
		f32 decoration_height = 0.04f;
		f32 outline_size = 0.001f;
		f32 min_width = 0.1f;
		f32 min_height = 0.f;
	};

	struct GuiPopupStyle {
		Color background_color = Color::Gray(140u);
	};

	struct GuiButtonStyle {
		Color color = Color::Gray(200u, 150u);
		Color hot_color = Color::White(230u);
		Color text_color = Color::Black();
	};

	struct GuiSliderStyle {
		Color color = Color::Gray(5u);
		Color button_color = Color::White();
		v2_f32 button_size = { 0.01f, 0.03f };
	};

	struct GuiLabelStyle {
		Color text_color = Color::White();
		TextAlignment text_alignment = TextAlignment_Center;
		Color background_color = Color::Gray(100u, 0u);
	};

	struct GuiCheckboxStyle {
		Color color = Color::White();
		GuiBox active_box = GuiBox::Quad(Color::Black());
		GuiBox inactive_box = GuiBox::Quad(Color::White(0u));
	};

	struct GuiDragStyle {
		Color text_color = Color::Black();
		Color background_color = Color::White();
	};

	struct GuiMenuItemStyle {
		Color color = Color::White();
		Color hot_color = Color::Gray(200u);
		Color text_color = Color::Black();
	};

	Result gui_create(u64 hashcode, GUI** pgui);
	Result gui_destroy(GUI* gui);

	void gui_begin(GUI* gui, f32 width, f32 height);
	void gui_end(GUI* gui);

	void gui_push_id(GUI* gui, u64 id);
	void gui_push_id(GUI* gui, const char* id);
	void gui_pop_id(GUI* gui, u32 count = 1u);

	void gui_begin_container(GUI* gui, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiContainerStyle& style = {});
	void gui_end_container(GUI* gui);
	
	bool gui_begin_popup(GUI* gui, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, const GuiPopupStyle& style = {});
	void gui_end_popup(GUI* gui);
	
	bool gui_begin_window(GUI* gui, const char* title, const GuiWindowStyle& style = {});
	void gui_end_window(GUI* gui);
	Result gui_show_window(GUI* gui, const char* title);
	Result gui_hide_window(GUI* gui, const char* title);

	bool gui_begin_menu_item(GUI* gui, const char* text, u64 id, const GuiMenuItemStyle& style = {});
	void gui_end_menu_item(GUI* gui);

	void gui_send_package(GUI* gui, const void* package, u32 package_size, u64 id);
	bool gui_recive_package(GUI* gui, void** package, u32* package_size, u64 sender_id, u64 id);

	bool gui_button(GUI* gui, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiButtonStyle& style = {});
	
	bool gui_drag_f32(GUI* gui, f32* value, f32 adv, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style = {});
	SV_INLINE bool gui_drag_f32(GUI* gui, f32* value, f32 adv, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style = {})
	{
		gui_drag_f32(gui, value, adv, f32_min, f32_max, id, x0, x1, y0, y1, style);
	}
	
	bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiSliderStyle& style = {});
	void gui_text(GUI* gui, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiLabelStyle& style = {});
	bool gui_checkbox(GUI* gui, bool* value, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckboxStyle& style = {});
	bool gui_checkbox(GUI* gui, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckboxStyle& style = {});
	
	v4_f32 gui_parent_bounds(GUI* gui);

	void gui_draw(GUI* gui, CommandList cmd);

	// High level

	void gui_begin_grid(GUI* gui, u32 element_count, f32 element_size, f32 padding, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1);
	void gui_begin_grid_element(GUI* gui, u64 id);
	void gui_end_grid_element(GUI* gui);
	void gui_end_grid(GUI* gui);

}
