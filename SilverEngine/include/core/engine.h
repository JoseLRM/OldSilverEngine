#pragma once

#include "defines.h"

namespace sv {

    struct Scene;   
    
    struct GlobalEngineData {

	const Version    version = { 0, 0, 0 };
	const char*      name = "SilverEngine 0.0.0";
	f32		 deltatime = 0.f;
	u64		 frame_count = 0U;
	u32		 FPS = 0u;
	bool		 close_request = false;
	bool             running = false;
	void*            game_memory = nullptr;
    };
    
    extern GlobalEngineData SV_API engine;
    
}
