#include "GameState.h"

class Loading : public SV::LoadingState {

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

SV_COMPONENT(TestComponent) {
	ui32 data;
	TestComponent() = default;
	TestComponent(ui32 d) : data(d) {}
};

SV_COMPONENT(PeneComponent) {
	SV::Entity data;
	PeneComponent() = default;
	PeneComponent(SV::Entity e) : data(e) {}
};

class Application : public SV::Application
{
	SV::OrthographicCamera camera;
	SV::Scene scene;

public:
	void Initialize() override
	{
		SV::StateManager::LoadState(new GameState(), new Loading());
		scene.Initialize();

		constexpr ui32 count = 1000;
		constexpr ui32 childsCount = 10;

		SV::Entity entities[count];

		scene.CreateEntities(count, SV_INVALID_ENTITY, entities);

		for (ui32 i = 0; i < count; ++i) {
			SV::Entity childs[childsCount];
			scene.CreateEntities(childsCount, entities[i], childs);
			scene.AddComponents<PeneComponent>(childs, childsCount, entities[i]);
		}

		scene.AddComponents<TestComponent>(entities, count);
	}

	void Update(float dt) override
	{
		SV::CompID requestedComponents[] = {
			TestComponent::ID
		};
		SV::CompID optionalComponents[] = {
			PeneComponent::ID
		};
		
		SV_ECS_SYSTEM_DESC desc[1];
		if(SV::Engine::IsMouse(SV_MOUSE_LEFT))
			desc[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_MULTITHREADED;
		else
			desc[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		desc[0].pRequestedComponents = requestedComponents;
		desc[0].pOptionalComponents = optionalComponents;
		desc[0].requestedComponentsCount = 1u;
		desc[0].optionalComponentsCount = 0u;

		desc[0].system = [](SV::Scene& scene, SV::Entity entity, SV::BaseComponent** comp, float dt) {
			TestComponent* testComp = reinterpret_cast<TestComponent*>(comp[0]);
			//SV::Log("1-> Entity %u: %u", entity, testComp->data);

			SV::Transform trans = scene.GetTransform(entity);
			SV::vec3 p = trans.GetLocalPosition();
			p.x += dt;
			p.z -= dt * 2.f;

			trans.SetPosition(p);

			// Get Childs
			SV::Entity const* childs;
			ui32 childsCount;
			scene.GetEntityChilds(entity, &childs, &childsCount);

			for (ui32 i = 0; i < childsCount; ++i) {
				SV::Transform childTrans = scene.GetTransform(childs[i]);

				SV::vec3 pos = childTrans.GetWorldPosition();
				//SV::Log("Position: %f %f %f", pos.x, pos.y, pos.z);

				
			}
		};

		//SV::LogSeparator();
		scene.ExecuteSystems(desc, 1u, dt);

	}
	void Render() override
	{
	}
	void Close() override
	{
		
	}
};

int main()
{
	Application app;

	SV_ENGINE_INITIALIZATION_DESC desc;
	desc.rendererDesc.resolutionWidth = 1280u;
	desc.rendererDesc.resolutionHeight = 720;
	desc.windowDesc.x = 200u;
	desc.windowDesc.y = 200u;
	desc.windowDesc.width = 1280u;
	desc.windowDesc.height = 720;
	desc.windowDesc.title = L"SilverEngineTest";
	desc.showConsole = true;
	desc.minThreadsCount = 2u;

	SV::Engine::Initialize(&app, &desc);
	while (SV::Engine::Loop());
	SV::Engine::Close();

	system("PAUSE");
	return 0;
}