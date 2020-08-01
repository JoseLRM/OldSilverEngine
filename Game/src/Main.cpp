#include "GameState.h"

class Loading : public sv::LoadingState {

public:

	void Initialize() override
	{

	}

	void Update(float dt) override
	{
		
	}
	void FixedUpdate() override
	{

	}

	void Render()
	{
	}

	void Close() override
	{
	}

};

void System(sv::Scene& scene, sv::Entity entity, sv::BaseComponent** comp, float dt)
{
	sv::SpriteComponent& sprComp = *reinterpret_cast<sv::SpriteComponent*>(comp[0]);
	sv::Transform trans = scene.GetTransform(entity);
	sv::vec3 rot = trans.GetLocalRotation();

	ui32 type = entity % 3;
	
	switch (type)
	{
	case 0:
		rot.z += dt * ToRadians(5.f);
		break;
	case 1:
		rot.z -= dt * ToRadians(50.f);
		break;
	case 2:
		rot.z += dt * ToRadians(180.f);
		break;
	}

	trans.SetRotation(rot);

}

class Application : public sv::Application
{
	sv::OrthographicCamera camera;
	sv::Scene scene;
	sv::Entity entity;

	sv::TextureAtlas texture;
	sv::Sprite sprite;

	sv::RenderLayerID transparentLayer;


public:

	void createEntity()
	{
		constexpr ui32 count = 1;
		sv::Entity entities[count];

		scene.CreateEntities(count, 0u, entities);
		scene.AddComponents<sv::SpriteComponent>(entities, count, sprite);

		for (ui32 i = 0; i < count; ++i) {
			sv::Transform trans = scene.GetTransform(entities[i]);
			sv::SpriteComponent& sprComp = *scene.GetComponent<sv::SpriteComponent>(entities[i]);

			sv::vec3 pos;
			sv::vec2 mousePos = camera.GetMousePos();
			pos.x = mousePos.x;
			pos.y = mousePos.y;
			//pos.x = (float(rand()) / RAND_MAX) * 800.f - 400.f;
			//pos.y = (float(rand()) / RAND_MAX) * 400.f - 200.f;

			sv::vec3 scale;
			scale.x = (float(rand()) / RAND_MAX) + 0.1f;
			scale.y = (float(rand()) / RAND_MAX) + 0.1f;

			sv::vec3 rot;
			rot.z = ToRadians((float(rand()) / RAND_MAX) * 360.f);

			sprComp.color.x = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.y = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.z = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.w = 255u;

			trans.SetPosition(pos);
			trans.SetScale(scale);
			trans.SetRotation(rot);

		}
	}

	void createEntities()
	{
		constexpr ui32 count = 1000;
		sv::Entity entities[count];

		scene.CreateEntities(count, 0u, entities);
		scene.AddComponents<sv::SpriteComponent>(entities, count, sprite, transparentLayer);

		for (ui32 i = 0; i < count; ++i) {
			sv::Transform trans = scene.GetTransform(entities[i]);
			sv::SpriteComponent& sprComp = *scene.GetComponent<sv::SpriteComponent>(entities[i]);

			sv::vec3 pos;
			sv::vec3 scale;
			sv::vec3 rot;

//#define TEST0

#ifdef TEST0
			sv::vec2 mousePos = camera.GetMousePos();
			pos.x = mousePos.x;
			pos.y = mousePos.y;

			scale.x = 1.f;
			scale.y = 1.f;
#else
			pos.x = (float(rand()) / RAND_MAX) * 40.f - 30.f;
			pos.y = (float(rand()) / RAND_MAX) * 20.f - 10.f;

			scale.x = (float(rand()) / RAND_MAX) + 0.1f;
			scale.y = (float(rand()) / RAND_MAX) + 0.1f;

			rot.z = ToRadians((float(rand()) / RAND_MAX) * 360.f);

			sprComp.color.x = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.y = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.z = ui8((float(rand()) / RAND_MAX) * 255.f);
			sprComp.color.w = 100u;
#endif

			trans.SetPosition(pos);
			trans.SetScale(scale);
			trans.SetRotation(rot);

		}
	}

