#include "SilverEngine.h"

#include "dev.h"

using namespace sv;

constexpr u32 VERSION = 0u;

struct GameMemory {

    Entity player;
    Entity cat;
    
};

GameMemory& get_game_memory()
{
    return *reinterpret_cast<GameMemory*>(engine.game_memory);
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	engine.game_memory = allocate_memory(sizeof(GameMemory));
    
	set_active_scene("level_0");
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

	u32 version;
	archive >> version;

	if (version == VERSION) {
	
	    archive >> m.player;
	    archive >> m.cat;
	    return true;
	}
    }


// Create camera
    scene->main_camera = create_entity(scene, SV_ENTITY_NULL, "Camera");
    add_component<CameraComponent>(scene, scene->main_camera);

    // Create player
    m.player = create_entity(scene, SV_ENTITY_NULL, "Player");
    SpriteComponent* spr = add_component<SpriteComponent>(scene, m.player);
    load_asset_from_file(spr->texture, "images/ghost.png");
    BodyComponent* body = add_component<BodyComponent>(scene, m.player);
    body->body_type = BodyType_Dynamic;

    // Create cat
    m.cat = create_entity(scene, SV_ENTITY_NULL, "Cat");
    add_component<SpriteComponent>(scene, m.cat);
    body = add_component<BodyComponent>(scene, m.cat);
    body->body_type = BodyType_Dynamic;
    
    return true;
}

SV_USER bool user_serialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << VERSION;
    archive << m.player;
    archive << m.cat;
    return true;
}

void explosion(Scene* scene, v2_f32 pos, f32 range, f32 intensity, Entity* ignore_list, u32 ignore_count)
{
    EntityView<BodyComponent> bodies(scene);

    for (ComponentView<BodyComponent> v : bodies) {

	BodyComponent& b = *v.comp;

	if (b.body_type == BodyType_Static)
	    continue;

	bool ignore = false;
	foreach(i, ignore_count)
	    if (ignore_list[i] == v.entity) {
		ignore = true;
		break;
	    }
	if (ignore) continue;
		
	
	Transform trans = get_entity_transform(scene, v.entity);

	v2_f32 p = trans.getLocalPosition().getVec2();

	v2_f32 to = p - pos;
	f32 distance = to.length();

	if (distance < range) {

	    to.normalize();
	    to *= intensity * pow(distance / range, 0.1f);

	    b.vel += to;
	}
    }
}

SV_USER void user_update()
{
    GameMemory& m = get_game_memory();
    Scene* scene = engine.scene;

    if (scene) {

	scene->gravity = { 0.f, -36.f };
	
	Transform trans = get_entity_transform(scene, m.player);
	BodyComponent* body = get_component<BodyComponent>(scene, m.player);
	v2_f32 pos = trans.getLocalPosition().getVec2();

	{
	    Transform cam = get_entity_transform(scene, scene->main_camera);
	    //cam.setPosition(pos.getVec3());
	}

	f32 vel = 5.f * engine.deltatime;

	if (input.keys[Key_A]) {
	    pos.x -= vel;
	}
	if (input.keys[Key_D]) {
	    pos.x += vel;
	}

	if (body->in_ground) {

	    if (input.keys[Key_Space]) {
		body->vel.y = 27.f * 0.5f;
		
	    }
	    
	}

	trans.setPosition(pos.getVec3());
    }
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "worlds/%s.scene", name);
    return true;
}

SV_USER bool user_validate_scene(const char* name)
{
    size_t size = strlen(name);

    if (size <= 6) return false;

    return true;
}
