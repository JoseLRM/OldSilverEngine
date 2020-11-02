#include "core_editor.h"

#include "panel_manager.h"

namespace sv {

	std::unordered_map<std::string, Panel*> g_Panels;

	Result panel_manager_initialize()
	{
		return Result_Success;
	}

	Result panel_manager_close()
	{
		for (auto vp : g_Panels)
		{
			delete vp.second;
		}
		g_Panels.clear();

		return Result_Success;
	}

	void panel_manager_display()
	{
		for (auto it = g_Panels.begin(); it != g_Panels.end(); )
		{
			if (!it->second->display()) {
				delete it->second;
				it = g_Panels.erase(it);
			}
			else ++it;
		}
	}

	void panel_manager_add(const char* name, Panel* panel)
	{
		Panel* vp = panel_manager_get(name);
		if (vp != nullptr) {
			SV_LOG_INFO("The viewport '%s' already exist", name);
			return;
		}

		g_Panels[name] = panel;
		panel->setName(name);
	}

	Panel* panel_manager_get(const char* name)
	{
		auto it = g_Panels.find(name);
		if (g_Panels.end() == it) return nullptr;
		return it->second;
	}

	void panel_manager_rmv(const char* name)
	{
		auto it = g_Panels.find(name);
		if (g_Panels.end() == it) {
			SV_LOG_INFO("Can't destroy the viewport '%s', doesn't exist", name);
			return;
		}
		delete it->second;
		g_Panels.erase(it);
	}

}