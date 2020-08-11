#include "core_editor.h"

#include "viewport_manager.h"
#include "renderer.h"

namespace sve {

	bool viewport_renderer2D_display()
	{
		ui32 count = sv::renderLayer_count_get();
		ImGui::Text("Render Layers");
		ImGui::Separator();
		ImGui::Text("Size = %u", count);
		
		static int newCount = 1;
		ImGui::DragInt("New Size", &newCount, 1);
		if (newCount < 1) newCount = 1;

		if (ImGui::Button("Resize")) {
			sv::renderLayer_count_set(newCount);
		}

		return true;
	}

}