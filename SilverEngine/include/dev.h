#pragma once

#if SV_DEV

#include "scene.h"

namespace sv {

    
    struct DebugCamera : public CameraComponent {
	v3_f32 position;
	v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f };
	f32 velocity = 0.45f;
    };

    enum GameState : u32 {
	GameState_Edit,
	GameState_Play,
	GameState_Pause,
    };
    
    struct GlobalDevData {
		
	bool console_active = false;
	GameState next_game_state = GameState_Edit;
	GameState game_state = GameState_Edit;
	
	bool debug_draw = true;
	DebugCamera camera;

	DoUndoStack do_undo_stack;

	GUI* gui;
    };

    extern GlobalDevData SV_API_VAR dev;

    // EDITOR GUI

    constexpr u32 ASSET_BROWSER_PACKAGE_TEXTURE = 69u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MESH = 70u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MATERIAL = 71u;
	
    struct AssetPackage {
	static constexpr u32 MAX_SIZE = 500u;
	char filepath[MAX_SIZE + 1u];
    };

    bool egui_begin();
    void egui_end();

    SV_API bool egui_begin_window(const char* title);
    SV_API void egui_end_window();

    SV_API void egui_header(const char* text, u64 id);
    SV_API bool egui_button(const char* text, u64 id);
    SV_API void egui_transform(Entity entity);

    SV_API bool egui_begin_component(Entity entity, CompID comp_id, bool* remove);
    SV_API void egui_end_component();

    SV_API void egui_comp_color(const char* text, u64 id, Color* pcolor);
    SV_API void egui_comp_texture(const char* text, u64 id, TextureAsset* texture);
    SV_API void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh);
    SV_API void egui_comp_material(const char* text, u64 id, MaterialAsset* material);
    SV_API bool egui_comp_bool(const char* text, u64 id, bool* value);
    SV_API bool egui_comp_drag_f32(const char* text, u64 id, f32* value, f32 adv = 0.1f, f32 min = f32_min, f32 max = f32_max);

}

#endif
