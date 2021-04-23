#include "debug/editor.h"

#if SV_DEV

namespace sv {

    constexpr f32 VPADDING = 5.f;
    constexpr f32 SEPARATOR = 30.f;
    constexpr f32 COMP_ITEM_HEIGHT = 30.f;

    bool egui_begin()
    {
	gui_begin(dev.gui, 0.1f, f32(os_window_size().x), f32(os_window_size().y));
		
		

	return true;
    }

    void egui_end()
    {
	gui_end(dev.gui);
    }

    bool egui_begin_window(const char* title)
    {	
	if (gui_begin_window(dev.gui, title, GuiLayout_Flow)) {
	    return true;
	}

	return false;
    }

    void egui_end_window()
    {
	gui_end_window(dev.gui);
    }

    void egui_header(const char* text, u64 id)
    {
	gui_text(dev.gui, text, id);
    }

    void egui_transform(Entity entity)
    {
	constexpr f32 TRANSFORM_HEIGHT = 30.f;

	gui_push_id(dev.gui, "ENTITY TRANSFORM");
	
	gui_begin_container(dev.gui, 0u, GuiLayout_Free);

	// TODO: euler rotation
	v3_f32& position = *get_entity_position_ptr(entity);
	v3_f32& scale = *get_entity_scale_ptr(entity);

	f32 yoff = 0.f;

	foreach(i, 2u) {

	    gui_push_id(dev.gui, i + 0x23549abf);

	    v3_f32* values;

	    switch (i) {
	    case 0:
		values = &position;
		break;
	    case 1:
		values = &scale;
		break;
	    default:
		values = &position;
	    }

	    // TODO: Hot color

	    constexpr f32 EXTERN_PADDING = 0.03f;
	    constexpr f32 INTERN_PADDING = 0.07f;

	    constexpr f32 ELEMENT_WIDTH = (1.f - EXTERN_PADDING * 2.f - INTERN_PADDING * 2.f) / 3.f;
	    
	    // X
	    {
		gui_push_style(dev.gui, GuiStyle_ButtonColor, Color{ 229u, 25u, 25u, 255u });
		
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 1.5f - INTERN_PADDING),
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f - INTERN_PADDING),
			   GuiCoord::IPixel(yoff),
			   GuiCoord::IPixel(yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 1u, GuiLayout_Free);
		
		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "X", 0u)) {

		    if (i == 1u)
			values->x = 1.f;
		    else
			values->x = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->x, 0.1f, 1u);

		gui_end_container(dev.gui);

		gui_pop_style(dev.gui);
	    }

	    // Y
	    {
		gui_push_style(dev.gui, GuiStyle_ButtonColor, Color{ 51u, 204u, 51u, 255u });
		
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f),
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f),
			   GuiCoord::IPixel(yoff),
			   GuiCoord::IPixel(yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 2u, GuiLayout_Free);

		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "Y", 0u)) {

		    if (i == 1u)
			values->y = 1.f;
		    else
			values->y = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->y, 0.1f, 1u);

		gui_end_container(dev.gui);

		gui_pop_style(dev.gui);
	    }

	    // Z
	    {
		gui_push_style(dev.gui, GuiStyle_ButtonColor, Color{ 13u, 25u, 229u, 255u });
		
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f + INTERN_PADDING),
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 1.5f + INTERN_PADDING),
			   GuiCoord::IPixel(yoff),
			   GuiCoord::IPixel(yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 3u, GuiLayout_Free);

		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "Z", 0u)) {

		    if (i == 1u)
			values->z = 1.f;
		    else
			values->z = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->z, 0.1f, 1u);

		gui_end_container(dev.gui);

		gui_pop_style(dev.gui);
	    }

	    gui_pop_id(dev.gui);

	    yoff += TRANSFORM_HEIGHT + VPADDING;
	}

	gui_end_container(dev.gui);

	gui_pop_id(dev.gui);
    }

    bool egui_button(const char* text, u64 id)
    {
	return gui_button(dev.gui, text, id);
    }

    bool egui_begin_component(Entity entity, CompID comp_id, bool* remove)
    {
	u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u) + entity);
	gui_push_id(dev.gui, id);

	*remove = false;
	
	gui_begin_container(dev.gui, 0u, GuiLayout_Free);

	gui_xbounds(dev.gui, GuiCoord::Pixel(35.f), GuiCoord::IPixel(10.f));
	gui_ybounds(dev.gui, GuiCoord::IPixel(0.f), GuiAlign_Top, GuiDim::Pixel(20.f));
	gui_text(dev.gui, get_component_name(comp_id), 1u);

	if (gui_begin_popup(dev.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 2u, GuiLayout_Flow)) {

	    gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f));
	    *remove = gui_button(dev.gui, "Remove", 0u);

	    gui_end_popup(dev.gui);
	}

	gui_xbounds(dev.gui, GuiCoord::Pixel(0.f), GuiAlign_Left, GuiDim::Aspect());
	gui_ybounds(dev.gui, GuiCoord::IPixel(0.f), GuiAlign_Top, GuiDim::Pixel(20.f));
	bool show = gui_checkbox(dev.gui, 3u);

	gui_end_container(dev.gui);

	if (show) {

	    gui_push_id(dev.gui, "SHOW COMPONENT");
	}
	else {

	    gui_pop_id(dev.gui);
	}

	return show;
    }

    void egui_end_component()
    {
	gui_pop_id(dev.gui, 2u);
    }
	
    void egui_comp_color(const char* text, u64 id, Color* pcolor)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);
	
	gui_text(dev.gui, text, 0u);
	gui_drag_color4(dev.gui, pcolor, 1u);

	gui_pop_id(dev.gui);
    }

    void egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);
	
	gui_text(dev.gui, text, 0u);
	gui_image(dev.gui, texture->get(), GPUImageLayout_ShaderResource, 0u);

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_TEXTURE)) {

	    bool res = load_asset_from_file(*texture, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the texture '%s'", package->filepath);
	    }
	}

	gui_pop_id(dev.gui);
    }

    void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);
	
	gui_text(dev.gui, text, 0u);
	gui_button(dev.gui, "TODO", 1u);

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MESH)) {

	    bool res = load_asset_from_file(*mesh, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the mesh '%s'", package->filepath);
	    }
	}

	gui_pop_id(dev.gui);
    }

    void egui_comp_material(const char* text, u64 id, MaterialAsset* material)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);
	
	gui_text(dev.gui, text, 0u);
	gui_button(dev.gui, "TODO", 1u);

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MATERIAL)) {

	    bool res = load_asset_from_file(*material, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the material '%s'", package->filepath);
	    }
	}

	gui_pop_id(dev.gui);
    }

    bool egui_comp_bool(const char* text, u64 id, bool* value)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);
	
	gui_text(dev.gui, text, 0u);
	bool res = gui_checkbox(dev.gui, value, 1u);

	gui_pop_id(dev.gui);

	return res;
    }

    bool egui_comp_drag_f32(const char* text, u64 id, f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);

	gui_text(dev.gui, text, 0u);
	bool res = gui_drag_f32(dev.gui, value, adv, min, max, 1u);

	gui_pop_id(dev.gui);

	return res;
    }

    bool egui_comp_drag_v4_f32(const char* text, u64 id, v4_f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(dev.gui, id);

	gui_same_line(dev.gui, 2u);

	gui_text(dev.gui, text, 0u);
	bool res = gui_drag_v4_f32(dev.gui, value, adv, min, max, 1u);

	gui_pop_id(dev.gui);

	return res;
    }

    bool egui_comp_drag_v2_f32(const char* text, u64 id, v2_f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(dev.gui, id);
	
	gui_same_line(dev.gui, 2u);

	gui_text(dev.gui, text, 0u);
	bool res = gui_drag_v2_f32(dev.gui, value, adv, min, max, 1u);

	gui_pop_id(dev.gui);

	return res;
    }

}

#endif
