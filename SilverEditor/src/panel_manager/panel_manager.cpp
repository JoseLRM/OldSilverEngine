#include "core_editor.h"

#include "panel_manager.h"

#include "panels/SimulationPanel.h"
#include "panels/SceneEditorPanel.h"
#include "panels/SceneHierarchyPanel.h"
#include "panels/EntityInspectorPanel.h"
#include "panels/AssetPanel.h"
#include "panels/SimulationToolsPanel.h"

namespace sv {

	std::unordered_map<std::string, Panel*> g_Viewports;

	Result viewport_initialize()
	{
		viewport_add("Simulation", new SimulationViewport());
		viewport_add("SceneEditor", new SceneEditorViewport());
		viewport_add("SceneHierarchy", new SceneHierarchyViewport());
		viewport_add("EntityInspector", new EntityInspectorViewport());
		viewport_add("Asset", new AssetPanel());
		viewport_add("SimulationTools", new SimulationToolsViewport());

		return Result_Success;
	}

	Result viewport_close()
	{
		for (auto vp : g_Viewports)
		{
			delete vp.second;
		}
		g_Viewports.clear();

		return Result_Success;
	}

	void viewport_display()
	{
		for (auto vp : g_Viewports)
		{
			vp.second->display();
		}
	}

	void viewport_add(const char* name, Panel* viewport)
	{
		Panel* vp = viewport_get(name);
		if (vp != nullptr) {
			SV_LOG_INFO("The viewport '%s' already exist", name);
			return;
		}

		g_Viewports[name] = viewport;
	}

	Panel* viewport_get(const char* name)
	{
		auto it = g_Viewports.find(name);
		if (g_Viewports.end() == it) return nullptr;
		return it->second;
	}

	void viewport_rmv(const char* name)
	{
		auto it = g_Viewports.find(name);
		if (g_Viewports.end() == it) {
			SV_LOG_INFO("Can't destroy the viewport '%s', doesn't exist", name);
			return;
		}
		delete it->second;
		g_Viewports.erase(it);
	}

}