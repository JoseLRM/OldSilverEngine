#include "core.h"

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

}