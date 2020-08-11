#include "core_editor.h"

#include "viewport_manager.h"

namespace sve {

	static std::unordered_map<std::string, Viewport> g_Viewports;

	void viewport_manager_add(const char* name, DisplayFunction displayFn)
	{
#ifdef SV_DEBUG
		auto it = g_Viewports.find(name);
		if (it != g_Viewports.end()) {
			SV_ASSERT(false);
		}
#endif
		Viewport viewport;
		viewport.displayFn = displayFn;
		viewport.show = false;

		g_Viewports[name] = viewport;
	}

	void viewport_manager_display()
	{
		for (auto& it : g_Viewports) {
			Viewport& viewport = it.second;
			if (!viewport.show) continue;

			if (ImGui::Begin(it.first.c_str())) {

				ImVec2 pos = ImGui::GetWindowPos();
				ImVec2 size = ImGui::GetWindowSize();

				viewport.properties.x = pos.x;
				viewport.properties.y = pos.y;
				viewport.properties.width = size.x;
				viewport.properties.height = size.y;
				viewport.properties.focus = ImGui::IsWindowFocused();

				if (!viewport.displayFn()) {
					viewport_manager_hide(it.first.c_str());
				}
			}
			ImGui::End();
		}
	}

	void viewport_manager_show(const char* name)
	{
		auto it = g_Viewports.find(name);
		if (it == g_Viewports.end()) {
			sv::log_error("Viewport '%s' not found", name);
			return;
		}

		sv::log_info("Showing viewport '%s'", name);
		it->second.show = true;
	}

	void viewport_manager_hide(const char* name)
	{
		auto it = g_Viewports.find(name);
		if (it == g_Viewports.end()) {
			sv::log_error("Viewport '%s' not found", name);
			return;
		}

		sv::log_info("Hiding viewport inactive '%s'", name);
		it->second.show = false;
	}

	ViewportProperties viewport_manager_properties_get(const char* name)
	{
		auto it = g_Viewports.find(name);
		if (it == g_Viewports.end()) {
			sv::log_error("Viewport '%s' not found", name);
			return {};
		}

		return it->second.properties;
	}

	bool& viewport_manager_get_show(const char* name)
	{
		return g_Viewports[name].show;
	}

}