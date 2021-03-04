#pragma once

#include "SilverEngine/graphics.h"
#include "SilverEngine/font.h"

namespace sv {

	enum GuiConstraint : u32 {
		GuiConstraint_Relative,
		GuiConstraint_Center,
		GuiConstraint_Pixel,
		GuiConstraint_Aspect,
	};

	enum GuiAlignment : u32 {
		GuiAlignment_Left = 0u,
		GuiAlignment_Bottom = 0u,
		GuiAlignment_Center = 1u,
		GuiAlignment_Right = 2u,
		GuiAlignment_Top = 2u,

		GuiAlignment_InverseLeft = 3u,
		GuiAlignment_InverseBottom = 3u,
		GuiAlignment_InverseCenter = 4u,
		GuiAlignment_InverseRight = 5u,
		GuiAlignment_InverseTop = 5u,
	};

	struct GuiCoord {

		f32 value;
		GuiConstraint constraint;
		GuiAlignment alignment;

		SV_INLINE static GuiCoord center() { return { 0.f, GuiConstraint_Center, GuiAlignment_Center }; }
		SV_INLINE static GuiCoord flow(f32 n) { return { n, GuiConstraint_Pixel, GuiAlignment_InverseTop }; }
		
	};

	struct GuiDim {

		f32 value;
		GuiConstraint constraint;

		SV_INLINE static GuiDim relative(f32 n) { return { n, GuiConstraint_Relative }; }
		SV_INLINE static GuiDim aspect(f32 n) { return { n, GuiConstraint_Aspect }; }
		SV_INLINE static GuiDim pixel(f32 n) { return { n, GuiConstraint_Pixel }; }
		
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

	struct GuiButtonStyle {
		Color color = Color::Gray(200u);
		Color hot_color = Color::White();
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

	Result gui_create(GUI** pgui);
	Result gui_destroy(GUI* gui);

	void gui_begin(GUI* gui, f32 width, f32 height);
	void gui_end(GUI* gui);
	
	void gui_begin_container(GUI* gui, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiContainerStyle& style = {});
	void gui_end_container(GUI* gui);
	
	bool gui_begin_window(GUI* gui, const char* title, const GuiWindowStyle& style = {});
	void gui_end_window(GUI* gui);
	Result gui_show_window(GUI* gui, const char* title);
	Result gui_hide_window(GUI* gui, const char* title);

	bool gui_button(GUI* gui, const char* text, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiButtonStyle& style = {});
	bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiSliderStyle& style = {});
	void gui_text(GUI* gui, const char* text, GuiCoord x, GuiCoord y, GuiDim w, GuiDim h, const GuiTextStyle& style = {});
	// TODO drag
	
	void gui_draw(GUI* gui, GPUImage* offscreen, CommandList cmd);

}
