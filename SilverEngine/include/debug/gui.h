#pragma once

#include "platform/graphics.h"
#include "platform/os.h"
#include "core/font.h"
#include "core/scene.h"

namespace sv {

    constexpr u32 GUI_WINDOW_NAME_SIZE = 30u;

    bool _gui_initialize();
    bool _gui_close();
	void _gui_load(const char* name);

    bool _gui_begin();
    void _gui_end();

    void _gui_draw(CommandList cmd);

    struct GuiStyle {
		Color widget_primary_color = { 30u, 0u, 0u, 255u };
		Color widget_secondary_color = { 10u, 10u, 10u, 255u };
		Color widget_highlighted_color = { 40u, 5u, 5u, 255u };
		Color widget_focused_color = Color::Red();
		Color widget_text_color = Color::White();
		Color check_color = Color::White();
		Color window_background_color = Color::Gray(4u);
		Color window_focused_background_color = Color::Gray(6u);
		Color window_decoration_color = Color::Gray(20u);
		Color window_focused_decoration_color = Color::Gray(5);
		Color window_button_color = { 30u, 0u, 0u, 255u };
		Color window_button_highlighted_color = { 40u, 5u, 5u, 255u };
		Color window_button_focused_color = { 10u, 10u, 10u, 255u };
		Color popup_background_color = Color::Gray(5u);
		Color popup_outline_color = Color::Gray(50u);
		f32   popup_outline_size = 3.f;
		Color screen_background_primary_color = Color::Gray(1u);
		Color screen_background_secondary_color = Color::Gray(3u);
		Color docking_button_color = Color::Red();
		Color docking_highlighted_button_color = Color::White();
		f32 scale = 1.f;
    };

	enum GuiElementListFlag : u32 {
		GuiElementListFlag_ShowDefault, // TODO
		GuiElementListFlag_Parent
	};

    enum GuiDragFlag : u32 {
		GuiDragFlag_Position = SV_BIT(0),
		GuiDragFlag_Scale = SV_BIT(1),
		GuiDragFlag_Rotation = SV_BIT(2)
    };

    enum GuiImageFlag : u32 {
		GuiImageFlag_Fullscreen = SV_BIT(0)
    };

	enum GuiImageButtonFlag : u32 {
		GuiImageButtonFlag_NoBackground = SV_BIT(0),
		GuiImageButtonFlag_Disabled = SV_BIT(1),
    };

    enum GuiWindowFlag : u32 {
		GuiWindowFlag_NoClose = SV_BIT(0u),
		GuiWindowFlag_NoDocking = SV_BIT(1u),
		GuiWindowFlag_FocusMaster = SV_BIT(2u),
		GuiWindowFlag_DefaultHide = SV_BIT(3u),
		GuiWindowFlag_NoResize = SV_BIT(4u),
		GuiWindowFlag_AutoResize = SV_BIT(5u) | GuiWindowFlag_NoResize,

		GuiWindowFlag_Temporal = GuiWindowFlag_FocusMaster | GuiWindowFlag_NoDocking | GuiWindowFlag_DefaultHide | GuiWindowFlag_AutoResize,
    };

    enum GuiPopupTrigger : u32 {
		GuiPopupTrigger_Root,
		GuiPopupTrigger_LastWidget
    };

	enum GuiReciverTrigger : u32 {
		GuiReciverTrigger_Root,
		GuiReciverTrigger_LastWidget
    };

	enum GuiTopLocation : u32 {
		GuiTopLocation_Left,
		GuiTopLocation_Center,
		GuiTopLocation_Right,
	};

    SV_API void gui_push_id(u64 id);
    SV_API void gui_push_id(const char* id);
    SV_API void gui_pop_id(u32 count = 1u);
	
    SV_API bool gui_begin_window(const char* title, u32 flags = 0u);
    SV_API void gui_end_window();
    SV_API bool gui_show_window(const char* title);
    SV_API bool gui_hide_window(const char* title);
	SV_API bool gui_showing_window(const char* title);

    SV_API bool gui_begin_popup(GuiPopupTrigger trigger);
    SV_API void gui_end_popup();
	SV_API void gui_close_popup();

	SV_API v2_f32 gui_root_size();
	SV_API v2_f32 gui_root_position();

	SV_API bool gui_image_catch_input(u64 id);

	SV_API void gui_send_package(const void* data, size_t size, u64 package_id);
	SV_API bool gui_recive_package(void** dst, u64 package_id, GuiReciverTrigger trigger);

