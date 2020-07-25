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
	ui32 data;
	PeneComponent() = default;
	PeneComponent(ui32 d) : data(d) {}
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
		SV::Entity entities[count];

		scene.CreateEntities(count, SV_INVALID_ENTITY, entities);

		scene.AddComponents(entities, count, TestComponent(10));

		for (ui32 i = 0; i < count; ++i) {
			ui32 n = ui32(float(rand()) / RAND_MAX * 1000.f);
			if(i % 30 == 0)
				scene.AddComponent(entities[i], PeneComponent(n));
		}
	}

	void Update(float dt) override
	{
		SV::CompID requestedComponents[] = {
			TestComponent::ID
		};
		SV::CompID optionalComponents[] = {
			PeneComponent::ID
		};
		
		SV_ECS_SYSTEM_DESC desc[2];
		if(SV::Engine::IsMouse(SV_MOUSE_LEFT))
			desc[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_PARALLEL;
		else desc[0].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		desc[0].pRequestedComponents = requestedComponents;
		desc[0].pOptionalComponents = optionalComponents;
		desc[0].requestedComponentsCount = 1u;
		desc[0].optionalComponentsCount = 1u;

		desc[0].system = [](SV::Scene& scene, SV::Entity entity, SV::BaseComponent** comp, float dt) {
			TestComponent* testComp = reinterpret_cast<TestComponent*>(comp[0]);
			//SV::Log("1-> Entity %u: %u", entity, testComp->data);

			PeneComponent* peneComp = reinterpret_cast<PeneComponent*>(comp[1]);
			if (peneComp) {
				//SV::Log("1-> PeneEntity %u: %u", entity, peneComp->data);
			}
		};

		desc[1].executionMode = SV_ECS_SYSTEM_EXECUTION_MODE_SAFE;
		desc[1].pRequestedComponents = requestedComponents;
		desc[1].pOptionalComponents = optionalComponents;
		desc[1].requestedComponentsCount = 1u;
		desc[1].optionalComponentsCount = 1u;

		desc[1].system = [](SV::Scene& scene, SV::Entity entity, SV::BaseComponent** comp, float dt) {
			TestComponent* testComp = reinterpret_cast<TestComponent*>(comp[0]);
			//SV::Log("2-> Entity %u: %u", entity, testComp->data);

			PeneComponent* peneComp = reinterpret_cast<PeneComponent*>(comp[1]);
			if (peneComp) {
				//SV::Log("2-> PeneEntity %u: %u", entity, peneComp->data);
			}
		};

		//SV::LogSeparator();
		scene.ExecuteSystems(desc, 2u, dt);

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