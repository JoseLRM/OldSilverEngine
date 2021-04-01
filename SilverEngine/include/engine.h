#pragma once

#include "core.h"

namespace sv {

    struct Scene;

    typedef Result(*UserInitializeFn)();
    typedef void(*UserUpdateFn)();
    typedef Result(*UserCloseFn)();
    typedef bool(*UserValidateSceneFn)(const char* name);
    typedef bool(*UserGetSceneFilepathFn)(const char* name, char* filepath);
    typedef Result(*UserInitializeSceneFn)(Scene* scene, Archive* parchive);
    typedef Result(*UserCloseSceneFn)(Scene* scene);
    typedef Result(*UserSerializeSceneFn)(Scene* scene, Archive* parchive);
    
    struct UserCallbacks {
	UserInitializeFn initialize;
	UserUpdateFn update;
	UserCloseFn close;
	UserValidateSceneFn validate_scene;
	UserGetSceneFilepathFn get_scene_filepath;
	UserInitializeSceneFn initialize_scene;
	UserCloseSceneFn close_scene;
	UserSerializeSceneFn serialize_scene;
    };

    constexpr u32 SCENE_NAME_SIZE = 50u;
    
    struct GlobalEngineData {

	UserCallbacks    user = {};
	const Version    version = { 0, 0, 0 };
	const char*      name = "SilverEngine 0.0.0";
	f32		 deltatime = 0.f;
	u64		 frame_count = 0U;
	u32		 FPS = 0u;
	bool		 close_request = false;
	bool             running = false;
	Scene*	         scene = nullptr;
	char	         next_scene_name[SCENE_NAME_SIZE];
	void*            game_memory = nullptr;
    };
    
    extern GlobalEngineData SV_API_VAR engine;

    SV_INLINE bool user_validate_scene(const char* name)
    {
	return engine.user.validate_scene ? engine.user.validate_scene(name) : true;
    }

    SV_INLINE bool user_get_scene_filepath(const char* name, char* filepath)
    {
	return engine.user.get_scene_filepath ? engine.user.get_scene_filepath(name, filepath) : false;
    }

    SV_INLINE Result user_initialize_scene(Scene* scene, Archive* parchive)
    {
	return engine.user.initialize_scene ? engine.user.initialize_scene(scene, parchive) : Result_Success;
    }

    SV_INLINE Result user_serialize_scene(Scene* scene, Archive* parchive)
    {
	return engine.user.serialize_scene ? engine.user.serialize_scene(scene, parchive) : Result_Success;
    }

    SV_INLINE Result user_close_scene(Scene* scene)
    {
	return engine.user.close_scene ? engine.user.close_scene(scene) : Result_Success;
    }
    
}
