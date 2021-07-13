#include "debug/editor.h"

#if SV_EDITOR

namespace sv {
    
    void egui_header(const char* text, u64 id)
    {
		gui_text(text, id);
    }

    void egui_transform(Entity entity)
    {
		// TODO: This is global
		static v3_f32 rotation;
		static v4_f32 last_quaternion;
		
		v3_f32& position = *get_entity_position_ptr(entity);
		v4_f32& rotation_quat = *get_entity_rotation_ptr(entity);
		v3_f32& scale = *get_entity_scale_ptr(entity);

		if (rotation_quat != last_quaternion) {
			
			rotation = quaternion_to_euler_angles(rotation_quat);
		}

		gui_push_id("ENTITY TRANSFORM");
	
		gui_drag_v3_f32(NULL, position, 0.05f, -f32_max, f32_max, 0u, GuiDragFlag_Position);
		if (gui_drag_v3_f32(NULL, rotation, 0.05f, -f32_max, f32_max, 1u, GuiDragFlag_Rotation)) {

			rotation_quat = quaternion_from_euler_angles(rotation);
		}
		gui_drag_v3_f32(NULL, scale, 0.05f, -f32_max, f32_max, 2u, GuiDragFlag_Scale);

		last_quaternion = rotation_quat;
	
		gui_pop_id();
    }

    bool egui_begin_component(CompID comp_id, bool* remove)
    {
		u64 id = u64("SHOW COMPONENT") ^ u64((u64(comp_id) << 32u));
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
   

    bool egui_comp_texture(const char* text, u64 id, TextureAsset* texture)
    {
		bool res = false;
		
		gui_push_id(id);

		gui_image(texture->get(), 50.f, 0u);

		AssetPackage* package;
		if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_TEXTURE, GuiReciverTrigger_LastWidget)) {

			res = load_asset_from_file(*texture, package->filepath);
			if (!res) {

				SV_LOG_ERROR("Can't load the texture '%s'", package->filepath);
			}
		}

		gui_pop_id();

		return res;
    }

	bool gui_sound_asset(const char* text, Sound& sound, u64 id)
    {
		bool res = false;
		
		gui_push_id(id);

		const char* filepath = sound.get_filepath();
		if (filepath == NULL) filepath = "None";
		
		gui_text(filepath);

		AssetPackage* package;
		if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_SOUND, GuiReciverTrigger_LastWidget)) {

			res = load_asset_from_file(sound, package->filepath);
			if (!res) {

				SV_LOG_ERROR("Can't load the sound '%s'", package->filepath);
			}
		}

		if (gui_begin_popup(GuiPopupTrigger_LastWidget)) {

			if (gui_button("Remove")) {
				unload_asset(sound);
				gui_close_popup();
				res = true;
			}
			
			gui_end_popup();
		}

		gui_pop_id();

		return res;
    }

    void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh)
    {
		bool res = false;
		
		gui_push_id(id);

		gui_button("TODO -> Mesh");

		AssetPackage* package;
		if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_MESH, GuiReciverTrigger_LastWidget)) {

			res = load_asset_from_file(*mesh, package->filepath);
			if (!res) {

				SV_LOG_ERROR("Can't load the mesh '%s'", package->filepath);
			}
		}

		gui_pop_id();
    }

    void egui_comp_material(const char* text, u64 id, MaterialAsset* material)
    {
		bool res = false;
		
		gui_push_id(id);

		gui_button("TODO -> Material");

		AssetPackage* package;
		if (gui_recive_package((void**)&package, ASSET_BROWSER_PACKAGE_MATERIAL, GuiReciverTrigger_LastWidget)) {

			res = load_asset_from_file(*material, package->filepath);
			if (!res) {

				SV_LOG_ERROR("Can't load the material '%s'", package->filepath);
			}
		}

		gui_pop_id();
    }
    
}

#endif
