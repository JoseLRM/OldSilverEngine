#include "SilverEngine.h"

using namespace sv;

void update_scene()
{
    SceneData& scene = *get_scene_data();
    
    if (entity_exist(scene.player)) {
	
    }
}

void show_component_info(void*, ShowComponentEvent* event)
{
    static bool test = false;
    
    egui_comp_bool("", 0u, &test);

    if (input.keys[Key_Enter] == InputState_Pressed) {

	ModelInfo info;
	if (load_model("c:/Users/josel/3D Objects/gobber/GoblinX.obj", info)) {

	    import_model("assets/model_test", info);
	}

	clear_scene();

	create_entity_model(SV_ENTITY_NULL, "assets/model_test");
    }
}

void on_body_collision(void*, BodyCollisionEvent* event)
{
    if (event->state == CollisionState_Enter) {
	SV_LOG("Enter");
    }
    if (event->state == CollisionState_Leave) {
	SV_LOG("Leave");
    }
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	set_scene("Test");
    }

    event_user_register("update_scene", update_scene);
    event_user_register("on_body_collision", on_body_collision);
    event_user_register("show_component_info", show_component_info);
    
    return true;
}

SV_USER bool user_close(bool close)
{
    return true;
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
