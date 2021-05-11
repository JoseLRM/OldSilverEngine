#include "debug/editor.h"

#if SV_DEV

namespace sv {
    
    void egui_header(const char* text, u64 id)
    {
	gui_text(text, id);
    }

    void egui_transform(Entity entity)
    {
	constexpr f32 TRANSFORM_HEIGHT = 30.f;

	gui_push_id("ENTITY TRANSFORM");

	/*
	// TODO: euler rotation
	v3_f32& position = *get_entity_position_ptr(entity);
	v3_f32& scale = *get_entity_scale_ptr(entity);

	f32 yoff = 0.f;

	foreach(i, 2u) {

	    gui_push_id(i + 0x23549abf);

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
		gui_push_style(GuiStyle_ButtonColor, Color{ 229u, 25u, 25u, 255u });
		
		gui_begin_container(1u, GuiLayout_Flow);
		
		if (gui_button("X", 0u)) {

		    if (i == 1u)
			values->x = 1.f;
		    else
			values->x = 0.f;
		}

		gui_drag_f32(&values->x, 0.1f, 1u);

		gui_end_container();

		gui_pop_style();
	    }

	    // Y
	    {
		gui_push_style(GuiStyle_ButtonColor, Color{ 51u, 204u, 51u, 255u });
		
		gui_begin_container(2u, GuiLayout_Flow);

		if (gui_button("Y", 0u)) {

		    if (i == 1u)
			values->y = 1.f;
		    else
			values->y = 0.f;
		}

		gui_drag_f32(&values->y, 0.1f, 1u);

		gui_end_container();

		gui_pop_style();
	    }

	    // Z
	    {
		gui_push_style(GuiStyle_ButtonColor, Color{ 13u, 25u, 229u, 255u });

		gui_begin_container(3u, GuiLayout_Flow);

		if (gui_button("Z", 0u)) {

		    if (i == 1u)
			values->z = 1.f;
		    else
			values->z = 0.f;
		}

		gui_drag_f32(&values->z, 0.1f, 1u);

		gui_end_container();

		gui_pop_style();
	    }

	    gui_pop_id();

	    yoff += TRANSFORM_HEIGHT + 5.f;
	}

	gui_end_container();*/

	gui_pop_id();
    }

    bool egui_begin_component(Entity entity, CompID comp_id, bool* remove)
    {
	u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u) + entity);
	gui_push_id(id);

	*remove = false;
	
	bool show = gui_collapse(get_component_name(comp_id), 0u);
	
	/*if (gui_begin_popup(GuiPopupTrigger_LastWidget, MouseButton_Right, 1u, GuiLayout_Flow)) {

	    *remove = gui_button("Remove", 0u);

	    gui_end_popup();
	    }*/

	if (show) {

	    gui_push_id("SHOW COMPONENT");
	}
	else {

	    gui_pop_id();
	}

	return show;
    }

    void egui_end_component()
    {
	gui_pop_id(2u);
    }
	
    void egui_comp_color(const char* text, u64 id, Color* pcolor)
    {
	gui_push_id(id);

	/*gui_same_line(2u);
	
	gui_text(text, 0u);
	gui_drag_color4(pcolor, 1u);*/

	gui_pop_id();
    }

    void egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
    {
	gui_push_id(id);

	gui_image(texture->get(), GPUImageLayout_ShaderResource, 0u);

	/*AssetPackage* package;
	if (gui_recive_package((void**)&package, nullptr, ASSET_BROWSER_PACKAGE_TEXTURE)) {

	    bool res = load_asset_from_file(*texture, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the texture '%s'", package->filepath);
	    }
	    }*/

	gui_pop_id();
    }

    void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh)
    {
	gui_push_id(id);

	/*gui_same_line(2u);
	
	gui_text(text, 0u);
	gui_button("TODO", 1u);

	AssetPackage* package;
	if (gui_recive_package((void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MESH)) {

	    bool res = load_asset_from_file(*mesh, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the mesh '%s'", package->filepath);
	    }
	    }*/

	gui_pop_id();
    }

    void egui_comp_material(const char* text, u64 id, MaterialAsset* material)
    {
	gui_push_id(id);

	/*gui_same_line(2u);
	
	gui_text(text, 0u);
	gui_button("TODO", 1u);

	AssetPackage* package;
	if (gui_recive_package((void**)&package, nullptr, ASSET_BROWSER_PACKAGE_MATERIAL)) {

	    bool res = load_asset_from_file(*material, package->filepath);
	    if (!res) {

		SV_LOG_ERROR("Can't load the material '%s'", package->filepath);
	    }
	    }*/

	gui_pop_id();
    }

    bool egui_comp_bool(const char* text, u64 id, bool* value)
    {
	return gui_checkbox(text, *value, id);
    }

    bool egui_comp_drag_f32(const char* text, u64 id, f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(id);

	gui_text(text, 0u);
	bool res = gui_drag_f32(*value, adv, min, max, 1u);

	gui_pop_id();

	return res;
    }

    bool egui_comp_drag_v4_f32(const char* text, u64 id, v4_f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(id);

	gui_text(text, 0u);
	bool res = gui_drag_v4_f32(*value, adv, min, max, 1u);

	gui_pop_id();

	return res;
    }

    bool egui_comp_drag_v2_f32(const char* text, u64 id, v2_f32* value, f32 adv, f32 min, f32 max)
    {
	gui_push_id(id);

	gui_text(text, 0u);
	bool res = gui_drag_v2_f32(*value, adv, min, max, 1u);

	gui_pop_id();

	return res;
    }
    
}

#endif
