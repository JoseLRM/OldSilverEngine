#pragma once

#if SV_DEV

#include "core/scene.h"
#include "core/engine.h"

// TEMP
#include "debug/gui.h"

namespace sv {

    void _editor_update();
    bool _editor_initialize();
    bool _editor_close();
    void _editor_draw();

    struct DebugCamera : public CameraComponent {
	v3_f32 position;
	v4_f32 rotation = { 0.f, 0.f, 0.f, 1.f }; // TODO: Use pitch and yaw
	f32 velocity = 0.45f;
    };

    enum EngineState : u32 {
	EngineState_None,
	EngineState_ProjectManagement,
	EngineState_Edit,
	EngineState_Play,
	EngineState_Pause,
    };

    // TODO: Remove this struct
    struct GlobalDevData {

	EngineState engine_state = EngineState_None;
	EngineState next_engine_state = EngineState_None;

	bool debug_draw = true;
	bool postprocessing = true;
	bool draw_collisions = false;
	
	DebugCamera camera;

	DoUndoStack do_undo_stack;
	
    };

    extern GlobalDevData SV_API dev;

    struct ShowComponentEvent {
	CompID comp_id;
	BaseComponent* comp;
    };
    struct ShowEntityEvent {
	Entity entity;
    };

    // EDITOR GUI

    constexpr u32 ASSET_BROWSER_PACKAGE_TEXTURE = 69u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MESH = 70u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MATERIAL = 71u;
	
    struct AssetPackage {
	char filepath[FILEPATH_SIZE + 1u];
    };

    SV_API void egui_header(const char* text, u64 id);
    SV_API bool egui_button(const char* text, u64 id);
    SV_API void egui_transform(Entity entity);

    SV_API bool egui_begin_component(Entity entity, CompID comp_id, bool* remove);
    SV_API void egui_end_component();

    SV_API void egui_comp_color(const char* text, u64 id, Color* pcolor);
    SV_API void egui_comp_texture(const char* text, u64 id, TextureAsset* texture);
    SV_API void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh);
    SV_API void egui_comp_material(const char* text, u64 id, MaterialAsset* material);

}

#endif
