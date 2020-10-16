#pragma once

#include "core.h"

namespace sv {

	// Console

	void console_show();
	void console_hide();
	void console_clear();

	// logging system

	enum LoggingStyle : ui32 {
		LoggingStyle_None,
		LoggingStyle_Red				= SV_BIT(0),
		LoggingStyle_Green				= SV_BIT(1),
		LoggingStyle_Blue				= SV_BIT(2),
		LoggingStyle_BackgroundRed		= SV_BIT(3),
		LoggingStyle_BackgroundGreen	= SV_BIT(4),
		LoggingStyle_BackgroundBlue		= SV_BIT(5),
	};
	typedef ui32 LoggingStyleFlags;

	void console_log_separator();
	void console_log(const char* s, ...);
	void console_log(LoggingStyleFlags style, const char* s, ...);
	void console_log_error(bool fatal, const char* title, const char* content, ...);

}