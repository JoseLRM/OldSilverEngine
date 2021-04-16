#include "SilverEngine.h"

using namespace sv;

struct GameMemory {
    Entity foo;
};

GameMemory& get_game_memory()
{
    return *reinterpret_cast<GameMemory*>(engine.game_memory);
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	engine.game_memory = allocate_memory(sizeof(GameMemory));
    
	set_active_scene("Test");
    }
    
    return true;
}

SV_USER bool user_close()
{
    free_memory(engine.game_memory);
    
    return true;
}

SV_USER bool user_initialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    
    if (parchive) {
	Archive& archive = *parchive;
	archive >> m.foo;
	return true;
    }


    // Create main camera
    scene->main_camera = create_entity(scene, SV_ENTITY_NULL, "Camera");
    add_component<CameraComponent>(scene, scene->main_camera);

    // Create some entity
    m.foo = create_entity(scene, SV_ENTITY_NULL, "Foo");
    SpriteComponent* spr = add_component<SpriteComponent>(scene, m.foo);
    spr->color = Color::Salmon();
    
    BodyComponent* body = add_component<BodyComponent>(scene, m.foo);
    body->body_type = BodyType_Dynamic;

    return true;
}

SV_USER bool user_serialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << m.foo;
    return true;
}

SV_USER void user_update()
{
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
