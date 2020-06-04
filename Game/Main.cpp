#include <SilverEngine>

int main()
{
	SV::Initialize();

	SV::Engine engine;

	SV_ENGINE_INITIALIZATION_DESC desc;
	desc.SetDefault();
	desc.windowDesc.showConsole = true;

	engine.Run(&desc);

	SV::Close();

	return 0;
}