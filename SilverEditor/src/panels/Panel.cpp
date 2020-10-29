#include "core_editor.h"
#include "Panel.h"

namespace sv {
	Panel::Panel(const char* name) : m_Name(name)
	{}

	void Panel::display()
	{
		if (!m_Enabled) return;

		beginDisplay();

		bool open;
		if (ImGui::Begin(getName(), &open, getWindowFlags())) {

			m_Visible = true;
			m_Focused = ImGui::IsWindowFocused();
			ImVec2 size = ImGui::GetWindowSize();
			m_Size.set(ui32(size.x), ui32(size.y));

			if (!onDisplay()) m_Enabled = false;

		}
		else {
			m_Visible = false;
			m_Focused = false;
		}
		ImGui::End();

		endDisplay();
	}

	void Panel::show()
	{
		m_Enabled = true;
	}

	void Panel::hide()
	{
		m_Enabled = false;
		m_Focused = false;
	}

}