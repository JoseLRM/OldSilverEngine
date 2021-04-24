#pragma once

#include "defines.h"

namespace sv {

    struct Archive;
    
    typedef bool(*UserInitializeFn)(bool init);
    typedef bool(*UserCloseFn)(bool close);
    typedef bool(*UserValidateSceneFn)(const char* name);
    typedef bool(*UserGetSceneFilepathFn)(const char* name, char* filepath);
    
    struct UserCallbacks {
	UserInitializeFn initialize;
	UserCloseFn close;
	UserValidateSceneFn validate_scene;
	UserGetSceneFilepathFn get_scene_filepath;
    };

    void _user_callbacks_set(const UserCallbacks& callbacks);

    bool _user_initialize(bool init_engine);
    bool _user_close(bool close_engine);
    bool _user_validate_scene(const char* name);
    bool _user_get_scene_filepath(const char* name, char* filepath);

}
