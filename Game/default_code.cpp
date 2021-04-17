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

SV_USER bool user_close(bool close)
{
    if (close)
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

SV_USER void user_update()
{
    GameMemory& m = get_game_memory();

    if (entity_exist(engine.scene, m.foo)) {

	Transform trans = get_entity_transform(engine.scene, m.foo);
	v2_f32 pos = trans.getLocalPosition().getVec2();

	f64 t = timer_now();

	pos.x = f32(cos(t)) * 4.f;
	pos.y = f32(sin(t)) * 2.f;

	trans.setPosition(pos.getVec3());
    }
}

SV_USER bool user_serialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << m.foo;
    return true;
}


SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}
