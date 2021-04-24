#include "user.h"

namespace sv {

    static UserCallbacks user_callbacks = {};

    void _user_callbacks_set(const UserCallbacks& callbacks)
    {
	user_callbacks = callbacks;
    }

    bool _user_initialize(bool init_engine)
    {
	if (user_callbacks.initialize)
	    return user_callbacks.initialize(init_engine);
	return true;
    }
    
    bool _user_close(bool close_engine)
    {
	if (user_callbacks.close)
	    return user_callbacks.close(close_engine);
	return true;
    }
    
    bool _user_validate_scene(const char* name)
    {
	if (user_callbacks.validate_scene)
	    return user_callbacks.validate_scene(name);
	return true;
    }
    
    bool _user_get_scene_filepath(const char* name, char* filepath)
    {
	if (user_callbacks.get_scene_filepath)
	    return user_callbacks.get_scene_filepath(name, filepath);
	return false;
    }
        
}
