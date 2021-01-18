#pragma once

#include "core.h"

namespace sv {

	struct InitializationDesc {

		ApplicationCallbacks	callbacks;
		WindowDesc				windowDesc;
		u32						minThreadsCount;
		const char*				assetsFolderPath;

	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

}