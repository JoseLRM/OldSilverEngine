#include "core_editor.h"
#include "Viewport.h"

namespace sve {
	Viewport::Viewport(const char* name) : m_Name(name)
	{}

	void Viewport::display()
	{
		if (!m_Enabled) return;

		beginDisplay();

		if (ImGui::Begin(getName(), nullptr, getWindowFlags())) {

			m_Visible = true;
			m_Focused = ImGui::IsWindowFocused();
			ImVec2 size = ImGui::GetWindowSize();
			m_Size.set(size.x, size.y);

			if (!onDisplay()) m_Enabled = false;

		}
		else {
			m_Visible = false;
			m_Focused = false;
		}
		ImGui::End();

		endDisplay();
	}

	void Viewport::show()
	{
		m_Enabled = true;
	}

	void Viewport::hide()
	{
		m_Enabled = false;
		m_Focused = false;
	}

}