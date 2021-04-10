#pragma once

#include "graphics.h"
#include "font.h"

namespace sv {

    enum GuiAlign : u32 {
	GuiAlign_Center = 0u,
	GuiAlign_Left = 1u,
	GuiAlign_Bottom = 1u,
	GuiAlign_Right = 2u,
	GuiAlign_Top = 2u,
    };
    
    enum GuiCoordConstraint : u8 {
	GuiCoordConstraint_Relative,
	GuiCoordConstraint_Center,
	GuiCoordConstraint_Pixel,
	GuiCoordConstraint_InversePixel,
    };

    struct GuiCoord {

	f32 value;
	GuiCoordConstraint constraint;

	SV_INLINE static GuiCoord Center() { return { 0.f, GuiCoordConstraint_Center }; }
	SV_INLINE static GuiCoord Relative(f32 n) { return { n, GuiCoordConstraint_Relative }; }
	SV_INLINE static GuiCoord IRelative(f32 n) { return { 1.f - n, GuiCoordConstraint_Relative }; }
	SV_INLINE static GuiCoord Pixel(f32 n) { return { n, GuiCoordConstraint_Pixel }; }
	SV_INLINE static GuiCoord IPixel(f32 n) { return { n, GuiCoordConstraint_InversePixel }; }
		
    };

    enum GuiDimConstraint : u8 {
	GuiDimConstraint_Relative,
	GuiDimConstraint_Absolute,
	GuiDimConstraint_Scale,
	GuiDimConstraint_Pixel,
	GuiDimConstraint_Aspect,
    };

    struct GuiDim {

	f32 value;
	GuiDimConstraint constraint;

	SV_INLINE static GuiDim Relative(f32 n) { return { n, GuiDimConstraint_Relative }; }
	SV_INLINE static GuiDim Absolute(f32 n) { return { n, GuiDimConstraint_Absolute }; }
	SV_INLINE static GuiDim Pixel(f32 n) { return { n, GuiDimConstraint_Pixel }; }
        SV_INLINE static GuiDim Scale(f32 n) { return { n, GuiDimConstraint_Scale }; }
	SV_INLINE static GuiDim Aspect(f32 n = 1.f) { return { n, GuiDimConstraint_Aspect }; }
		
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

    struct GUI;

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

    struct GuiParentUserData {
	f32 yoff;
	f32 xoff;
    };

    SV_API bool gui_create(u64 hashcode, GUI** pgui);
    SV_API bool gui_destroy(GUI* gui);

    SV_API void gui_begin(GUI* gui, f32 scale, f32 width, f32 height);
    SV_API void gui_end(GUI* gui);

    SV_API void gui_push_id(GUI* gui, u64 id);
    SV_API void gui_push_id(GUI* gui, const char* id);
    SV_API void gui_pop_id(GUI* gui, u32 count = 1u);

    SV_API void gui_xbounds(GUI* gui, GuiCoord x0, GuiCoord x1);
    SV_API void gui_xbounds(GUI* gui, GuiCoord x, GuiAlign align, GuiDim w);
    SV_API void gui_ybounds(GUI* gui, GuiCoord y0, GuiCoord y1);
    SV_API void gui_ybounds(GUI* gui, GuiCoord y, GuiAlign align, GuiDim h);
    SV_API void gui_bounds(GUI* gui, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1);
    SV_API void gui_bounds(GUI* gui, GuiCoord x, GuiAlign xalign, GuiDim w, GuiCoord y, GuiAlign yalign, GuiDim h);

    SV_API void gui_begin_container(GUI* gui, u64 id, const GuiContainerStyle& style = {});
    SV_API void gui_end_container(GUI* gui);
    
    SV_API bool gui_begin_popup(GUI* gui, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, const GuiPopupStyle& style = {});
    SV_API void gui_end_popup(GUI* gui);
	
    SV_API bool gui_begin_window(GUI* gui, const char* title, const GuiWindowStyle& style = {});
    SV_API void gui_end_window(GUI* gui);
    SV_API bool gui_show_window(GUI* gui, const char* title);
    SV_API bool gui_hide_window(GUI* gui, const char* title);

    SV_API bool gui_begin_menu_item(GUI* gui, const char* text, u64 id, const GuiMenuItemStyle& style = {});
    SV_API void gui_end_menu_item(GUI* gui);

    SV_API void gui_send_package(GUI* gui, const void* package, u32 package_size, u64 package_id);
    SV_API bool gui_recive_package(GUI* gui, void** package, u32* package_size, u64 package_id);

    SV_API bool gui_button(GUI* gui, const char* text, u64 id, const GuiButtonStyle& style = {});
	
    SV_API bool gui_drag_f32(GUI* gui, f32* value, f32 adv, f32 min, f32 max, u64 id, const GuiDragStyle& style = {});
    SV_INLINE bool gui_drag_f32(GUI* gui, f32* value, f32 adv, u64 id, const GuiDragStyle& style = {})
    {
	return gui_drag_f32(gui, value, adv, -f32_max, f32_max, id, style);
    }
	
    SV_API bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id, const GuiSliderStyle& style = {});
    SV_API void gui_text(GUI* gui, const char* text, u64 id, const GuiLabelStyle& style = {});
    SV_API bool gui_checkbox(GUI* gui, bool* value, u64 id, const GuiCheckboxStyle& style = {});
    SV_API bool gui_checkbox(GUI* gui, u64 id, const GuiCheckboxStyle& style = {});
	
    SV_API v4_f32 gui_parent_bounds(GUI* gui);
    SV_API GuiParentUserData& gui_parent_userdata(GUI* gui);

    SV_API void gui_draw(GUI* gui, CommandList cmd);

    // High level

    SV_API void gui_begin_grid(GUI* gui, u32 element_count, f32 element_size, f32 padding);
    SV_API void gui_begin_grid_element(GUI* gui, u64 id);
    SV_API void gui_end_grid_element(GUI* gui);
    SV_API void gui_end_grid(GUI* gui);

}
