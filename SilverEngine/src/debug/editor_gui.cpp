#include "debug/editor.h"

#if SV_DEV

namespace sv {
    
    void egui_header(const char* text, u64 id)
    {
		gui_text(text, id);
    }

    void egui_transform(Entity entity)
    {
		// TODO: euler rotation
		v3_f32& position = *get_entity_position_ptr(entity);
		v3_f32& scale = *get_entity_scale_ptr(entity);

		gui_push_id("ENTITY TRANSFORM");
	
		gui_drag_v3_f32(0, position, 0.05f, -f32_max, f32_max, 0u, GuiDragFlag_Position);
		//gui_drag_v3_f32(rotation, 0.05f, -f32_max, f32_max, 1u, GuiDragFlag_Rotation);
		gui_drag_v3_f32(0, scale, 0.05f, -f32_max, f32_max, 2u, GuiDragFlag_Scale);
	
		gui_pop_id();
    }

    bool egui_begin_component(Entity entity, CompID comp_id, bool* remove)
    {
		u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u) + entity);
		gui_push_id(id);

		*remove = false;
	
		bool show = gui_collapse(get_component_name(comp_id), 0u);
	
		if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {
			*remove = gui_button("Remove", 0u);
			gui_end_popup();
		}

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
		v4_f32 value = pcolor->toVec4();
		if (gui_drag_v4_f32(text, value, 0.01f, 0.f, 1.f, id)) {
			pcolor->setFloat(value.x, value.y, value.z, value.w);
		}
    }

    void egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
    {
		gui_push_id(id);

		gui_image(texture->get(), 50.f, 0u);

		AssetPackage* package;
		if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_TEXTURE)) {

			bool res = load_asset_from_file(*texture, package->filepath);
			if (!res) {

				SV_LOG_ERROR("Can't load the texture '%s'", package->filepath);
			}
		}

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
    
}

#endif
