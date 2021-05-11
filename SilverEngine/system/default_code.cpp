#include "SilverEngine.h"

using namespace sv;

struct TestComponent : public BaseComponent {

    static constexpr u32 VERSION = 1u;
    static CompID ID;

    bool state = true;

    void serialize(Serializer& s)
    {
	serialize_bool(s, state);
    }
    
    void deserialize(Deserializer& d, u32 version)
    {
	if (version == 1u) {
	    deserialize_bool(d, state);
	}
    }
};

CompID TestComponent::ID = SV_COMPONENT_ID_INVALID;

#if SV_DEV

void show_component_info(ShowComponentEvent* event)
{
    if (event->comp_id == TestComponent::ID) {
	
	TestComponent* comp = reinterpret_cast<TestComponent*>(event->comp);
	gui_checkbox("State", comp->state, 0u);
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


SV_USER void user_initialize()
{
    event_user_register("initialize_game", initialize_game);
    event_user_register("update_scene", update_scene);

    register_component<TestComponent>("Test");

#if SV_DEV
    event_user_register("show_component_info", show_component_info);
#endif   
}

SV_USER void user_close()
{
    invalidate_component_callbacks(TestComponent::ID);
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
