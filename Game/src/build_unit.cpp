#include "SilverEngine.h"

#include "dev.h"

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

SV_USER Result user_initialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    
    if (parchive) {
	Archive& archive = *parchive;
	archive >> m.entity;
	return Result_Success;
    }
    
    m.entity = create_entity(scene);
    add_component<SpriteComponent>(scene, m.entity);
    
    return Result_Success;
}

SV_USER Result user_serialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << m.entity;
    return Result_Success;
}

SV_USER void user_update()
{
    GameMemory& m = get_game_memory();
    Scene* scene = engine.scene;

    if (scene) {

	Transform trans = get_entity_transform(scene, m.entity);
	v2_f32 pos = trans.getLocalPosition().getVec2();

	f32 vel = 5.f * engine.deltatime;
	
	if (input.keys[Key_W]) {
	    pos.y += vel;
	}
	if (input.keys[Key_S]) {
	    pos.y -= vel;
	}
	if (input.keys[Key_A]) {
	    pos.x -= vel;
	}
	if (input.keys[Key_D]) {
	    pos.x += vel;
	}

	trans.setPosition(pos.getVec3());

	if (input.mouse_buttons[MouseButton_Right]) {

	    Entity e = create_entity(scene);
	    add_component<SpriteComponent>(scene, e);

	    Transform t = get_entity_transform(scene, e);

	    CameraComponent* cam = get_main_camera(scene);

	    if (cam)
		t.setPosition((input.mouse_position * v2_f32(cam->width, cam->height)).getVec3());
	}
    }
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "scenes/%s.scene", name);
    return true;
}
