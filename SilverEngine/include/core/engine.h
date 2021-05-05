#pragma once

#include "defines.h"

namespace sv {

    struct Scene;
    
    struct GlobalEngineData {

	const Version    version = { SV_VERSION_MAJOR, SV_VERSION_MINOR, SV_VERSION_REVISION };
	const char*      name = "SilverEngine";
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
#if SV_DEV
	if (src[0] == '$') {
	    ++src;
	    strcpy(dst, src);
	}
	else {
	    strcpy(dst, engine.project_path);
	    strcat(dst, src);
	}
#else
	// TODO: This is usless without the dev mode
	if (src[0] == '$')
	    ++src;
	    	
	strcpy(dst, src);
#endif
    }

#if SV_DEV
    void _engine_initialize_project(const char* project_path);
    void _engine_close_project();
    
    void _engine_reset_game();
#endif
    
}
