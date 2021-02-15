#pragma once

#include "core.h"

namespace sv {

	struct InitializationDesc {

		ApplicationCallbacks	callbacks;
		WindowDesc				windowDesc;
		u32						minThreadsCount;

	};

	Result engine_initialize(const InitializationDesc* desc);
	Result engine_loop();
	Result engine_close();

}