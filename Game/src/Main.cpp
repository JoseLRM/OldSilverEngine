#include <SilverEngine>

class Application : public SV::Application
{
	SV::OrthographicCamera camera;
	SV::Scene scene;

	SV::Entity player;

	bool openSceneWindow;
	SV::Entity selectedSceneWindow = SV_INVALID_ENTITY;

	SV::TextureAtlas texture;
	SV::Sprite sprite;

public:
	void Initialize() override
	{
		GetEngine().GetRenderer().SetCamera(&camera);
		camera.SetDimension(SV::vec2(10));
		camera.SetAspect(GetEngine().GetRenderer().GetResolutionAspect());

		texture.Create("res/Skybox.jpg", GetGraphics());
		sprite = texture.CreateSprite(0.f, 0.f, 1.f, 1.f);

		scene.Initialize();

		player = scene.CreateEntity();
		scene.AddComponent(player, SV::SpriteComponent(sprite));
		scene.AddComponent(player, SV::NameComponent("Player"));
	}

	void MovePlayer(float dt)
	{
		SV::Input& input = GetInput();
		SV::Transform& playerTrans = scene.GetTransform(player);
		float force = 10.f * dt;

		SV::vec3 position = playerTrans.GetLocalPosition();

		if (input.IsKey('D')) {
			position.x += force;
		}
		if (input.IsKey('A')) {
			position.x -= force;
		}
		if (input.IsKey('W')) {
			position.y += force;
		}
		if (input.IsKey('S')) {
			position.y -= force;
		}

		//position.Normalize();
		//position *= force;

		playerTrans.SetPosition(position);
	}

	void CameraController(float dt)
	{
		static float threshold = 1.f;
		static float exp = 2.f;
		static float limit = 10.f;

#ifdef SV_IMGUI
		if (ImGui::Begin("Camera")) {
			ImGui::DragFloat("Threshold", &threshold, 0.01f);
			ImGui::DragFloat("Exponential", &exp, 0.01f);
			ImGui::DragFloat("Limit", &limit, 0.01f);
		}
		ImGui::End();
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

		if(GetInput().IsKeyPressed(SV_KEY_F11)) GetGraphics().SetFullscreen(!GetGraphics().InFullscreen());

		if (GetInput().IsMousePressed(SV_MOUSE_LEFT)) {
			std::vector<SV::Entity> entities;
			SV::vec3 mousePos = SV::vec3(camera.GetMousePos(GetInput()));
			scene.CreateEntities(1, 0, &entities);
			scene.AddComponents(entities, SV::SpriteComponent(sprite));
			scene.AddComponents(entities, SV::NameComponent("Player"));
			for (ui32 i = 0; i < entities.size(); ++i) {
				SV::Entity entity = entities[i];
				SV::SpriteComponent* sprComp = scene.GetComponent<SV::SpriteComponent>(entity);
				float r = (float(rand()) / RAND_MAX);
				float g = (float(rand()) / RAND_MAX);
				float b = (float(rand()) / RAND_MAX);
				sprComp->color = SVColor4f::ToColor4b({ r, g, b ,1.f});
				float r0 = (float(rand()) / RAND_MAX) * 5.f -2.5f;
				float r1 = (float(rand()) / RAND_MAX) * 5.f -2.5f;
				scene.GetTransform(entity).SetPosition(mousePos);
			}
		}
	}
	void Render() override
	{
		SV::Renderer& renderer = GetRenderer();

		renderer.DrawScene(scene);

#ifdef SV_IMGUI
		SVImGui::ShowScene(scene, &selectedSceneWindow , &openSceneWindow);
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
	desc.executeInThisThread = true;

	engine.Run(&app, &desc);
	//engine2.Run(&app2);

	SV::Close();
	system("PAUSE");
	return 0;
}