#include "SilverEngine.h"

using namespace sv;

void update_scene()
{
    SceneData& scene = *get_scene_data();
    
    if (entity_exist(scene.player)) {
	
    }
}

void show_component_info(ShowComponentEvent* event)
{
    static bool test = false;
    
    egui_comp_bool("", 0u, &test);

    if (false && input.keys[Key_Enter] == InputState_Pressed) {

	ModelInfo info;
	if (load_model("c:/Users/josel/3D Objects/gobber/GoblinX.obj", info)) {

	    import_model("assets/model_test", info);
	}

	clear_scene();

	create_entity_model(SV_ENTITY_NULL, "assets/model_test");
    }
}

void on_body_collision(BodyCollisionEvent* event)
{
    if (event->state == CollisionState_Enter) {
	SV_LOG("Collision Enter");
    }
    if (event->state == CollisionState_Leave) {
	SV_LOG("Collision Leave");
    }
}

void on_trigger_collision(TriggerCollisionEvent* event)
{
    if (event->state == CollisionState_Enter) {
	SV_LOG("Trigger Enter");
	
    }
    if (event->state == CollisionState_Leave) {
	SV_LOG("Trigger Leave");
	BodyComponent* b = get_component<BodyComponent>(event->body.entity);
	b->vel += v2_f32(-10, 30);
    }
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	set_scene("Test");
    }

    event_user_register("update_scene", update_scene);
    event_user_register("on_body_collision", on_body_collision);
    event_user_register("on_trigger_collision", on_trigger_collision);
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
