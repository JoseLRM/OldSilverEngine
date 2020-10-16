#pragma once

#include "core.h"

namespace sv {

	Result console_initialize(bool show, const char* logFolder);
	Result console_close();

}