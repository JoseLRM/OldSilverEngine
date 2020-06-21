#pragma once

#include "ColisionSystem.h"

class GameState : public SV::State {

	SV::Scene scene;

	SV::Entity player;
	SV::Entity ground;

	ColisionSystem colisionsSystem;

	SV::TextureAtlas texture;
	SV::Sprite sprite;

	SV::Entity tileMap;

public:
	GameState(SV::Engine& engine) : State(engine) {}

	void Load() override
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
		texture.Create("res/Tileset.png", GetGraphics());
		texture.SetSamplerState(SV_GFX_TEXTURE_ADDRESS_WRAP, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_POINT, GetGraphics());

		scene.Initialize();

		float sprH = 1.f / 6.f;

		sprite = texture.CreateSprite(0.f, 0.f, 0.1f, sprH);
		SV::Sprite s0 = texture.CreateSprite(0.1f, sprH, 0.1f, sprH);
		SV::Sprite s1 = texture.CreateSprite(0.2f, sprH * 2.f, 0.1f, sprH);
		SV::Sprite s2 = texture.CreateSprite(0.f, 0.f, 0.1f, sprH);
		SV::Sprite s3 = texture.CreateSprite(0.1f, sprH, 0.1f, sprH);
		SV::Sprite s4 = texture.CreateSprite(0.2f, sprH * 2.f, 0.1f, sprH);
		SV::Sprite s5 = texture.CreateSprite(0.f, 0.f, 0.1f, sprH);
		SV::Sprite s6 = texture.CreateSprite(0.1f, sprH, 0.1f, sprH);
		SV::Sprite s7 = texture.CreateSprite(0.2f, sprH * 2.f, 0.1f, sprH);

		player = scene.CreateEntity();
		scene.AddComponent(player, SV::SpriteComponent(s1));
		scene.AddComponent(player, SV::NameComponent("Player"));
		scene.AddComponent(player, PhysicsComponent());

		ground = scene.CreateEntity();
		scene.AddComponent(ground, SV::SpriteComponent());
		scene.AddComponent(ground, SV::NameComponent("Ground"));
		scene.AddComponent(ground, ColisionComponent());
		scene.GetTransform(ground).SetPosition({ 0.f, -3.f, 0.f });
		scene.GetTransform(ground).SetScale({ 100.f, 2.f, 0.f });

		tileMap = scene.CreateEntity();
		scene.AddComponent(tileMap, SV::NameComponent("TileMap"));
		scene.AddComponent(tileMap, SV::TileMapComponent());

		SV::TileMapComponent* tmComp = scene.GetComponent<SV::TileMapComponent>(tileMap);
		SV::TileMap& tm = *tmComp->tileMap.get();

		tm.CreateTile(s1);
		SV::Tile tile = tm.CreateTile(sprite);
		tm.CreateTile(s0);
		tm.CreateTile(s2);
		tm.CreateTile(s3);
		tm.CreateTile(s4);
		tm.CreateTile(s5);
		tm.CreateTile(s6);
		tm.CreateTile(s7);
		tm.CreateGrid(10, 10);

		for (ui32 y = 0; y < 10; ++y) {
			for (ui32 x = 0; x < 10; ++x)
			{
				tm.PutTile(x, y, (x % 2 == 0) ? tile : tile - 1);
			}
		}
	}
	void Initialize() override 
	{
		
	}

	void MovePlayer(float dt)
	{
		SV::Input& input = GetInput();
		SV::Transform& playerTrans = scene.GetTransform(player);
		PhysicsComponent* physics = scene.GetComponent<PhysicsComponent>(player);
		float force = 10.f * dt;

		SV::vec3 position = playerTrans.GetLocalPosition();

		if (input.IsKey('D')) {
			position.x += force;
		}
		if (input.IsKey('A')) {
			position.x -= force;
		}
		if (input.IsKeyPressed(SV_KEY_SPACE)) {
			physics->vel.y = 20.f;
		}

		playerTrans.SetPosition(position);
	}

	void CameraController(SV::OrthographicCamera& camera, float dt)
	{
		constexpr float threshold = 0.05f;
		constexpr float exp = 4.f;
		constexpr float limit = 500.f;

		camera.Adjust(GetWindow());

		SV::Transform& playerTrans = scene.GetTransform(player);

		SV::vec2 playerPos = SV::vec2(playerTrans.GetLocalPosition());
		SV::vec2 position = camera.GetPosition();

		SV::vec2 toPlayer = playerPos - position;
		float distance = toPlayer.Mag();
		toPlayer /= distance;

		distance -= threshold;
		if (distance < 0.f) return;

		float force = pow(distance, exp);

		if (force > limit) {
			force = limit;
		}

		toPlayer *= force * dt;

		position += toPlayer;
		camera.SetPosition(position);
	}

	void Update(float dt) override
	{
		if (GetRenderer().GetCamera() == nullptr) return;
		SV::OrthographicCamera& camera = *reinterpret_cast<SV::OrthographicCamera*>(GetRenderer().GetCamera());

		MovePlayer(dt);
		CameraController(camera, dt);

		scene.UpdateSystem(&colisionsSystem, dt);

		if (GetInput().IsMousePressed(SV_MOUSE_CENTER)) {
			std::vector<SV::Entity> entities;
			SV::vec3 mousePos = SV::vec3(camera.GetMousePos(GetInput()));
			scene.CreateEntities(10000, 0, &entities);
			scene.AddComponents(entities, SV::SpriteComponent(sprite));
			scene.AddComponents(entities, SV::NameComponent("Player"));
			scene.AddComponents(entities, PhysicsComponent());
			for (ui32 i = 0; i < entities.size(); ++i) {
				SV::Entity entity = entities[i];
				SV::SpriteComponent* sprComp = scene.GetComponent<SV::SpriteComponent>(entity);
				float r = (float(rand()) / RAND_MAX);
				float g = (float(rand()) / RAND_MAX);
				float b = (float(rand()) / RAND_MAX);
				sprComp->color = SVColor4f::ToColor4b({ r, g, b ,1.f });
				scene.GetTransform(entity).SetPosition(mousePos);
			}
		}

		if (GetInput().IsKeyPressed('P')) GetStateManager().LoadState(new GameState(GetEngine()));
	}
	void Render() override
	{
		SV::Renderer& renderer = GetRenderer();
		renderer.DrawScene(scene);
	}
	void Unload() override
	{
		texture.Release(GetGraphics());
	}
	void Close() override
	{

	}

};