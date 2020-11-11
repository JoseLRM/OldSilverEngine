#pragma once

#include "core_editor.h"

#include "panels/Panel.h"

namespace sv {

	Result		panel_manager_initialize();
	Result		panel_manager_close();
	void		panel_manager_display();

	void		panel_manager_add(const char* name, Panel* viewport);
	Panel*		panel_manager_get(const char* name);
	void		panel_manager_rmv(const char* name);

}