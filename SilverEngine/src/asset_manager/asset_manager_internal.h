#pragma once

#include "asset_manager.h"

namespace sv {

	bool asset_manager_initialize();
	void asset_manager_update(float dt);
	bool asset_manager_close();

}