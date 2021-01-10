#include "game.h"

#include "core/window.h"

#include "spacestate.h"

std::unique_ptr<GameState> g_State;

Result game_initialize()
{
	g_State = std::make_unique<SpaceState>();

	return g_State->initialize();
}

void game_update(f32 dt)
{
#ifdef FPS_MODE
	static f32 fpsTime = 0.f;
	static u32 fpsCount = 0u;

	fpsTime += dt;
	++fpsCount;

	if (fpsTime > 0.25f) {

		std::wstring title = GAME_TITLE;
		title += L" || FPS: ";
		title += std::to_wstring(fpsCount);

		fpsCount = 0u;
		fpsTime -= 0.25f;

		window_title_set(title.c_str());
	}

#endif

	// Adjust camera
	CameraProjection& camera = g_State->getCamera();
	camera.adjust(window_aspect_get());
	camera.updateMatrix(); // TODO: Use the event system

	// Update State
	g_State->update(dt);

	// Fixed Update
	{
		static f32 stepCount = 0.f;
		stepCount += dt;

		while (stepCount >= 1.f / 60.f) {
			stepCount -= 1.f / 60.f;

			g_State->fixedUpdate();
		}
	}
}

void game_render()
{
	g_State->render();
}

Result game_close()
{
	return g_State->close();
}


