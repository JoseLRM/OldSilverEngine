#include <SilverEngine>

class Application : public SV::Application
{
	SV::OrthographicCamera camera;
	SV::Scene scene;

	SV::Entity player;

	bool openSceneWindow;
	SV::Entity selectedSceneWindow = SV_INVALID_ENTITY;

public:
	void Initialize() override
	{
		GetEngine().GetRenderer().SetCamera(&camera);
		camera.SetDimension(SV::vec2(100));
		camera.SetAspect(GetEngine().GetRenderer().GetResolutionAspect());

		scene.Initialize();

		player = scene.CreateEntity();
		scene.AddComponent(player, SV::SpriteComponent());
		scene.AddComponent(player, SV::NameComponent("Player"));
	}
	void Update(float dt) override
	{
		if (GetInput().IsMouse(SV_MOUSE_LEFT)) {
			SV::vec3 mousePos = SV::vec3(camera.GetMousePos(GetInput()));
			SV::Entity entity = scene.CreateEntity();
			scene.AddComponent(entity, SV::SpriteComponent());
			scene.AddComponent(entity, SV::NameComponent("Player"));
			scene.GetTransform(entity).SetPosition(mousePos);
		}
	}
	void Render() override
	{
		SV::Renderer& renderer = GetRenderer();

		renderer.DrawScene(scene);

		SVImGui::ShowScene(scene, &selectedSceneWindow , &openSceneWindow);
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