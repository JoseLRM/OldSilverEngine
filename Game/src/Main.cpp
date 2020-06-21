#include "GameState.h"

class Loading : public SV::LoadingState {

	SV::RenderLayer rl;
	SV::Engine& engine;

public:
	Loading(SV::Engine& engine) : engine(engine) {}

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
		rl.Reset();
		SV::RenderQueue2D& queue = engine.GetRenderer().GetRenderQueue2D();

		float time = SV::Timer::Now();
		float w = sin(time*5.f) * 3.f + 4.f;
		float h = cos(time*5.f) * 3.f + 4.f;

		ui32 count = 10;
		SV::Color4f minColor = SVColor4f::WHITE;
		SV::Color4f maxColor = SVColor4f::RED;

		float adv = 1.f / float(count);
		for (ui32 i = 0; i < count; ++i) {

			SV::Color4b color = SVColor4f::ToColor4b((minColor + (maxColor / count) * i) / 2.f);

			queue.DrawEllipse({ 0.f, 0.f }, SV::vec2(w * adv * i, h * adv * i), color, rl);
		}

		queue.AddLayer(rl);
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
		GetEngine().GetRenderer().SetCamera(&camera);
		camera.SetDimension(SV::vec2(10));
		camera.SetAspect(GetEngine().GetRenderer().GetResolutionAspect());

		GetStateManager().LoadState(new GameState(GetEngine()), new Loading(GetEngine()));
	}

	void Update(float dt) override
	{
		if (GetInput().IsKeyPressed(SV_KEY_F11)) GetGraphics().SetFullscreen(!GetGraphics().InFullscreen());
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