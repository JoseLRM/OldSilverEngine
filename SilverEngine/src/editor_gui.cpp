#include "dev.h"

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
	if (gui_begin_window(dev.gui, title)) {
			
	    GuiParentUserData& info = gui_parent_userdata(dev.gui);
	    info.yoff = 0.f;

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
	constexpr f32 HEIGHT = 10.f;

	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + 40.f));
	gui_text(dev.gui, text, id);
	info.yoff += 40.f + SEPARATOR + VPADDING;
    }

    void egui_transform(Entity entity)
    {
	constexpr f32 TRANSFORM_HEIGHT = 30.f;

	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	// TODO: euler rotation
	Transform trans = get_entity_transform(engine.scene, entity);
	v3_f32 position = trans.getLocalPosition();
	v3_f32 scale = trans.getLocalScale();

	GuiContainerStyle container_style;
	container_style.color = Color::White(0u);

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
	    GuiButtonStyle x_style;
	    x_style.color = { 229u, 25u, 25u, 255u };

	    GuiButtonStyle y_style;
	    y_style.color = { 51u, 204u, 51u, 255u };

	    GuiButtonStyle z_style;
	    z_style.color = { 13u, 25u, 229u, 255u };

	    constexpr f32 EXTERN_PADDING = 0.03f;
	    constexpr f32 INTERN_PADDING = 0.07f;

	    constexpr f32 ELEMENT_WIDTH = (1.f - EXTERN_PADDING * 2.f - INTERN_PADDING * 2.f) / 3.f;

	    // X
	    {
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 1.5f - INTERN_PADDING),
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f - INTERN_PADDING),
			   GuiCoord::IPixel(info.yoff),
			   GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 1u, container_style);
		
		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "X", 0u, x_style)) {

		    if (i == 1u)
			values->x = 1.f;
		    else
			values->x = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->x, 0.1f, 1u);

		gui_end_container(dev.gui);
	    }

	    // Y
	    {
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f - ELEMENT_WIDTH * 0.5f),
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f),
			   GuiCoord::IPixel(info.yoff),
			   GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 2u, container_style);

		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "Y", 0u,  y_style)) {

		    if (i == 1u)
			values->y = 1.f;
		    else
			values->y = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->y, 0.1f, 1u);

		gui_end_container(dev.gui);
	    }

	    // Z
	    {
		gui_bounds(dev.gui,
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 0.5f + INTERN_PADDING),
			   GuiCoord::Relative(0.5f + ELEMENT_WIDTH * 1.5f + INTERN_PADDING),
			   GuiCoord::IPixel(info.yoff),
			   GuiCoord::IPixel(info.yoff + TRANSFORM_HEIGHT));
		
		gui_begin_container(dev.gui, 3u, container_style);

		gui_bounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(0.25f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		if (gui_button(dev.gui, "Z", 0u, z_style)) {

		    if (i == 1u)
			values->z = 1.f;
		    else
			values->z = 0.f;
		}

		gui_bounds(dev.gui, GuiCoord::Relative(0.25f), GuiCoord::Relative(1.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
		gui_drag_f32(dev.gui, &values->z, 0.1f, 1u);

		gui_end_container(dev.gui);
	    }

	    gui_pop_id(dev.gui);

	    info.yoff += TRANSFORM_HEIGHT + VPADDING;
	}

	info.yoff += SEPARATOR;

	trans.setPosition(position);
	trans.setScale(scale);
    }

    bool egui_button(const char* text, u64 id)
    {
	constexpr f32 HEIGHT = 25.f;

	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_bounds(dev.gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + HEIGHT));
	bool res = gui_button(dev.gui, text, id);
	info.yoff += HEIGHT + VPADDING;
	return res;
    }

    bool egui_begin_component(Entity entity, CompID comp_id, bool* remove)
    {
	GuiContainerStyle container_style;
	container_style.color = Color::Red(100u);

	GuiLabelStyle text_style;
	text_style.text_color = Color::Black();
	text_style.text_alignment = TextAlignment_Left;

	GuiCheckboxStyle checkbox_style;
	checkbox_style.active_box = GuiBox::Triangle(Color::Black(), true, 0.5f);
	checkbox_style.inactive_box = GuiBox::Triangle(Color::Black(), false, 0.5f);
	checkbox_style.color = Color::Black(0u);

	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u) + entity);
	gui_push_id(dev.gui, id);

	*remove = false;

	gui_bounds(dev.gui, GuiCoord::Relative(0.1f), GuiCoord::Relative(0.9f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + 40.f));
	gui_begin_container(dev.gui, 0u, container_style);

	gui_bounds(dev.gui, GuiCoord::Pixel(35.f), GuiCoord::IPixel(10.f), GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
	gui_text(dev.gui, get_component_name(comp_id), 1u, text_style);

	if (gui_begin_popup(dev.gui, GuiPopupTrigger_LastWidget, MouseButton_Right, 2u)) {

	    gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(5.f), GuiCoord::IPixel(25.f));
	    *remove = gui_button(dev.gui, "Remove", 0u);

	    gui_end_popup(dev.gui);
	}

	gui_xbounds(dev.gui, GuiCoord::Pixel(0.f), GuiAlign_Left, GuiDim::Aspect());
	gui_ybounds(dev.gui, GuiCoord::Relative(0.f), GuiCoord::Relative(1.f));
	bool show = gui_checkbox(dev.gui, 3u, checkbox_style);

	gui_end_container(dev.gui);

	info.yoff += 40.f + VPADDING;

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
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);
	gui_bounds(dev.gui, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_button(dev.gui, "TODO", 1u);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	gui_pop_id(dev.gui);
    }

    void egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
    {
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);

	gui_bounds(dev.gui, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_button(dev.gui, "TODO", 1u);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_TEXTURE)) {

	    Result res = load_asset_from_file(*texture, package->filepath);
	    if (result_fail(res)) {

		SV_LOG_ERROR("Can't load the texture '%s': %s", package->filepath, result_str(res));
	    }
	}

	gui_pop_id(dev.gui);
    }

    void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh)
    {
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);
	gui_bounds(dev.gui, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_button(dev.gui, "TODO", 1u);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MESH)) {

	    Result res = load_asset_from_file(*mesh, package->filepath);
	    if (result_fail(res)) {

		SV_LOG_ERROR("Can't load the mesh '%s': %s", package->filepath, result_str(res));
	    }
	}

	gui_pop_id(dev.gui);
    }

    void egui_comp_material(const char* text, u64 id, MaterialAsset* material)
    {
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);
	gui_bounds(dev.gui, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_button(dev.gui, "TODO", 1u);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	AssetPackage* package;
	if (gui_recive_package(dev.gui, (void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MATERIAL)) {

	    Result res = load_asset_from_file(*material, package->filepath);
	    if (result_fail(res)) {

		SV_LOG_ERROR("Can't load the material '%s': %s", package->filepath, result_str(res));
	    }
	}

	gui_pop_id(dev.gui);
    }

    bool egui_comp_bool(const char* text, u64 id, bool* value)
    {
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);

	gui_xbounds(dev.gui, GuiCoord::Relative(0.55f), GuiAlign_Left, GuiDim::Aspect());
	gui_ybounds(dev.gui, GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	bool res = gui_checkbox(dev.gui, value, 1u);

	gui_pop_id(dev.gui);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	return res;
    }

    bool egui_comp_drag_f32(const char* text, u64 id, f32* value, f32 adv, f32 min, f32 max)
    {
	GuiParentUserData& info = gui_parent_userdata(dev.gui);

	gui_push_id(dev.gui, id);

	gui_bounds(dev.gui, GuiCoord::Relative(0.05f), GuiCoord::Relative(0.5f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	gui_text(dev.gui, text, 0u);

	gui_bounds(dev.gui, GuiCoord::Relative(0.55f), GuiCoord::Relative(0.95f), GuiCoord::IPixel(info.yoff), GuiCoord::IPixel(info.yoff + COMP_ITEM_HEIGHT));
	bool res = gui_drag_f32(dev.gui, value, adv, min, max, 1u);

	gui_pop_id(dev.gui);
	info.yoff += COMP_ITEM_HEIGHT + VPADDING;

	return res;
    }

}

#endif
