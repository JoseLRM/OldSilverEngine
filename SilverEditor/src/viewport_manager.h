#pragma once

#include "core_editor.h"

#include "viewports/Viewport.h"

namespace sve {

	sv::Result	viewport_initialize();
	sv::Result  viewport_close();
	void		viewport_display();

	void		viewport_add(const char* name, Viewport* viewport);
	Viewport*	viewport_get(const char* name);
	void		viewport_rmv(const char* name);

}