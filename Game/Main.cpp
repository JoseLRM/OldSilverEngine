#include <SilverEngine>

SVDefineTag(TestTag);

class SystemTest : public SV::System {

public:
	SystemTest()
	{
		AddRequestedComponent(SV::NameComponent::ID);
		AddOptionalComponent(TestTag::ID);

		SetCollectiveSystem();
	}

	void UpdateEntity(SV::Entity entity, SV::BaseComponent** comp, SV::Scene& scene, float dt) override
	{
		SV::NameComponent* nameComp = reinterpret_cast<SV::NameComponent*>(comp[0]);
		SV::LogI("Name Comp from system: %s", nameComp->GetName().c_str());
		if (comp[1]) SV::LogI("Has TestTag :)");
	}

	void UpdateEntities(std::vector<SV::BaseComponent**>& comp, SV::Scene& scene, float dt) override
	{
		for (ui32 i = 0; i < comp.size(); ++i) {
			SV::NameComponent* nameComp = reinterpret_cast<SV::NameComponent*>(comp[i][0]);
			//SV::LogI("Name Comp from system: %s", nameComp->GetName().c_str());
			//if (comp[i][1]) SV::LogI("Has TestTag :)");

			SV::Entity entity = nameComp->entity;
			SV::LogI("Layer name = %s", scene.GetLayer(entity)->name.c_str());
		}
	}

};

class Application : public SV::Application
{
	SV::RenderLayer layer;
	SV::OrthographicCamera camera;
	SV::Scene scene;
	SystemTest system;

public:
	void Initialize() override
	{
		GetEngine().GetRenderer().SetCamera(&camera);

		camera.SetDimension(SV::vec2(1280.f, 720.f));

		scene.Initialize();

		scene.CreateLayer("Test", 69);

		SV::Entity entity = scene.CreateEntity();
		scene.CreateEntity();

		scene.SetLayer(entity, scene.GetLayer("Teste"));

		scene.AddComponent(entity, SV::NameComponent());
		scene.AddComponent(entity, TestTag());

		std::vector<SV::Entity> entities;
		scene.CreateEntities(100, entity, &entities);

		scene.AddComponents(entities, SV::NameComponent());

		for (ui32 i = 0; i < entities.size(); ++i) {
			SV::NameComponent* nameComp = scene.GetComponent<SV::NameComponent>(entities[i]);

			nameComp->SetName(std::string("Hello :) ") + std::to_string(i));
			SV::LogI("Entity from comp -> %s", nameComp->GetName().c_str());
		}
	}
	void Update(float dt) override
	{
		scene.UpdateSystem(&system, dt);
		SV::LogSeparator();
	}
	void Render() override
	{
		SV::RenderQueue2D& rq = GetEngine().GetRenderer().GetRenderQueue2D();
		layer.Reset();
		rq.AddLayer(layer);

		SV::Input& input = GetEngine().GetInput();

		SV::vec2 mousePos = camera.GetMousePos(input);

		float size = 25.f;
		float halfSize = size / 2.f;

		ui32 reserve = (camera.GetDimension().x / size) * (camera.GetDimension().y / size);
		rq.ReserveQuads(reserve, layer);

		float l0 = 50;
		float l1 = 175.f;

		for (float x = -camera.GetDimension().x / 2.f; x < camera.GetDimension().x / 2.f; x += size) {
			for (float y = -camera.GetDimension().y / 2.f; y < camera.GetDimension().y / 2.f; y += size) {
				ui8 r = SV::Math::FloatColorToByte((y + camera.GetDimension().y / 2.f) / camera.GetDimension().y);
				ui8 g = 255u;
				ui8 b = SV::Math::FloatColorToByte((x + camera.GetDimension().x / 2.f) / camera.GetDimension().x);

				SV::vec2 pos = SV::vec2(x + halfSize, y + halfSize);
				SV::vec2 toMouse = mousePos - pos;
				float distance = toMouse.Mag();
				
				if (distance < l0) g = 0u;
				else if (distance < l1) {
					float d = (distance - l0) / (l1 - l0);
					g = SV::Math::FloatColorToByte(d);
				}

				rq.DrawQuad(pos, SV::vec2(size * 0.5f), SV::Color4b(r, g, b, 255), layer);
			}
		}
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