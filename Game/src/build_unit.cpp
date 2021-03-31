#include "SilverEngine.h"

using namespace sv;

struct GameMemory {

    Entity entity;
    
};

GameMemory& get_game_memory()
{
    return *reinterpret_cast<GameMemory*>(engine.game_memory);
}

SV_USER Result user_initialize()
{
    SV_LOG_INFO("Init from Game\n");

    engine.game_memory = allocate_memory(sizeof(GameMemory));

    set_active_scene("Test");
    
    return Result_Success;
}

SV_USER Result user_close()
{
    SV_LOG_INFO("Close from Game\n");

    free_memory(engine.game_memory);
    
    return Result_Success;
}

SV_USER Result user_initialize_scene(Scene* scene, Archive* archive)
{
    GameMemory& m = get_game_memory();
    m.entity = create_entity(scene);
    add_component<SpriteComponent>(scene, m.entity);
    
    return Result_Success;
}

SV_USER void user_update()
{
    GameMemory& m = get_game_memory();
    Scene* scene = engine.scene;

    if (scene) {

	SpriteComponent* spr = get_component<SpriteComponent>(scene, m.entity);
	if (spr) {
	    spr->color = Color::Blue(u8((sin(timer_now()) + 1.f) * 0.5f * 255.f));
	}

	Transform trans = get_entity_transform(scene, m.entity);
	v2_f32 pos = trans.getLocalPosition().getVec2();

	pos.x = cos((f32)timer_now()) * 10.f;
	pos.y = sin((f32)timer_now()) * 10.f;

	trans.setPosition(pos.getVec3());
	trans.setScale({ 1.f, 1.f, 69.f });
	
    }
}

