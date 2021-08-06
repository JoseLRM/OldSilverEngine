#pragma once

#include "defines.h"
#include "platform/os.h"

namespace sv {

    struct Scene;
    
    struct GlobalEngineData {

		const Version version = { SV_VERSION_MAJOR, SV_VERSION_MINOR, SV_VERSION_REVISION };
		f32		      deltatime = 0.f;
		u64		      frame_count = 0U;
		u32		      FPS = 0u;
		bool		  close_request = false;
		bool          running = false;
		void*         game_memory = nullptr;
		f32           timestep = 1.f;

#if SV_EDITOR
	
		char project_path[FILEPATH_SIZE + 1u] = "";
		bool update_scene = true;

#endif
	
    };
    
    extern GlobalEngineData SV_API engine;

    SV_INLINE void filepath_resolve(char* dst, const char* src)
    {
#if SV_EDITOR
		if (src[0] == '$') {
			++src;
			strcpy(dst, src);
		}
		else {
			if (path_is_absolute(src))
				strcpy(dst, src);
			else {
				strcpy(dst, engine.project_path);
				strcat(dst, src);
			}
		}
#else
		// TODO: This is usless without the dev mode
		if (src[0] == '$')
			++src;
	    	
		strcpy(dst, src);
#endif
    }

#if SV_EDITOR
    void _engine_initialize_project(const char* project_path);
    void _engine_close_project();
    
    void _engine_reset_game(const char* scene = "");
#endif
    
}