    SV_API bool gui_button(const char* text, u64 id = u64_max);
	SV_API bool gui_image_button(const char* text, GPUImage* image, v4_f32 texcoord, u64 id = u64_max, u32 flags = 0u);
    SV_API bool gui_checkbox(const char* text, bool& value, u64 id = u64_max);
    SV_API bool gui_checkbox(const char* text, u64 id = u64_max);
    SV_API bool gui_drag_f32(const char* text, f32& value, f32 adv = 0.1f, f32 min = -f32_max, f32 max = f32_max, u64 id = u64_max, u32 flags = 0u);
    SV_API bool gui_drag_v2_f32(const char* text, v2_f32& value, f32 adv = 0.1f, f32 min = -f32_max, f32 max = f32_max, u64 id = u64_max, u32 flags = 0u);
    SV_API bool gui_drag_v3_f32(const char* text, v3_f32& value, f32 adv = 0.1f, f32 min = -f32_max, f32 max = f32_max, u64 id = u64_max, u32 flags = 0u);
    SV_API bool gui_drag_v4_f32(const char* text, v4_f32& value, f32 adv = 0.1f, f32 min = -f32_max, f32 max = f32_max, u64 id = u64_max, u32 flags = 0u);
    SV_API bool gui_drag_u32(const char* text, u32& value, u32 adv = 1u, u32 min = 0u, u32 max = u32_max, u64 id = u64_max, u32 flags = 0u);
    SV_API void gui_text(const char* text, u64 id = u64_max);
	SV_API bool gui_text_field(char* buff, size_t buff_size, u64 id, u32 flags = 0u);
    SV_API bool gui_collapse(const char* text, u64 id = u64_max);
    SV_API void gui_image_ex(GPUImage* image, GPUImageLayout layout, f32 height, v4_f32 texcoord, u64 id, u32 flags = 0u);
    SV_API void gui_separator(u32 level);

	SV_API bool gui_begin_combobox(const char* preview, u64 id, u32 flags = 0u);
	SV_API void gui_end_combobox();

    SV_API void gui_begin_grid(f32 width, f32 padding, u64 id);
    SV_API void gui_end_grid();

    SV_API bool gui_select_filepath(const char* filepath, char* out, u64 id, u32 flags = 0u);
    SV_API bool gui_asset_button(const char* text, GPUImage* image, v4_f32 texcoord, u64 id, u32 flags = 0u);

	SV_API void gui_begin_top(GuiTopLocation location);
	SV_API void gui_end_top();

	// LISTS

	SV_API void gui_begin_list(u64 id);
	SV_API void gui_end_list();
	SV_API bool gui_begin_element_list(const char* text, u64 id, bool selected, u32 flags = 0u);
	SV_API void gui_end_element_list();
	SV_API bool gui_element_pressed();

    // HELPERS

	SV_INLINE void gui_image(GPUImage* image, f32 height, v4_f32 texcoord, u64 id, u32 flags = 0u)
	{
		gui_image_ex(image, GPUImageLayout_ShaderResource, height, texcoord, id, flags);
	}

    SV_INLINE void gui_image(GPUImage* image, f32 height, u64 id, u32 flags = 0u) { gui_image(image, height, { 0.f, 0.f, 1.f, 1.f }, id, flags); }

	// HIGH LEVEL

	constexpr u64 GUI_PACKAGE_SPRITE = 0x4F39A88B2;
	constexpr u64 GUI_PACKAGE_SPRITE_ANIMATION = 0xA34C39E83;

	struct GuiSpritePackage {
		char sprite_sheet_filepath[FILEPATH_SIZE + 1u];
		u32 sprite_id;
	};

	struct GuiSpriteAnimationPackage {
		char sprite_sheet_filepath[FILEPATH_SIZE + 1u];
		u32 animation_id;
	};

	SV_INLINE bool gui_sprite(const char* text, SpriteSheetAsset& sprite_sheet, u32& sprite_id, u64 id = u64_max)
	{
		SpriteSheet* sheet = sprite_sheet.get();
		GPUImage* image = sheet ? sprite_sheet->texture.get() : NULL;
		v4_f32 texcoord = sheet ? sheet->get_sprite_texcoord(sprite_id) : v4_f32{};
		
		gui_image(image, 70.f, texcoord, id);
		
		GuiSpritePackage* package;

		if (gui_recive_package((void**)&package, GUI_PACKAGE_SPRITE, GuiReciverTrigger_LastWidget)) {

			SpriteSheetAsset asset;

			if (load_asset_from_file(asset, package->sprite_sheet_filepath)) {

				sprite_sheet = asset;
				sprite_id = package->sprite_id;
				return true;
			}
		}
		
		return false;
	}

	SV_INLINE bool gui_sprite_animation(const char* text, SpriteSheetAsset& sprite_sheet, u32& animation_id, u64 id = u64_max)
	{
		SpriteSheet* sheet = sprite_sheet.get();
		GPUImage* image = sheet ? sprite_sheet->texture.get() : NULL;
		// TODO
		v4_f32 texcoord = sheet ? sheet->get_sprite_texcoord(animation_id) : v4_f32{};
		
		gui_image(image, 70.f, texcoord, id);
		
		GuiSpriteAnimationPackage* package;

		if (gui_recive_package((void**)&package, GUI_PACKAGE_SPRITE_ANIMATION, GuiReciverTrigger_LastWidget)) {

			SpriteSheetAsset asset;

			if (load_asset_from_file(asset, package->sprite_sheet_filepath)) {

				sprite_sheet = asset;
				animation_id = package->animation_id;
				return true;
			}
		}
		
		return false;
	}

	SV_INLINE bool gui_drag_color(const char* text, Color& color, u64 id = u64_max)
	{
		v4_f32 value = color_to_vec4(color);
		if (gui_drag_v4_f32(text, value, 0.01f, 0.f, 1.f, id)) {
			color = color_float(value.x, value.y, value.z, value.w);
			return true;
		}
		return false;
	}

	SV_API void gui_display_style_editor();
	
}
