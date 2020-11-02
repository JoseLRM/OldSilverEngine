#include "core_editor.h"

#include "TextEditorPanel.h"
#include "utils/io.h"

namespace sv {
	
	TextEditorPanel::TextEditorPanel(const char* filePath) : m_FilePath(filePath)
	{
		m_FilePath = filePath;
#ifdef SV_RES_PATH
		std::string filePathStr = SV_RES_PATH;
		filePathStr += filePath;
		filePath = filePathStr.c_str();
#endif

		std::ifstream file(filePath);
		file >> m_Content;
		file.close();
	}

	bool TextEditorPanel::onDisplay()
	{
		ImGui::InputTextMultiline("test.txt", m_Content.data(), m_Content.size());

		return true;
	}
}