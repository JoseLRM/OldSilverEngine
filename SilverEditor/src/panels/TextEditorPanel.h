#pragma once

#include "Panel.h"

namespace sv {

	class TextEditorPanel : public Panel {
	public:
		TextEditorPanel(const char* filePath);

	protected:
		bool onDisplay() override;

	private:
		std::string m_FilePath;
		std::string m_Content;

	};

}