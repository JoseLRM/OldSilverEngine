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

class Application : public SV::Application
{
	SV::OrthographicCamera camera;

public:
	void Initialize() override
	{
		SV::StateManager::LoadState(new GameState(), new Loading());
	}

	void Update(float dt) override
	{
		//if (GetInput().IsKeyPressed(SV_KEY_F11)) GetGraphics().SetFullscreen(!GetGraphics().InFullscreen());
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