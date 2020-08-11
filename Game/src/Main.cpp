#include "GameState.h"

class Loading : public sv::State {

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



class Application : public sv::Application
{
	

public:

	

	void Initialize() override
	{
		sv::engine_state_load(new GameState(), new Loading());
		

		
	}

	void Update(float dt) override
	{
		
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

	sv::InitializationDesc desc;
	desc.rendererDesc.resolutionWidth = 1920u;
	desc.rendererDesc.resolutionHeight = 1080u;
	desc.rendererDesc.outputMode = sv::RendererOutputMode_backBuffer;
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