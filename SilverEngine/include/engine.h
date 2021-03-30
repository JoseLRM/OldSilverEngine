#pragma once

#include "core.h"

namespace sv {

    struct Scene;

    typedef Result(*UserInitializeFn)();
    typedef void(*UserUpdateFn)();
    typedef Result(*UserCloseFn)();
    
    struct UserCallbacks {
	UserInitializeFn initialize;
	UserUpdateFn update;
	UserCloseFn close;
    };
    
    struct GlobalEngineData {

	UserCallbacks      user = {};
	const Version      version = { 0, 0, 0 };
	const char*        name = "SilverEngine 0.0.0";
	f32		   deltatime = 0.f;
	u64		   frame_count = 0U;
	u32		   FPS = 0u;
	bool		   close_request = false;
	bool               running = false;
	Scene*	           scene = nullptr;
	std::string	   next_scene_name;
    };

    extern GlobalEngineData engine;
    
}
