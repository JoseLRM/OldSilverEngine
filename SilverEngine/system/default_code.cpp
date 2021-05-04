#include "SilverEngine.h"

using namespace sv;

struct TestComponent {

    static constexpr u32 VERSION = 1u;
    static CompID ID;

    bool state = true;

    void serialize(Serializer& s)
    {
	serialize_bool(s, state);
    }
    
    void deserializer(Deserializer& d, u32 version)
    {
	if (version == 1u) {
	    deserialize_bool(d, state);
	}
    }
};

CompID TestComponent::ID = SV_COMPONENT_INVALID_ID;

#if SV_DEV

void show_component_info(ShowComponentEvent* event)
{
    if (event->comp_id == TestComponent::ID) {
	
	TestComponent* comp = reinterpret_cast<TestComponent*>(event->comp);
	egui_comp_bool("State", 0u, &comp->state);
    }
}

#endif

void initialize_game()
{
    set_scene("Main");
}

void update_scene()
{
    SceneData& scene = *get_scene_data();
    
    if (entity_exist(scene.player)) {

	TestComponent* test = get_component<TestComponent>(scene.player);
	SpriteComponent* spr = get_component<SpriteComponent>(scene.player);

	if (test && spr) {

	    spr->color = test->state ? Color::Red() : Color::Blue();
	}
    }
}


SV_USER bool user_initialize(bool init)
{
    event_user_register("initialize_game", initialize_game);
    event_user_register("update_scene", update_scene);

    register_component<TestComponent>("Test");

#if SV_DEV
    event_user_register("show_component_info", show_component_info);
#endif
    
    return true;
}

SV_USER bool user_close(bool close)
{
    invalidate_component(TestComponent::ID);
    
    return true;
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
