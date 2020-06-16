#pragma once

#include "core.h"

#ifdef SV_IMGUI

#include "external/ImGui/imgui.h"

namespace SV {
	class Window;
	class Graphics;
	class Input;
	class Scene;
	class TileMapComponent;
	class RenderQueue2D;
	class RenderLayer;
	class OrthographicCamera;
	typedef ui32 Entity;
}

namespace SVImGui {
	namespace _internal {
		bool Initialize(SV::Window& window, SV::Graphics& graphics);
		void BeginFrame();
		void EndFrame();

		i64 UpdateWindowProc(void* handle, ui32 msg, i64 wParam, i64 lParam);
		bool Close();
	}

	void ShowGraphics(SV::Graphics& graphics, bool* open);
	void ShowScene(SV::Scene& scene, SV::Entity* selectedEntity, bool* open);
	void ShowTileMapEditor(SV::TileMapComponent* tileMap, SV::Input& input, SV::OrthographicCamera& camera, SV::RenderQueue2D& rq, SV::RenderLayer& layer);
	void ShowImGuiDemo();

}

#endif