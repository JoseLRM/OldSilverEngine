#pragma once

#include "platform/graphics.h"
#include "platform/os.h"
#include "core/font.h"

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
	GuiDimConstraint_Adjust, // Used only for parents
    };

    struct GuiDim {

	f32 value;
	GuiDimConstraint constraint;

	SV_INLINE static GuiDim Relative(f32 n) { return { n, GuiDimConstraint_Relative }; }
	SV_INLINE static GuiDim Absolute(f32 n) { return { n, GuiDimConstraint_Absolute }; }
	SV_INLINE static GuiDim Pixel(f32 n) { return { n, GuiDimConstraint_Pixel }; }
        SV_INLINE static GuiDim Scale(f32 n) { return { n, GuiDimConstraint_Scale }; }
	SV_INLINE static GuiDim Aspect(f32 n = 1.f) { return { n, GuiDimConstraint_Aspect }; }
	SV_INLINE static GuiDim Adjust(f32 n = 1.f) { return { n, GuiDimConstraint_Adjust }; }
	
    };

    enum GuiCheckboxShape : u32 {
	GuiCheckboxShape_Quad,
	GuiCheckboxShape_Triangle,
    };

    enum GuiPopupTrigger : u32 {
	GuiPopupTrigger_Custom,
	GuiPopupTrigger_Parent,
	GuiPopupTrigger_LastWidget,
    };

    enum GuiStyle : u32 {
	GuiStyle_ContainerColor,
	GuiStyle_WindowBackgroundColor,
	GuiStyle_WindowDecorationColor,
	GuiStyle_WindowOutlineColor,
	GuiStyle_WindowDecorationHeight,
	GuiStyle_WindowOutlineSize,
	GuiStyle_WindowMinWidth,
	GuiStyle_WindowMinHeight,
	GuiStyle_PopupColor,
	GuiStyle_ButtonColor,
	GuiStyle_ButtonHotColor,
	GuiStyle_ButtonTextColor,
	GuiStyle_ButtonImage,
	GuiStyle_ButtonTexcoord,
	GuiStyle_SliderColor,
	GuiStyle_SliderButtonColor,
	GuiStyle_SliderButtonSize,
	GuiStyle_TextColor,
	GuiStyle_TextAlignment,
	GuiStyle_TextBackgroundColor,
	GuiStyle_CheckboxButtonColor,
	GuiStyle_CheckboxBackgroundColor,
	GuiStyle_CheckboxShape,
	GuiStyle_CheckboxShapeSizeMult,
	GuiStyle_DragTextColor,
	GuiStyle_DragBackgroundColor,
	GuiStyle_ImageTexcoord,
	// TODO Menuitem
	GuiStyle_FlowX0,
	GuiStyle_FlowX1,
	GuiStyle_FlowSubX0,
	GuiStyle_FlowSubX1,
    };

    enum GuiLayout : u32 {
	GuiLayout_Free,
	GuiLayout_Flow,
	GuiLayout_Grid,
    };

    struct GUI;

    SV_API bool gui_create(u64 hashcode, GUI** pgui);
    SV_API bool gui_destroy(GUI* gui);

    SV_API void gui_begin(GUI* gui, f32 scale, f32 width, f32 height);
    SV_API void gui_end(GUI* gui);

    SV_API void gui_same_line(GUI* gui, u32 count);

    SV_API void gui_push_id(GUI* gui, u64 id);
    SV_API void gui_push_id(GUI* gui, const char* id);
    SV_API void gui_pop_id(GUI* gui, u32 count = 1u);

    SV_API void gui_global_style_set(GUI* gui, GuiStyle style, const void* value, size_t size);
    SV_API void gui_global_style_get(GUI* gui, GuiStyle style, void* value, size_t size);
    SV_API void gui_push_style(GUI* gui, GuiStyle style, const void* value, size_t size);
    SV_API void gui_pop_style(GUI* gui, u32 count = 1u);
    
    template<typename T>
    SV_INLINE void gui_push_style(GUI* gui, GuiStyle style, const T& t) { gui_push_style(gui, style, &t, sizeof(T)); }
    template<typename T>
    SV_INLINE void gui_global_style_set(GUI* gui, GuiStyle style, const T& t) { gui_global_style_set(gui, style, &t, sizeof(T)); }

    template<typename T>
    SV_INLINE void gui_global_style_get(GUI* gui, GuiStyle style, T& t) { gui_global_style_get(gui, style, &t, sizeof(T)); }
    
    SV_API void gui_xbounds(GUI* gui, GuiCoord x0, GuiCoord x1);
    SV_API void gui_xbounds(GUI* gui, GuiCoord x, GuiAlign align, GuiDim w);
    SV_API void gui_ybounds(GUI* gui, GuiCoord y0, GuiCoord y1);
    SV_API void gui_ybounds(GUI* gui, GuiCoord y, GuiAlign align, GuiDim h);
    SV_API void gui_bounds(GUI* gui, GuiCoord x0, GuiCoord x1, GuiCoord y0, GuiCoord y1);
    SV_API void gui_bounds(GUI* gui, GuiCoord x, GuiAlign xalign, GuiDim w, GuiCoord y, GuiAlign yalign, GuiDim h);

    SV_API void gui_begin_container(GUI* gui, u64 id, GuiLayout layout);
    SV_API void gui_end_container(GUI* gui);
    
    SV_API bool gui_begin_popup(GUI* gui, GuiPopupTrigger trigger, MouseButton mouse_button, u64 id, GuiLayout layout);
    SV_API void gui_end_popup(GUI* gui);
	
    SV_API bool gui_begin_window(GUI* gui, const char* title, GuiLayout layout);
    SV_API void gui_end_window(GUI* gui);
    SV_API bool gui_show_window(GUI* gui, const char* title);
    SV_API bool gui_hide_window(GUI* gui, const char* title);

    SV_API bool gui_begin_menu_item(GUI* gui, const char* text, u64 id, GuiLayout layout);
    SV_API void gui_end_menu_item(GUI* gui);

    SV_API void gui_send_package(GUI* gui, const void* package, u32 package_size, u64 package_id);
    SV_API bool gui_recive_package(GUI* gui, void** package, u32* package_size, u64 package_id);

    SV_API bool gui_text_field(GUI* gui, char* buff, size_t buff_size, u64 id);
    
    SV_API bool gui_button(GUI* gui, const char* text, u64 id);
	
    SV_API bool gui_drag_f32(GUI* gui, f32* value, f32 adv, f32 min, f32 max, u64 id);
    SV_API bool gui_drag_u8(GUI* gui, u8* value, u8 adv, u8 min, u8 max, u64 id);
    
    SV_INLINE bool gui_drag_f32(GUI* gui, f32* value, f32 adv, u64 id)
    {
	return gui_drag_f32(gui, value, adv, -f32_max, f32_max, id);
    }
    SV_INLINE bool gui_drag_u8(GUI* gui, u8* value, u8 adv, u64 id)
    {
	return gui_drag_u8(gui, value, adv, 0u, u8_max, id);
    }
	
    SV_API bool gui_slider(GUI* gui, f32* value, f32 min, f32 max, u64 id);
    SV_API void gui_text(GUI* gui, const char* text, u64 id);
    SV_API bool gui_checkbox(GUI* gui, const char* text, bool* value, u64 id);
    SV_API bool gui_checkbox(GUI* gui, const char* text, u64 id);

    SV_API void gui_image(GUI* gui, GPUImage* image, GPUImageLayout layout, u64 id);
	
    SV_API v4_f32 gui_parent_bounds(GUI* gui);

    SV_API void gui_draw(GUI* gui, CommandList cmd);

    // High level

    SV_INLINE bool gui_drag_color4(GUI* gui, Color* color, u64 id)
    {
	gui_push_style(gui, GuiStyle_ContainerColor, Color::Transparent());
	
	gui_begin_container(gui, id, GuiLayout_Flow);

	gui_push_style(gui, GuiStyle_DragBackgroundColor, *color);
	gui_push_style(gui, GuiStyle_FlowX0, 0.f);
	gui_push_style(gui, GuiStyle_FlowX1, 1.f);
	gui_push_style(gui, GuiStyle_FlowSubX0, 0.02f);
	gui_push_style(gui, GuiStyle_FlowSubX1, 0.98f);
	
	gui_same_line(gui, 4u);

	bool res = false;
	if (gui_drag_u8(gui, &color->r, 1u, 0u)) res = true;
	if (gui_drag_u8(gui, &color->g, 1u, 1u)) res = true;
	if (gui_drag_u8(gui, &color->b, 1u, 2u)) res = true;
	if (gui_drag_u8(gui, &color->a, 1u, 3u)) res = true;

	gui_pop_style(gui, 6u);
	
	gui_end_container(gui);

	return res;
    }

    SV_INLINE bool gui_drag_v4_f32(GUI* gui, v4_f32* v, f32 adv, f32 min, f32 max, u64 id)
    {
	gui_push_style(gui, GuiStyle_ContainerColor, Color::Transparent());
	
	gui_begin_container(gui, id, GuiLayout_Flow);

	gui_push_style(gui, GuiStyle_FlowX0, 0.f);
	gui_push_style(gui, GuiStyle_FlowX1, 1.f);
	gui_push_style(gui, GuiStyle_FlowSubX0, 0.02f);
	gui_push_style(gui, GuiStyle_FlowSubX1, 0.98f);
	
	gui_same_line(gui, 4u);

	bool res = false;
	if (gui_drag_f32(gui, &v->x, adv, min, max, 0u)) res = true;
	if (gui_drag_f32(gui, &v->y, adv, min, max, 1u)) res = true;
	if (gui_drag_f32(gui, &v->z, adv, min, max, 2u)) res = true;
	if (gui_drag_f32(gui, &v->w, adv, min, max, 3u)) res = true;

	gui_pop_style(gui, 5u);
	
	gui_end_container(gui);

	return res;
    }

    SV_INLINE bool gui_drag_v2_f32(GUI* gui, v2_f32* v, f32 adv, f32 min, f32 max, u64 id)
    {
	gui_push_style(gui, GuiStyle_ContainerColor, Color::Transparent());
	
	gui_begin_container(gui, id, GuiLayout_Flow);

	gui_push_style(gui, GuiStyle_FlowX0, 0.f);
	gui_push_style(gui, GuiStyle_FlowX1, 1.f);
	gui_push_style(gui, GuiStyle_FlowSubX0, 0.02f);
	gui_push_style(gui, GuiStyle_FlowSubX1, 0.98f);
	
	gui_same_line(gui, 2u);

	bool res = false;
	if (gui_drag_f32(gui, &v->x, adv, min, max, 0u)) res = true;
	if (gui_drag_f32(gui, &v->y, adv, min, max, 1u)) res = true;

	gui_pop_style(gui, 5u);
	
	gui_end_container(gui);

	return res;
    }

    SV_API void gui_display_style_settings(GUI* gui);

}
