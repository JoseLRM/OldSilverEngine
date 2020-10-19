#include "core_editor.h"

#include "viewport_manager.h"

#include "viewports/SimulationViewport.h"
#include "viewports/SceneEditorViewport.h"
#include "viewports/SceneHierarchyViewport.h"
#include "viewports/EntityInspectorViewport.h"
#include "viewports/AssetViewport.h"
#include "viewports/SimulationToolsViewport.h"

namespace sve {

	std::unordered_map<std::string, Viewport*> g_Viewports;

	sv::Result viewport_initialize()
	{
		viewport_add("Simulation", new SimulationViewport());
		viewport_add("SceneEditor", new SceneEditorViewport());
		viewport_add("SceneHierarchy", new SceneHierarchyViewport());
		viewport_add("EntityInspector", new EntityInspectorViewport());
		viewport_add("Asset", new AssetViewport());
		viewport_add("SimulationTools", new SimulationToolsViewport());

		return sv::Result_Success;
	}

	sv::Result viewport_close()
	{
		for (auto vp : g_Viewports)
		{
			delete vp.second;
		}
		g_Viewports.clear();

		return sv::Result_Success;
	}

	void viewport_display()
	{
		for (auto vp : g_Viewports)
		{
			vp.second->display();
		}
	}

	void viewport_add(const char* name, Viewport* viewport)
	{
		Viewport* vp = viewport_get(name);
		if (vp != nullptr) {
			SV_LOG_INFO("The viewport '%s' already exist", name);
			return;
		}

		g_Viewports[name] = viewport;
	}

	Viewport* viewport_get(const char* name)
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