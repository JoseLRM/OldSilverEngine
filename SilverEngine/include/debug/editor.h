#pragma once

#if SV_EDITOR

#include "core/scene.h"
#include "core/engine.h"

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

		f32 pitch = 0.f;
		f32 yaw = 0.f;
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

		bool postprocessing = true;
		bool draw_collisions = false;
		bool draw_cameras = false;
	
		DebugCamera camera;

		DoUndoStack do_undo_stack;
	
    };

    extern GlobalDevData SV_API dev;

    struct DisplayComponentEvent {
		CompID comp_id;
		Component* comp;
    };
    struct DisplayEntityEvent {
		Entity entity;
    };

	List<Entity>& editor_selected_entities();

	// COMPILER

	struct CompilePluginDesc {
		const char* plugin_path;
	};
	
	bool compile_plugin(const CompilePluginDesc& desc);

    // EDITOR GUI

    constexpr u32 ASSET_BROWSER_PACKAGE_TEXTURE = 69u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MESH = 70u;
    constexpr u32 ASSET_BROWSER_PACKAGE_MATERIAL = 71u;
    constexpr u32 ASSET_BROWSER_PACKAGE_SPRITE_SHEET = 72u;

	constexpr u32 ASSET_BROWSER_PREFAB = 73u;
	
    struct AssetPackage {
		char filepath[FILEPATH_SIZE + 1u];
    };

	struct PrefabPackage {
		char filepath[FILEPATH_SIZE + 1u];
    };

    SV_API void egui_header(const char* text, u64 id);
    SV_API void egui_transform(Entity entity);

    SV_API bool egui_begin_component(CompID comp_id, bool* remove);
    SV_API void egui_end_component();

    SV_API bool egui_comp_texture(const char* text, u64 id, TextureAsset* texture);
    SV_API void egui_comp_mesh(const char* text, u64 id, MeshAsset* mesh);
    SV_API void egui_comp_material(const char* text, u64 id, MaterialAsset* material);

}

#endif
