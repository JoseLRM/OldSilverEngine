#pragma once

#include "asset_system.h"

namespace sv {

	Result assets_initialize(const char* assetsFolderPath);
	Result assets_close();
	Result assets_update(float dt);

}