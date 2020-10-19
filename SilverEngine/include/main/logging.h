#pragma once

#include "core.h"

namespace sv {

	// Window Messages

	enum MessageStyle : ui32 {
		MessageStyle_None = 0u,

		MessageStyle_IconInfo		= SV_BIT(0),
		MessageStyle_IconWarning	= SV_BIT(1),
		MessageStyle_IconError		= SV_BIT(2),

		MessageStyle_Ok				= SV_BIT(3),
		MessageStyle_OkCancel		= SV_BIT(4),
		MessageStyle_YesNo			= SV_BIT(5),
		MessageStyle_YesNoCancel	= SV_BIT(6),
	};
	typedef ui32 MessageStyleFlags;

	int show_message(const wchar* title, const wchar* content, MessageStyleFlags style = MessageStyle_None);

	// logging system

#ifndef SV_ENABLE_LOGGING

#define SV_LOG_CLEAR()
#define SV_LOG_SEPARATOR()
#define SV_LOG()
#define SV_LOG_INFO()
#define SV_LOG_WARNING()
#define SV_LOG_ERROR()

#else

	void __internal__do_not_call_this_please_or_you_will_die__console_log(ui32 id, const char* s, ...);
	void __internal__do_not_call_this_please_or_you_will_die__console_clear();

#define SV_LOG_CLEAR() sv::__internal__do_not_call_this_please_or_you_will_die__console_clear()
#define SV_LOG_SEPARATOR() sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, "----------------------------------------------------")
#define SV_LOG(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(0u, x, __VA_ARGS__)
#define SV_LOG_INFO(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(1u, "[INFO] " x, __VA_ARGS__)
#define SV_LOG_WARNING(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(2u, "[WARNING] " x, __VA_ARGS__)
#define SV_LOG_ERROR(x, ...) sv::__internal__do_not_call_this_please_or_you_will_die__console_log(3u, "[ERROR] " x, __VA_ARGS__)

#endif

}