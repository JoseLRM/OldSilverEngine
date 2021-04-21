#include "SilverEngine.h"

using namespace sv;

struct TestComponent : public BaseComponent {

    static CompID ID;
    static constexpr u32 VERSION = 0u;
    
    u32 data = 0u;

    void serialize(Archive& archive) { archive << data; }
    void deserialize(u32 version, Archive& archive) { archive >> data; }
};

CompID TestComponent::ID = SV_COMPONENT_ID_INVALID;

constexpr u32 VERSION = 0u;

struct GameMemory {

    Entity player;
    Entity cat;
    
};

GameMemory& get_game_memory()
{
    return *reinterpret_cast<GameMemory*>(engine.game_memory);
}

void update()
{
    GameMemory& m = get_game_memory();

    if (there_is_scene()) {
	
	BodyComponent* body = get_component<BodyComponent>(m.player);
	v2_f32& pos = *get_entity_position2D_ptr(m.player);

	set_entity_position(get_scene_data()->main_camera, pos.getVec3());

	f32 vel = 5.f * engine.deltatime;

	if (input.keys[Key_A]) {

	    SpriteComponent* spr = get_component<SpriteComponent>(m.player);
	    if (spr) {
		spr->texcoord.x = 0.1f;
		spr->texcoord.z = 0.f;
	    }
	    
	    pos.x -= vel;
	}
	if (input.keys[Key_D]) {

	    SpriteComponent* spr = get_component<SpriteComponent>(m.player);
	    if (spr) {
		spr->texcoord.z = 0.1f;
		spr->texcoord.x = 0.f;
	    }
	    
	    pos.x += vel;
	}

	if (body->in_ground) {

	    if (input.keys[Key_Space]) {
		body->vel.y = 27.f * 0.5f;
		
	    }
	    
	}
    }
}

void on_body_collision(void* _reg, BodyCollisionEvent* event)
{
    SV_LOG_INFO("All right");
}

SV_USER bool user_initialize(bool init)
{
    if (init) {
	engine.game_memory = SV_ALLOCATE_MEMORY(sizeof(GameMemory));
    
	set_scene("level_0");
    }

    register_component<TestComponent>("Test");

    event_user_register("on_body_collision", on_body_collision);
    event_user_register("update", update);
    
    return true;
}

SV_USER bool user_close(bool close)
{
    if (close)
	SV_FREE_MEMORY(engine.game_memory);

    invalidate_component_callbacks(TestComponent::ID);
    
    return true;
}

SV_USER bool user_initialize_scene(Archive* parchive)
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
    Entity& main_camera = get_scene_data()->main_camera;
    main_camera = create_entity(SV_ENTITY_NULL, "Camera");
    add_component<CameraComponent>(main_camera);

    // Create player
    m.player = create_entity(SV_ENTITY_NULL, "Player");
    SpriteComponent* spr = add_component<SpriteComponent>(m.player);
    load_asset_from_file(spr->texture, "assets/images/temp.png");
    spr->texcoord = { 0.f, 0.f, 0.1f, 0.1f  };
    BodyComponent* body = add_component<BodyComponent>(m.player);
    body->body_type = BodyType_Dynamic;

    // Create cat
    m.cat = create_entity(SV_ENTITY_NULL, "Cat");
    spr = add_component<SpriteComponent>(m.cat);
    spr->texcoord = { 0.f, 0.1f, 0.1f, 0.2f  };
    load_asset_from_file(spr->texture, "assets/images/temp.png");
    body = add_component<BodyComponent>(m.cat);
    body->body_type = BodyType_Dynamic;

    // Create block
    Entity block = create_entity(SV_ENTITY_NULL, "Block");
    spr = add_component<SpriteComponent>(block);
    spr->texcoord = { 0.8f, 0.f, 1.f, 0.2f  };
    load_asset_from_file(spr->texture, "assets/images/temp.png");
    add_component<BodyComponent>(block);    
    
    return true;
}

SV_USER bool user_serialize_scene(Archive* parchive)
{
    GameMemory& m = get_game_memory();
    Archive& archive = *parchive;
    archive << VERSION;
    archive << m.player;
    archive << m.cat;
    return true;
}

SV_USER bool user_get_scene_filepath(const char* name, char* filepath)
{
    sprintf(filepath, "assets/scenes/%s.scene", name);
    return true;
}

SV_USER bool user_validate_scene(const char* name)
{
    size_t size = strlen(name);

    if (size <= 6) return false;

    return true;
}
