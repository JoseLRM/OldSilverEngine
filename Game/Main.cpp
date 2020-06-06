#include <SilverEngine>

class Application : public SV::Application
{
	SV::RenderLayer layer;
	SV::OrthographicCamera camera;

public:
	void Initialize() override
	{
		GetEngine().GetRenderer().SetCamera(&camera);

		camera.SetDimension(SV::vec2(1280.f, 720.f));
	}
	void Update(float dt) override
	{
	}
	void Render() override
	{
		SV::RenderQueue2D& rq = GetEngine().GetRenderer().GetRenderQueue2D();
		layer.Reset();
		rq.AddLayer(layer);

		SV::Input& input = GetEngine().GetInput();

		SV::vec2 mousePos = camera.GetMousePos(input);
		
		float size = 4.f;
		float halfSize = size / 2.f;

		ui32 reserve = (camera.GetDimension().x / size) * (camera.GetDimension().y / size);
		rq.ReserveQuads(reserve, layer);

		for (float x = -camera.GetDimension().x / 2.f; x < camera.GetDimension().x / 2.f; x += size) {
			for (float y = -camera.GetDimension().y / 2.f; y < camera.GetDimension().y / 2.f; y += size) {
				ui8 r = SV::Math::FloatColorToByte((y + camera.GetDimension().y / 2.f) / camera.GetDimension().y);
				ui8 g = 255u;
				ui8 b = SV::Math::FloatColorToByte((x + camera.GetDimension().x / 2.f) / camera.GetDimension().x);

				SV::vec2 pos = SV::vec2(x + halfSize, y + halfSize);
				SV::vec2 toMouse = mousePos - pos;
				float distance = toMouse.Mag();
				
				if (distance < 100.f) g = 0u;
				else if (distance < 200.f) {
					float d = (distance - 100.f) / 100.f;
					g = SV::Math::FloatColorToByte(d);
				}

				rq.DrawQuad(pos, SV::vec2(size * 0.5f), SV::Color4b(r, g, b, 255), layer);
			}
		}
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

	SV::Engine engine2;
	Application app2;

	SV_ENGINE_INITIALIZATION_DESC desc;
	desc.SetDefault();
	desc.executeInThisThread = false;

	engine.Run(&app, &desc);
	engine2.Run(&app2);

	SV::Close();
	//system("PAUSE");
	return 0;
}