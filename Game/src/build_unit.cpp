#include "SilverEngine.h"

#include "dev.h"

using namespace sv;

struct GameMemory {

    Entity player;
    
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
	archive >> m.player;
	return true;
    }

    scene->main_camera = create_entity(scene, SV_ENTITY_NULL, "Camera");
    add_component<CameraComponent>(scene, scene->main_camera);
    m.player = create_entity(scene, SV_ENTITY_NULL, "Player");
    add_component<SpriteComponent>(scene, m.player);
    BodyComponent* body = add_component<BodyComponent>(scene, m.player);
    body->body_type = BodyType_Dynamic;
    
    return true;
}

SV_USER bool user_serialize_scene(Scene* scene, Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << m.player;
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

	f32 vel = 30.f * engine.deltatime;

	if (body->in_ground) {

	    if (input.keys[Key_Tab]) {
		body->vel.y = 40.f;
	    }
	    
	    if (input.keys[Key_Space]) {
		body->vel.y = 27.f * 0.5f;
		
	    }
	    
	}

	if (input.keys[Key_A]) {
	    body->vel.x -= vel;
	}
	if (input.keys[Key_D]) {
	    body->vel.x += vel;
	}

	trans.setPosition(pos.getVec3());

	if (input.mouse_buttons[MouseButton_Right]) {

	    static Time last = 0.0;
	    static u32 seed = 0x3453F23;
	    Time now = timer_now();
	    if (now - last > 0.05f) {

		last = now;
		
		Entity e = create_entity(scene);
		add_component<SpriteComponent>(scene, e)->color = { (u8)math_random_u32(seed++, 256u), (u8)math_random_u32(seed++, 256u), (u8)math_random_u32(seed++, 256u), 255u };
		BodyComponent* b = add_component<BodyComponent>(scene, e);
		b->body_type = BodyType_Dynamic;
		b->bounciness = 0.7f;
		b->mass = 0.05f;

		Transform t = get_entity_transform(scene, e);

		CameraComponent* cam = get_main_camera(scene);

		v2_f32 cam_pos;

		if (cam)
		    cam_pos = input.mouse_position * v2_f32(cam->width, cam->height);

		b->vel = cam_pos - pos;
		b->vel.normalize();
		b->vel *= 100.f;
		t.setPosition(pos.getVec3());
		t.setScale({ 0.07f, 0.07f, 1.f });
	    }
	}

	if (input.keys[Key_E] == InputState_Pressed) {
	    explosion(engine.scene, pos, 100.f, 40.f, &m.player, 1u);
	}
    }
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "worlds/%s.scene", name);
    return true;
}
