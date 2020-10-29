#pragma once

#include "core_editor.h"

#include "panels/Panel.h"

namespace sv {

	Result	viewport_initialize();
	Result  viewport_close();
	void		viewport_display();

	void		viewport_add(const char* name, Panel* viewport);
	Panel*		viewport_get(const char* name);
	void		viewport_rmv(const char* name);

}