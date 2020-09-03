#pragma once

#include "core.h"

namespace sv {

	struct InitializationConsoleDesc {
		bool show;
		const char* logFolder;
	};

	// Console

	void console_show();
	void console_hide();

	// logging system

	enum LoggingFlags : ui32 {
		LoggingFlags_None,
		LoggingFlags_Red				= SV_BIT(0),
		LoggingFlags_Green				= SV_BIT(1),
		LoggingFlags_Blue				= SV_BIT(2),
		LoggingFlags_BackgroundRed		= SV_BIT(3),
		LoggingFlags_BackgroundGreen	= SV_BIT(4),
		LoggingFlags_BackgroundBlue		= SV_BIT(5),
	};

	void log_separator();
	void log(const char* s, ...);
	void log_ex(const char* s, ui32 flags, ...);

	void log_info(const char* s, ...);
	void log_warning(const char* s, ...);
	void log_error(const char* s, ...);

}