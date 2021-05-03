#include "user.h"

namespace sv {

    static UserCallbacks user_callbacks = {};

    void _user_callbacks_set(const UserCallbacks& callbacks)
    {
	user_callbacks = callbacks;
    }

    bool _user_initialize()
    {
	if (user_callbacks.initialize)
	    return user_callbacks.initialize();
	return false;
    }
    
    bool _user_close()
    {
	if (user_callbacks.close)
	    return user_callbacks.close();
	return false;
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

	sprintf(filepath, "assets/scenes/%s.scene", name);
	return true;
    }
        
}
