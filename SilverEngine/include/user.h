#pragma once

#include "defines.h"

namespace sv {
    
    typedef void(*UserInitializeFn)();
    typedef void(*UserCloseFn)();
    typedef bool(*UserValidateSceneFn)(const char* name);
    typedef bool(*UserGetSceneFilepathFn)(const char* name, char* filepath);
    
    struct UserCallbacks {
	UserInitializeFn initialize;
	UserCloseFn close;
	UserValidateSceneFn validate_scene;
	UserGetSceneFilepathFn get_scene_filepath;
    };

    void _user_callbacks_set(const UserCallbacks& callbacks);

    void _user_initialize();
    void _user_close();
    bool _user_validate_scene(const char* name);
    bool _user_get_scene_filepath(const char* name, char* filepath);

    bool _user_connected();

}
