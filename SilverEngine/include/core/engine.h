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

#if SV_DEV
	
	char project_path[FILEPATH_SIZE + 1u] = "";
	bool update_scene = true;

#endif
	
    };
    
    extern GlobalEngineData SV_API engine;

    SV_INLINE void filepath_resolve(char* dst, const char* src)
    {
	size_t size = strlen(src);
	if (size) {

	    if (src[0] == '$') {
		++src;
		strcpy(dst, src);
	    }
	    else {
		strcpy(dst, engine.project_path);
		strcat(dst, src);
	    }
	}
	else strcpy(dst, src);
    }

#if SV_DEV
    void _engine_initialize_project(const char* project_path);
    void _engine_close_project();
    
    void _engine_reset_game();
#endif
    
}