	void Initialize() override
	{
		sv::engine_state_load(new GameState(), new Loading());
		scene.Initialize();

		texture.CreateFromFile("res/Tileset.png", false, SV_GFX_ADDRESS_MODE_WRAP);
		sprite = texture.AddSprite(0.1f, 0.f, 0.1f, 1.f / 6.f);

		transparentLayer = sv::renderer_layer_create(10, SV_REND_SORT_MODE_NONE, true);

		entity = scene.CreateEntity();
		scene.AddComponent<sv::SpriteComponent>(entity, sprite, SV_COLOR_WHITE);
	}

	void Update(float dt) override
	{

		camera.Adjust();

		sv::vec2 dir;
		float dirZoom = 0u;
		float add = 7.f * dt * camera.GetZoom() * 0.05f;
		float addZoom = dt * 10.f * (camera.GetZoom()*0.05f);

		if (sv::input_key('W')) {
			dir.y += add;
		}
		if (sv::input_key('S')) {
			dir.y -= add;
		}
		if (sv::input_key('D')) {
			dir.x += add;
		}
		if (sv::input_key('A')) {
			dir.x -= add;
		}
		if (sv::input_key(SV_KEY_SPACE)) {
			dirZoom += addZoom;
		}
		if (sv::input_key(SV_KEY_SHIFT)) {
			dirZoom -= addZoom;
		}
		camera.SetZoom(camera.GetZoom() + dirZoom);
		camera.SetPosition(camera.GetPosition() + dir);


		sv::Transform trans = scene.GetTransform(entity);
		sv::vec3 rot = trans.GetLocalRotation();

		if (sv::input_key('I')) {
			rot.x += dt * 2.f;
		}
		if (sv::input_key('O')) {
			rot.y += dt * 2.f;
		}
		if (sv::input_key('P')) {
			rot.z += dt * 2.f;
		}

		trans.SetRotation(rot);


		if (sv::input_mouse_pressed(SV_MOUSE_LEFT)) {
			createEntity();
			sv::log("Entities: %u", scene.GetEntityList().size());
		}
		if (sv::input_mouse_pressed(SV_MOUSE_CENTER)) {
			createEntities();
			sv::log("Entities: %u", scene.GetEntityList().size());
		}

		sv::CompID req[] = {
			sv::SpriteComponent::ID
		};
		SV_ECS_SYSTEM_DESC system;
		
		system.executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		system.optionalComponentsCount = 0u;
		system.requestedComponentsCount = 1u;
		system.system = System;
		system.pRequestedComponents = req;
		if (sv::input_mouse(SV_MOUSE_RIGHT))
			scene.ExecuteSystems(&system, 1u, dt);

		if (sv::input_key_pressed(SV_KEY_ESCAPE)) sv::engine_request_close();
		if (sv::input_key_pressed('F')) sv::window_fullscreen_set(!sv::window_fullscreen_get());
	}
	void Render() override
	{
		sv::renderer_scene_begin();
		sv::renderer_draw_scene(scene);
		sv::renderer_scene_end();

		sv::renderer_present(camera);
	}
	void Close() override
	{
	}
};

int main()
{
	Application app;

	SV_ENGINE_INITIALIZATION_DESC desc;
	desc.rendererDesc.resolutionWidth = 1920u;
	desc.rendererDesc.resolutionHeight = 1080u;
	desc.windowDesc.x = 200u;
	desc.windowDesc.y = 200u;
	desc.windowDesc.width = 1280u;
	desc.windowDesc.height = 720;
	desc.windowDesc.fullscreen = false;
	desc.windowDesc.title = L"SilverEngine";
	desc.showConsole = true;
	desc.minThreadsCount = 2u;

	sv::engine_initialize(&app, &desc);
	while (sv::engine_loop());
	sv::engine_close();

	system("PAUSE");
	return 0;
}