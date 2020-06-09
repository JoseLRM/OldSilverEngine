#include "core.h"

#include "Graphics_dx11.h"

#include "external/ImGui/imgui_impl_dx11.h"
#include "external/ImGui/imgui_impl_win32.h"
#include "ImGuiManager.h"

#ifdef SV_IMGUI

namespace SVImGui {

	ImGuiContext* g_ImGuiContext = nullptr;

	namespace _internal {
		bool Initialize(SV::Window& window, SV::Graphics& graphics)
		{
			SV::DirectX11Device& dx11 = *reinterpret_cast<SV::DirectX11Device*>(&graphics.GetDevice());

			IMGUI_CHECKVERSION();
			g_ImGuiContext = ImGui::CreateContext();

			if (g_ImGuiContext == nullptr) {
				SV::LogE("Can't create ImGui Context");
				return false;
			}

			if (!ImGui_ImplWin32_Init(window.GetWindowHandle())) {
				SV::LogE("Can't initialize ImGui Windows Impl");
				Close();
				return false;
			}
			if (!ImGui_ImplDX11_Init(dx11.device.Get(), dx11.immediateContext.Get())) {
				SV::LogE("Can't initialize ImGui DirectX11 Impl");
				Close();
				return false;
			}

			

			return true;
		}
		void BeginFrame()
		{
			if (g_ImGuiContext == nullptr) return;

			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		}
		void EndFrame()
		{
			if (g_ImGuiContext == nullptr) return;

			
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
		i64 UpdateWindowProc(void* handle, ui32 msg, i64 wParam, i64 lParam)
		{
			return ImGui_ImplWin32_WndProcHandler(reinterpret_cast<HWND>(handle), msg, wParam, lParam);
		}
		bool Close()
		{
			ImGui_ImplDX11_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext(g_ImGuiContext);
			return true;
		}
	}
	
	void ShowGraphics(const SV::Graphics& graphics, bool* open)
	{
		if (ImGui::Begin("Graphics")) {
			
		}
		ImGui::End();
	}
	
}

#endif