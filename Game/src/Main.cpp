#include "ColisionSystem.h"

class Application : public SV::Application
{
	SV::OrthographicCamera camera;
	SV::Scene scene;

	SV::Entity player;
	SV::Entity ground;

	ColisionSystem colisionsSystem;

	bool openSceneWindow;
	SV::Entity selectedSceneWindow = SV_INVALID_ENTITY;

	SV::TextureAtlas texture;
	SV::Sprite sprite;

	SV::Entity tileMap;

	SV::RenderLayer editorRL;

public:
	void Initialize() override
	{
		GetEngine().GetRenderer().SetCamera(&camera);
		camera.SetDimension(SV::vec2(10));
		camera.SetAspect(GetEngine().GetRenderer().GetResolutionAspect());

		texture.Create("res/Tileset.png", GetGraphics());
		texture.SetSamplerState(SV_GFX_TEXTURE_ADDRESS_WRAP, SV_GFX_TEXTURE_FILTER_MIN_MAG_MIP_POINT, GetGraphics());

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

		scene.Initialize();

		player = scene.CreateEntity();
		scene.AddComponent(player, SV::SpriteComponent(s1));
		scene.AddComponent(player, SV::NameComponent("Player"));
		scene.AddComponent(player, PhysicsComponent());

		ground = scene.CreateEntity();
		scene.AddComponent(ground, SV::SpriteComponent());
		scene.AddComponent(ground, SV::NameComponent("Ground"));
		scene.AddComponent(ground, ColisionComponent());
		scene.GetTransform(ground).SetPosition({0.f, -3.f, 0.f });
		scene.GetTransform(ground).SetScale({100.f, 2.f, 0.f });

		tileMap = scene.CreateEntity();
		scene.AddComponent(tileMap, SV::NameComponent("TileMap"));
		scene.AddComponent(tileMap, SV::TileMapComponent());

		editorRL.SetValue(99999);
		editorRL.SetTransparent(true);
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
				tm.PutTile(x, y, (x % 2 == 0) ? tile : tile-1);
			}
		}
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

	void CameraController(float dt)
	{
		static float threshold = 0.05f;
		static float exp = 4.f;
		static float limit = 500.f;

#ifdef SV_IMGUI
		//if (ImGui::Begin("Camera")) {
		//	ImGui::DragFloat("Threshold", &threshold, 0.01f);
		//	ImGui::DragFloat("Exponential", &exp, 0.01f);
		//	ImGui::DragFloat("Limit", &limit, 0.01f);
		//}
		//ImGui::End();
#endif

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
		MovePlayer(dt);
		CameraController(dt);

		scene.UpdateSystem(&colisionsSystem, dt);

		if(GetInput().IsKeyPressed(SV_KEY_F11)) GetGraphics().SetFullscreen(!GetGraphics().InFullscreen());

		if (GetInput().IsMousePressed(SV_MOUSE_CENTER)) {
			std::vector<SV::Entity> entities;
			SV::vec3 mousePos = SV::vec3(camera.GetMousePos(GetInput()));
			scene.CreateEntities(1, 0, &entities);
			scene.AddComponents(entities, SV::SpriteComponent(sprite));
			scene.AddComponents(entities, SV::NameComponent("Player"));
			scene.AddComponents(entities, PhysicsComponent());
			for (ui32 i = 0; i < entities.size(); ++i) {
				SV::Entity entity = entities[i];
				SV::SpriteComponent* sprComp = scene.GetComponent<SV::SpriteComponent>(entity);
				float r = (float(rand()) / RAND_MAX);
				float g = (float(rand()) / RAND_MAX);
				float b = (float(rand()) / RAND_MAX);
				sprComp->color = SVColor4f::ToColor4b({ r, g, b ,1.f});
				scene.GetTransform(entity).SetPosition(mousePos);
			}
		}
	}
	void Render() override
	{
		SV::Renderer& renderer = GetRenderer();

		renderer.DrawScene(scene);

		editorRL.Reset();
		SV::RenderQueue2D& rq = renderer.GetRenderQueue2D();
		rq.AddLayer(editorRL);

#ifdef SV_IMGUI
		SVImGui::ShowTileMapEditor(scene.GetComponent<SV::TileMapComponent>(tileMap), GetInput(), camera, rq, editorRL);
		SVImGui::ShowScene(scene, &selectedSceneWindow , &openSceneWindow);
		SVImGui::ShowImGuiDemo();
#endif
	}
	void Close() override
	{
		scene.Close();
	}
};

int main()
{
	{
		SV_ENGINE_STATIC_INITIALIZATION_DESC desc;
		desc.SetDefault();
		desc.showConsole = true;

		SV::Initialize(&desc);
	}

	SV::Engine engine;
	Application app;

	//SV::Engine engine2;
	//Application app2;

	SV_ENGINE_INITIALIZATION_DESC desc;
	desc.SetDefault();
	desc.rendererDesc.windowAttachment.resolution = 1920u;
	desc.executeInThisThread = true;

	engine.Run(&app, &desc);
	//engine2.Run(&app2);

	SV::Close();
	system("PAUSE");
	return 0;
}