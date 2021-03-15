#pragma once

#include "SilverEngine/graphics.h"
#include "SilverEngine/font.h"

namespace sv {

	enum GuiConstraint : u32 {
		GuiConstraint_Relative,
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
		SV_INLINE static GuiCoord IRelative(f32 n) { return { 1.f - n, GuiConstraint_Relative }; }
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
		Color color = Color::Gray(100u);
		Color decoration_color = Color::Black();
		Color outline_color = Color::White();
		f32 decoration_height = 0.04f;
		f32 outline_size = 0.001f;
		f32 min_width = 0.1f;
		f32 min_height = 0.f;
	};

	struct GuiPopupStyle {
		Color background_color = Color::White(80u);
	};

	struct GuiButtonStyle {
		Color color = Color::Gray(200u);
		Color hot_color = Color::White();
		Color text_color = Color::Black();
	};

	struct GuiSliderStyle {
		Color color = Color::Gray(5u);
		Color button_color = Color::White();
		v2_f32 button_size = { 0.01f, 0.03f };
	};

	struct GuiTextStyle {
		Color text_color = Color::White();
		TextAlignment text_alignment = TextAlignment_Center;
		Color background_color = Color::Gray(100u, 0u);
	};

	struct GuiCheckBoxStyle {
		Color color = Color::White();
		GuiBox active_box = GuiBox::Quad(Color::Black());
		GuiBox inactive_box = GuiBox::Quad(Color::White(0u));
	};

	struct GuiDragStyle {
		Color text_color = Color::Black();
		Color background_color = Color::White();
	};

	Result gui_create(u64 hashcode, GUI** pgui);
	Result gui_destroy(GUI* gui);

	void gui_begin(GUI* gui, f32 width, f32 height);
	void gui_end(GUI* gui);

	void gui_push_id(GUI* gui, u64 id);
	void gui_push_id(GUI* gui, const char* id);
	void gui_pop_id(GUI* gui);

	void gui_begin_container(GUI* gui, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiContainerStyle& style = {});
	void gui_end_container(GUI* gui);
	
	bool gui_begin_popup(GUI* gui, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, const GuiPopupStyle& style = {});
	void gui_end_popup(GUI* gui);
	
	bool gui_begin_window(GUI* gui, const char* title, const GuiWindowStyle& style = {});
	void gui_end_window(GUI* gui);
	Result gui_show_window(GUI* gui, const char* title);
	Result gui_hide_window(GUI* gui, const char* title);

	bool gui_button(GUI* gui, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiButtonStyle& style = {});
	bool gui_drag_f32(GUI* gui, f32* value, f32 adv, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiDragStyle& style = {});
	bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiSliderStyle& style = {});
	void gui_text(GUI* gui, const char* text, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiTextStyle& style = {});
	bool gui_checkbox(GUI* gui, bool* value, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckBoxStyle& style = {});
	bool gui_checkbox(GUI* gui, u64 id, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1, const GuiCheckBoxStyle& style = {});
	
	void gui_draw(GUI* gui, CommandList cmd);

}
