#include "SilverEngine/engine.h"

#include "game.h"

int main()
{
	InitializationDesc desc;
	desc.callbacks.initialize = game_initialize;
	desc.callbacks.close = game_close;
	desc.callbacks.update = game_update;
	desc.callbacks.render = game_render;
	desc.windowDesc.bounds = { 0u, 0u, 1080u, 720u };
	desc.windowDesc.title = GAME_TITLE;
	desc.windowDesc.iconFilePath = nullptr;
#ifdef FPS_MODE
	desc.windowDesc.state = WindowState_Windowed;
	desc.windowDesc.flags = WindowFlag_Default;
#else
	desc.windowDesc.state = WindowState_Fullscreen;
#endif
	desc.minThreadsCount = 2u;
	desc.assetsFolderPath = "assets/";

	Result res = engine_initialize(&desc);

	if (result_fail(res)) {
		printf("Can't initialize SilverEngine\n");
		return 1;
	}

	while (true) {
		res = engine_loop();

		if (res == Result_CloseRequest || result_fail(res)) break;
	}

	engine_close();

}