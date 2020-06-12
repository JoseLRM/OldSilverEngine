#pragma once

#include "core.h"

#ifdef SV_IMGUI

#include "external/ImGui/imgui.h"

namespace SV {
	class Window;
	class Graphics;
	class Scene;
	class FrameBuffer;
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

	void ShowGraphics(const SV::Graphics& graphics, bool* open);
	void ShowScene(SV::Scene& scene, SV::Entity* selectedEntity, bool* open);
}

#endif